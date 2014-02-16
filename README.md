DisplaySwitch
=============

Windows commandline utility to switch the display between the HDMI and DVI ports of your video card.

Limited utility that as-built will only allow you to choose the DVI output or HDMI output.
I wrote this so I could bind it to a hotkey on my keyboard and quickly switch between my monitor and TV since Windows had a tendency to switch to the TV just because I turned on my receiver and I didn't want to have to switch the output just to be able to see what I was doing in order to switch the output back to my monitor.

If your setup isn't using one of these outputs this program will just fail.

Usage:
        DisplayController_Config.exe [new_output]
                1: Monitor (DVI)
                2: HDMI
                3: Toggle

        No parameter means Toggle
