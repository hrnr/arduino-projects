#!/usr/bin/env python3

import sys
import dateutil.parser
import matplotlib.pyplot as plt
import matplotlib.dates as mdates

time_data = []
sensor_data = []

# setup plot
plt.ion()
fig = plt.figure()
fig.autofmt_xdate()
ax = fig.add_subplot(111)
xfmt = mdates.DateFormatter('%X')
ax.xaxis.set_major_formatter(xfmt)
line1, = ax.plot([], [], 'b-')

def update_plot():
	line1.set_xdata(time_data)
	line1.set_ydata(sensor_data)

	ax.relim()
	ax.autoscale_view()
	fig.canvas.draw()
	plt.pause(0.01)

for line in sys.stdin:
	values = line.split(',')
	date = dateutil.parser.parse(values[0])
	sensor = values[2] # moisture

	time_data.append(date)
	sensor_data.append(sensor)

	update_plot()
