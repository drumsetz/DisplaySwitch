#include <Windows.h>
#include <cstdio>
#include <cstring>

using namespace std;

enum DisplaySelection {
	DS_MONITOR = 1,
	DS_HDMI = 2,
	DS_BOTH = 3,
	DS_CYCLE = 4,
};

/* This array must match the above enum */
const char *DisplaySelectionStrings[] = {
	"Monitor",
	"HDMI",
	"Both",
	"Cycle",
};
static const unsigned int MaxSelectionStrings = sizeof(DisplaySelectionStrings) / sizeof(DisplaySelectionStrings[0]);

#define DS_SELECT_NONE_FLAG 0
#define DS_SELECT_MONITOR_FLAG 1
#define DS_SELECT_HDMI_FLAG 2
typedef unsigned int DisplaySelectionFlags;

/* This array is used to list the order of output combos for the Cycle option.
   It is also contains the flags for each selection at (DS_* - 1) */
const DisplaySelectionFlags OptionFlags[] = {
	DS_SELECT_MONITOR_FLAG,                       /* DVI */
	DS_SELECT_HDMI_FLAG,                          /* HDMI */
	DS_SELECT_MONITOR_FLAG | DS_SELECT_HDMI_FLAG, /* Both */
};
static const unsigned int MaxOptions = sizeof(OptionFlags) / sizeof(OptionFlags[0]);

static DisplaySelectionFlags GetNextOption(DisplaySelectionFlags current_selection)
{
	unsigned int curr_idx = 0;
	for (curr_idx = 0; curr_idx < MaxOptions; curr_idx++) {
		if (OptionFlags[curr_idx] == current_selection)
			break;
	}
	if (curr_idx == MaxOptions) {
		return OptionFlags[0];
	}
	return OptionFlags[(curr_idx + 1) % MaxOptions];
}

#define MAX_PATH_ELEMENTS 128
#define MAX_MODE_ELEMENTS 8

