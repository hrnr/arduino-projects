#!/usr/bin/env python3

# reads data from arduino, writes sensor reading to stdout as csv

import serial
from datetime import datetime

def readData(bytesline):
	line = bytesline.decode("utf-8")
	thermistor, photoresistor = [x.strip() for x in line.split(' ')]
	print(datetime.now().isoformat(), thermistor, 0, photoresistor, 0, sep=',')

ser = serial.Serial(
	port='/dev/ttyACM0',
	baudrate=115200)

try:
	while True:
		readData(ser.readline())
except KeyboardInterrupt:
	pass
finally:
	ser.close()
