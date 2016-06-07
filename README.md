[![Build Status](https://travis-ci.org/msalau/rl78flash.svg?branch=master)](https://travis-ci.org/msalau/rl78flash)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/5448/badge.svg)](https://scan.coverity.com/projects/5448)

# About

This is a PC software to program RL78 microcontrollers via serial bootloader.

Features:
* This version can not set any security mode;
* Only S-record image files are accepted as input files;
* RL78/G10 parts can be programmed only in 1-wire mode,
  other RL78 parts support both modes (1-wire and 2-wire);
* Reset signal is controlled by DTR or RTS signal.

# Thanks to

* Hiroaki Okoshi
* Zurab aka kuber from http://electronix.ru/forum
* MON-80 at http://mon80.blog.fc2.com

# Download

Binary version for Windows can be downloaded from
https://github.com/msalau/rl78flash/releases

Linux users are encouraged to build rl78flash from source.

# Build

```
git clone https://github.com/msalau/rl78flash
cd rl78flash
make
```

# Usage examples

Show information about a target MCU and write a mot-image to it
```
$ rl78flash -viva /dev/ttyUSB0 firmware.mot
```

Write an RL78/G10 part that have 2k of flash, verify and reset the MCU
```
$ rl78g10flash -vvwcr /dev/ttyUSB0 firmware.mot 2k
```

See also output from
```
$ rl78flash -h
```
and
```
$ rl78g10flash -h
```

### Rewrite an MCU with the RESET pin acting as a GPIO

If the RESET pin of an MCU is acting as a GPIO it is no longer possible to
enter bootloader mode without additional user actions. To enter bootloader
mode it is necessary to power up the MCU with the RESET pin tied to ground.

Step-by-step procedure:

1. power down the MCU;
2. start rl78flash with -d option;
3. wait for the message "Turn MCU's power on and press any key...";
4. power up the MCU;
5. press any key;
6. that's all.

Example: Erase RPBRL78G14 with RESET acting as a GPIO
```
$ ./rl78flash -dive -m3 /dev/ttyUSB0
Turn MCU's power on and press any key...
Device: R5F104LE
Code size: 64 kB
Data size: 4 kB
Erase

$
```

This procedure is valid for rl78g10flash too.

# License

The MIT License (MIT)
Copyright (c) 2012-2016 Maksim Salau

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
