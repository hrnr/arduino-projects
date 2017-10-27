#!/usr/bin/env python3

# reads data from arduino send as binary message, writes sensor readings to
# stdout as csv

import serial
from datetime import datetime
from ctypes import *

class Measurements(Structure):
	_fields_ = [
		("temperature", c_uint16),
		("moisture", c_uint16),
		("light", c_uint16),
		("button", c_uint16),
	]

def readData(data):
	print(datetime.now().isoformat(), data.temperature, data.moisture, data.light, data.button, sep=',')

ser = serial.Serial(
	port='/dev/ttyACM0',
	baudrate=115200)

try:
	while True:
		m = Measurements()
		bytesarray = ser.read(sizeof(m))
		m = Measurements.from_buffer_copy(bytesarray)
		readData(m)
except KeyboardInterrupt:
	pass
finally:
	ser.close()
