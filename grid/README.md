Dancing Robot
=============

video: https://youtu.be/vJE2oU5vaBA

features
--------

* saving schedule to EEPROM
* special action on request
* choosing between user and built-in schedule

building
--------

Source code is in grid.ino (arduino ide) file. It can be build by using
Makefile based on [Arduino-Makefile](https://github.com/sudar/Arduino-Makefile).

I have used backported `EEPROM.h` from arduino 1.8.1, so this project can be
built with older versions of arduino too.

schedules
---------

There are several schedules in the `inputs` directory. `input.txt` is default
built-in schedule. `input_lowercase.txt` is the same, demonstrating that the
schedules are not case-sensitive. `input_small.txt` is schedule for 2x2 grid
seen in the video. `input_invalid_*` schdules should not be accepted by the
robot (see serial output).

starting robot
--------------

After robot is started (arduino boots up), it waits 15 seconds for user to
send schedule via serial line.

There is a little script `send-schedule.sh` to make sending schedule easier
for user using `screen`. Just start screen as

	screen -S arduino /dev/ttyACM0 115200

and then use script as

	./send-schedule.sh <path to input>

The new schedule will be persisted in EEPROM.

If no schedule is sent during 15 seconds, robot loads saved schedule from
EEPROM. If there is no schedule in EEPROM, robot will load build-in schedule
for 4x4 grid. Load of built-in schedule is indicated by 1 blink.

User can override this behavior to always load built-in schedule by holding the
button during startup.

After schedule is loaded robot blinks twice to mark it is ready.

executing schedule
------------------

Pressing the button will make robot follow the schedule. Robot will wait after
reaching each goal according to the schedule. This waiting is indicated by
blinking led.

When robot waits for its next goal, user can command robot to do the special
action by long press of the button. The special action will rotate robot so
that it faces NORTH in the coordinate system. Performing this action may be
unsafe on the edges of the grid where robot might get lost because of missing
markers. After performing the special action, robot continues executing the
rest of the schedule.

When robot finishes executing the schedule it blinks 4 times. Pressing the
button commands robot to return to the starting position. After robot returns
to the starting positions it blinks 4 times.
