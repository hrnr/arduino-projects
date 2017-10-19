#!/usr/bin/env python3

# reads data from arduino

import serial
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
from datetime import datetime

time_data = []
photoresistor_data = []

# setup graph
plt.ion()
fig = plt.figure()
fig.autofmt_xdate()
ax = fig.add_subplot(111)
xfmt = mdates.DateFormatter('%X')
ax.xaxis.set_major_formatter(xfmt)
line1, = ax.plot([], [], 'b-')

def readData(bytesline):
	line = bytesline.decode("utf-8")
	thermistor, photoresistor = [x.strip() for x in line.split(' ')]
	print(thermistor, photoresistor)
	time_data.append(datetime.now())
	photoresistor_data.append(photoresistor)
	update_plot()

def update_plot():
	line1.set_xdata(time_data)
	line1.set_ydata(photoresistor_data)

	ax.relim()
	ax.autoscale_view()
	fig.canvas.draw()
	plt.pause(0.01)

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
