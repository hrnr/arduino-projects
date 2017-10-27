#!/usr/bin/env python3

# send commands to arduino

import serial
from ctypes import *
import sys

class Command(Structure):
	_fields_ = [
		("cmd", c_uint16),
		("value", c_uint16),
	]

ser = serial.Serial(
	port='/dev/ttyACM0',
	baudrate=115200)

for line in sys.stdin:
    cmd, value = line.strip().split(' ')
    command = Command()
    command.cmd = ord(cmd)
    command.value = int(value)
    print(sizeof(command))
    print(ser.write(command))

ser.flush()
ser.close()
