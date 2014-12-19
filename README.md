DisplaySwitch
=============

Windows commandline utility to switch the display between the HDMI out and Monitor ports of your video card.
The Monitor can be either VGA or DVI (but not both).

Limited utility that as-built will only allow you to choose the Monitor output or HDMI output or output to both at the same time.
I wrote this so I could bind it to a hotkey on my keyboard and quickly switch between my monitor and TV since Windows had a tendency to switch to the TV just because I turned on my receiver and I didn't want to have to switch the output just to be able to see what I was doing in order to switch the output back to my monitor.

If your setup isn't using one of the supported outputs this program will just fail.

For best operation, you should have set up the outputs manually at least once as this program will tell Windows to use existing configurations. If no existing configurations exist it will allow Windows to create them, but you may not get the layout/resolution you want.

Usage:
        DisplayController_Config.exe [new_output]
                1: Monitor (DVI)
                2: HDMI
                3: Both
                4: Cycle

        No parameter means Cycle
