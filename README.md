custos
======

> Quis custodiet ipsos custodes?

— Decimus Junius Juvenalis

System monitoring utility.

Several modules exist to monitor different aspects of the system (see
[modules](#modules)).  A simple Lua file is used to configure the update
interval and the presentation (see [configuration](#configuration)), which can
be either simple text or `curses` windows.

Example
-------

```
load                            battery
  1.54 1.35 1.13                  [=========|------]  100%|57.66% 0
date                              ▄▄▄▄▄▄▄▄▃▃▃▃▃▃▃▃▅▆▇█████        Full
  P   2024-06-18T01:53:21       backlight
  E   2024-06-18T04:53:21         [================]  100% 0
  BR  2024-06-18T05:53:21       fs
  UTC 2024-06-18T08:53:21         [=============---] 81.6%  51G/ 62G /
  CE  2024-06-18T10:53:21         [=============---] 83.0% 135G/162G /home
                                  [----------------]  0.5%  36M/7.8G /tmp
                                thermal
                                  [=======---------] 46.0°C Package id 0
                                  [=======---------] 45.0°C Core 0
                                  [=======---------] 46.0°C Core 1
                                  [=======---------] 45.0°C Core 2
                                  [=======---------] 46.0°C Core 3
```

Configuration file:

```lua
modules { "load", "backlight", "battery", "fs", "thermal", "date" }

windows {
    {modules = {"load", "date"}},
    {modules = {"battery", "backlight", "fs", "thermal"}, x = 32},
}

timezones {
    {"P  ", "US/Pacific"},
    {"E  ", "US/Eastern"},
    {"BR ", "America/Sao_Paulo"},
    {"UTC", "UTC"},
    {"CE ", "Europe/Prague"},
}

backlights {
    {"0", "/sys/class/backlight/intel_backlight/brightness"},
}

batteries {
    {"0", "/sys/class/power_supply/BAT0"},
}

thermal {
    glob "/sys/devices/platform/coretemp.0/hwmon/hwmon*/temp*_input"
}

file_systems {
    "/",
    "/home",
    "/tmp",
}
```

Build
-----

```console
$ premake5 gmake2
$ make
```

Modules
-------

Modules can be individually enabled/disabled (see
[configuration](#configuration), and include:

- [backlight.c](src/custos/src/backlight.c): ACPI screen brightness level.
- [battery.c](src/custos/src/battery.c): battery charge and graph.
- [date.c](src/custos/src/date.c): date/time on one or more time zones.
- [fs.c](src/custos/src/fs.c): file system space utilization.
- [load.c](src/custos/src/load.c): system load average.
- [thermal.c](src/custos/src/thermal.c): ACPI thermal measurement.

Configuration
-------------

By default, all modules are loaded and presented vertically in sequence.  The
`modules` entry selects a subset:

```lua
modules { "date", "fs" }
```

The `windows` entry specifies a list of `curses` windows.  The list part
specifies the modules (and the order) to present, while the (optional) `x`, `y`,
`width`, and `height` keys can be used to position the window.

```lua
windows {
    {modules = {"load", "date"}},
    {modules = {"battery", "backlight", "fs", "thermal"}, x = 32},
}
```

Some modules are also configured using Lua (although they all have defaults):

### `batteries`

A list of ACPI directories and their associated names:

```lua
batteries {
    {"0", "/sys/class/power_supply/BAT0"},
}
```

### `backlight`

A list of ACPI directories and their associated names:

```lua
backlights {
    {"0", "/sys/class/backlight/intel_backlight/brightness"},
}
```

### `fs`

A list of file system directories.

```lua
file_systems {
    "/",
    "/home",
    "/tmp",
}
```

### `thermal`

A list of ACPI directories (the `glob` function is useful in this case):

```lua
thermal {
    glob "/sys/devices/platform/coretemp.0/hwmon/hwmon*/temp*_input"
}
```

### `date`

A list of time zone names and their associated (display) name:

```lua
timezones {
    {"P  ", "US/Pacific"},
    {"E  ", "US/Eastern"},
    {"BR ", "America/Sao_Paulo"},
    {"UTC", "UTC"},
    {"CE ", "Europe/Prague"},
}
```
