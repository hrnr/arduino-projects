#!/usr/bin/env python3

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.dates as mdates


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

def readData():
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