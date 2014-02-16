#include <Windows.h>
#include <cstdio>
#include <cstring>

using namespace std;

enum DisplaySelection {
	DS_MONITOR = 1,
	DS_HDMI = 2,
	DS_TOGGLE = 3,
};

char *DisplaySelectionString[] = {
	"Monitor",
	"HDMI",
	"Toggle",
};

#define MAX_PATH_ELEMENTS 10
#define MAX_MODE_ELEMENTS 10

int main(int argc, char *argv[])
{
	LONG retval;
	DISPLAYCONFIG_PATH_INFO pathArray[MAX_PATH_ELEMENTS];
	DISPLAYCONFIG_MODE_INFO modeArray[MAX_MODE_ELEMENTS];
	UINT32 numPathElements = sizeof(pathArray)/sizeof(pathArray[0]);
	UINT32 numModeElements = sizeof(modeArray)/sizeof(modeArray[0]);
	UINT32 hdmi_idx = MAX_PATH_ELEMENTS;
	UINT32 dvi_idx = MAX_PATH_ELEMENTS;
	DisplaySelection current_selection = DS_TOGGLE;

	retval = QueryDisplayConfig(QDC_ALL_PATHS, &numPathElements, pathArray, &numModeElements, modeArray, NULL);

	if (ERROR_SUCCESS != retval) {
		printf("Query failure - unable to set new path!!\n");
		return 2;
	}

	/* Find the target paths we care about */
	for (unsigned int path = 0; path < numPathElements; path++) {
		if (pathArray[path].sourceInfo.modeInfoIdx >= 0
			&& pathArray[path].sourceInfo.modeInfoIdx < numModeElements) {

			if (pathArray[path].targetInfo.outputTechnology == DISPLAYCONFIG_OUTPUT_TECHNOLOGY_HDMI) {
				hdmi_idx = path;
				if (DISPLAYCONFIG_PATH_ACTIVE & pathArray[path].flags)
					current_selection = DS_HDMI;
			}
			else if (pathArray[path].targetInfo.outputTechnology == DISPLAYCONFIG_OUTPUT_TECHNOLOGY_DVI) {
				dvi_idx = path;
				if (DISPLAYCONFIG_PATH_ACTIVE & pathArray[path].flags)
					current_selection = DS_MONITOR;
			}
		}
	}
	if (current_selection == DS_TOGGLE) {
		printf("Unable to determine current video output -- exiting\n");
		return 3;
	}

	/* No argument means toggle output */
	unsigned int input = DS_TOGGLE;
	DISPLAYCONFIG_PATH_INFO desiredSettings;

	/* Try to get the desired output setting */
	if (argc > 1) {
		input = atoi(argv[1]);
	}

	/* input should match the enum above. indexed into the string array as (input - 1) */
	if ((input - 1) < (sizeof(DisplaySelectionString) / sizeof(DisplaySelectionString[0]))) {
		printf("Device to be selected is the %s selection.\n", DisplaySelectionString[input - 1]);
		if ((DS_MONITOR == input && current_selection == DS_HDMI) || (DS_TOGGLE == input && DS_HDMI == current_selection)) {
			desiredSettings = pathArray[dvi_idx];
		}
		else if ((DS_HDMI == input && current_selection == DS_MONITOR) || (DS_TOGGLE == input && DS_MONITOR == current_selection))
		{
			desiredSettings = pathArray[hdmi_idx];
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
		for (int i = 0; i < sizeof(DisplaySelectionString) / sizeof(DisplaySelectionString[0]); i++) {
			printf("\t\t%d: %s\n", i + 1, DisplaySelectionString[i]);
		}
		printf("\n\tNo parameter means Toggle\n");
		return 1;
	}

	/* If we get here we will attempt to set the path -- let SetDisplayConfig pick the mode info */
	desiredSettings.flags = DISPLAYCONFIG_PATH_ACTIVE;
	desiredSettings.targetInfo.modeInfoIdx = DISPLAYCONFIG_PATH_MODE_IDX_INVALID;
	desiredSettings.sourceInfo.modeInfoIdx = DISPLAYCONFIG_PATH_MODE_IDX_INVALID;
	retval = SetDisplayConfig(1, &desiredSettings, 0, NULL, SDC_APPLY | SDC_TOPOLOGY_SUPPLIED | SDC_ALLOW_PATH_ORDER_CHANGES);

	if (ERROR_SUCCESS != retval) {
		printf("Unable to set -- trying again with SetDisplayConfig creating mode info\n");
		retval = SetDisplayConfig(1, &desiredSettings, 0, NULL, SDC_TOPOLOGY_SUPPLIED | SDC_ALLOW_CHANGES |
													SDC_ALLOW_PATH_ORDER_CHANGES |SDC_USE_SUPPLIED_DISPLAY_CONFIG);
		if (ERROR_SUCCESS != retval) {
			printf("Unable to set the new display config!!\n");
			return 3;
		}
	}

	return 0;
}