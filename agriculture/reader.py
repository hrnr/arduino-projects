#!/usr/bin/env python3

# reads data from arduino send as binary message, writes sensor readings to
# stdout as csv

import serial
from datetime import datetime
from ctypes import *
import sys

class Measurements(Structure):
	_pack_ = 1
	_fields_ = [
		("temperature", c_uint16),
		("moisture", c_uint16),
		("light", c_uint16),
		("button", c_uint8),
		("pump_on", c_uint8),
		("heating_on", c_uint8),
		("temperature_level", c_uint16),
		("moisture_level", c_uint16),
		("water_capacity", c_uint16),
		("remaining_water", c_uint16),
		("auto_mode", c_uint8),
	]

def readData(data):
	print(datetime.now().isoformat(), data.temperature, data.moisture, data.light, data.button,
		data.pump_on, data.heating_on, data.temperature_level, data.moisture_level, data.water_capacity,
		data.remaining_water, data.auto_mode, sep=',')
	sys.stdout.flush()

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
