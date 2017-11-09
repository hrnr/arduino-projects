# Smart agriculture with arduino

## reader.py

Reads sensors data from serial and outputs csv on stdout. Format is:

```
datetime(ISO 8601),thermistor(temperature),moisture,photoresitor(light),button
```

## ctl.py

Controls the pump attached to arduino. Reads commands on stdin and sends them to arduino in binary format.

Command format is :


```
CMD ARG
```

CMD is one letter identifier of command and ARG is *mandatory* unsigned 16-bit integer argument (as text).

### List of commands

* `R 0` - starts pump
* `S 0` - stops pump
* `T <duration is seconds>` - runs pump for given duration

###TODO

[] water level warning
[] automated mode