int main(int argc, char *argv[])
{
	LONG retval;
	DISPLAYCONFIG_PATH_INFO pathArray[MAX_PATH_ELEMENTS];
	DISPLAYCONFIG_MODE_INFO modeArray[MAX_MODE_ELEMENTS];
	UINT32 numPathElements = sizeof(pathArray)/sizeof(pathArray[0]);
	UINT32 numModeElements = sizeof(modeArray)/sizeof(modeArray[0]);
	UINT32 hdmi_idx = MAX_PATH_ELEMENTS;
	/* Assuming that monitor is either DVI or VGA, and that only one is
	 * connected. Bugs will present if both are connected. */
	UINT32 monitor_idx = MAX_PATH_ELEMENTS;
	DisplaySelectionFlags current_selection = DS_SELECT_NONE_FLAG;

	retval = QueryDisplayConfig(QDC_ALL_PATHS, &numPathElements, pathArray, &numModeElements, modeArray, NULL);

	if (ERROR_SUCCESS != retval) {
		printf("Query failure - unable to set new path!!\n");
		if (ERROR_INSUFFICIENT_BUFFER == retval)
			printf("Insufficient buffer failure; increase MAX_PATH_ELEMENTS or MAX_MODE_ELEMENTS and re-compile.");
		return 2;
	}

	/* Find the target paths we care about */
	for (unsigned int path = 0; path < numPathElements; path++) {
		if (pathArray[path].sourceInfo.modeInfoIdx >= 0
			&& pathArray[path].sourceInfo.modeInfoIdx < numModeElements) {

			if ((pathArray[path].targetInfo.outputTechnology == DISPLAYCONFIG_OUTPUT_TECHNOLOGY_HDMI)
					&& (pathArray[path].targetInfo.targetAvailable == TRUE)) {
				hdmi_idx = path;
				if (DISPLAYCONFIG_PATH_ACTIVE & pathArray[path].flags)
					current_selection |= DS_SELECT_HDMI_FLAG;
			}
			/* Both DVI and VGA (HD15) are considered the monitor. We're assuming only one is actually connected. */
			/* An available DVI will take precedence over an available VGA out unless the VGA is currently active. */
			/* There will be problems if both are available (which should mean plugged in). */
			else if ((pathArray[path].targetInfo.outputTechnology == DISPLAYCONFIG_OUTPUT_TECHNOLOGY_DVI)
					&& (pathArray[path].targetInfo.targetAvailable == TRUE)
					&& ((DS_SELECT_MONITOR_FLAG & current_selection) == 0)) {
				if (monitor_idx != MAX_PATH_ELEMENTS)
					printf("Both VGA and DVI monitor outputs available; This is not properly supported.\n");
				printf("Monitor set to DVI technology.\n");
				monitor_idx = path;
				if (DISPLAYCONFIG_PATH_ACTIVE & pathArray[path].flags)
					current_selection |= DS_SELECT_MONITOR_FLAG;
			}
			else if ((pathArray[path].targetInfo.outputTechnology == DISPLAYCONFIG_OUTPUT_TECHNOLOGY_HD15)
					&& (pathArray[path].targetInfo.targetAvailable == TRUE)) {
				if (monitor_idx != MAX_PATH_ELEMENTS)
					printf("Both VGA and DVI monitor outputs available; This is not properly supported.\n");
				if (DISPLAYCONFIG_PATH_ACTIVE & pathArray[path].flags) {
					current_selection |= DS_SELECT_MONITOR_FLAG;
					printf("Monitor set to VGA technology because it is currently active.\n");
					monitor_idx = path;
				} else if (monitor_idx == MAX_PATH_ELEMENTS) {
					printf("Monitor set to VGA technology.\n");
					monitor_idx = path;
				}
			}

		}
	}
	if (current_selection == DS_SELECT_NONE_FLAG) {
		printf("Unable to determine current video output -- exiting\n");
		return 3;
	}

	/* No argument means cycle output */
	unsigned int input = DS_CYCLE;
	DISPLAYCONFIG_PATH_INFO desiredSettings[2];
	unsigned int num_desired_settings = 0;

	/* Try to get the desired output setting */
	if (argc > 1) {
		input = atoi(argv[1]);
	}

	/* input should match the enum above. indexed into the string array as (input - 1) */
	if ((input - 1) < MaxSelectionStrings) {
		printf("Device to be selected is the %s selection.\n", DisplaySelectionStrings[input - 1]);
		DisplaySelectionFlags desired_selection;
		if (DS_CYCLE == input)
			desired_selection = GetNextOption(current_selection);
		else
			desired_selection = OptionFlags[input - 1];

		if (desired_selection != current_selection)
		{
			/* HDMI will get priority if multiple outputs being attempted */
			if (desired_selection & DS_SELECT_HDMI_FLAG)
			{
				if (hdmi_idx < MAX_PATH_ELEMENTS)
					desiredSettings[num_desired_settings++] = pathArray[hdmi_idx];
				else {
					printf("Attempting to enable HDMI, but unable to find config element -- exiting\n");
					return 4;
				}

			}
			if (desired_selection & DS_SELECT_MONITOR_FLAG)
			{
				if (monitor_idx < MAX_PATH_ELEMENTS)
					desiredSettings[num_desired_settings++] = pathArray[monitor_idx];
				else {
					printf("Attempting to enable MONITOR, but unable to find config element -- exiting\n");
					return 5;
				}
			}
		}
		else
		{
			printf("Output already set to the desired selection.\n");
			return 0;
		}
	}
	else
	{
		printf("Unknown display selection: %d\n\n", input);
		printf("Usage:\n");
		printf("\tDisplayController_Config.exe [new_output]\n");
		for (int i = 0; i < MaxSelectionStrings; i++) {
			printf("\t\t%d: %s\n", i + 1, DisplaySelectionStrings[i]);
		}
		printf("\n\tNo parameter means Cycle\n");
		return 1;
	}

	/* If we get here we will attempt to set the path -- let SetDisplayConfig pick the mode info */
	unsigned int setting = 0;
	for (setting = 0; setting < num_desired_settings; setting++) {
		desiredSettings[setting].flags = DISPLAYCONFIG_PATH_ACTIVE;
		desiredSettings[setting].targetInfo.modeInfoIdx = DISPLAYCONFIG_PATH_MODE_IDX_INVALID;
		desiredSettings[setting].sourceInfo.modeInfoIdx = DISPLAYCONFIG_PATH_MODE_IDX_INVALID;
	}
	retval = SetDisplayConfig(num_desired_settings, desiredSettings, 0, NULL,
							SDC_APPLY | SDC_TOPOLOGY_SUPPLIED | SDC_ALLOW_PATH_ORDER_CHANGES);

	if (ERROR_SUCCESS != retval) {
		printf("Unable to set -- trying again with SetDisplayConfig creating mode info\n");
		retval = SetDisplayConfig(num_desired_settings, desiredSettings, 0, NULL, SDC_TOPOLOGY_SUPPLIED | SDC_ALLOW_CHANGES |
															SDC_ALLOW_PATH_ORDER_CHANGES |SDC_USE_SUPPLIED_DISPLAY_CONFIG);
		if (ERROR_SUCCESS != retval) {
			printf("Unable to set the new display config!!\n");
			return 3;
		}
	}

	return 0;
}