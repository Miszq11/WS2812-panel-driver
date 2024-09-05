# WS2812 LED panel driver

The aim of this project is to create Linux Kernel driver for WS2812 LED panel.
Main objectives are to:
* create linux kernel module,
* create app which will utilize said driver capabilities,
* create sample animations with at least one with panning,
* utilize Buildroot tools,
* utilize [framebuffer API](https://docs.kernel.org/fb/api.html).

# Table of contents
  1. [About the project](#about-the-project)
      - [How LED are driven](#how-led-are-driven)
      - [How it works](#how-it-works)
      - [Harware used](#hardware-used)
  2. [Development](#development)
      - [About repository structure](#about-repository-structure)
      - [Kernel modules](#kernel-modules)
      - [Building setup](#building-setup)
      - [Running](#running)
      - [Known bugs](#known-bugs)
      - [Work status](#work-status)
      - [Why & Hows](#why--hows)
          - [framebuffes](#framebuffer)
          - [Workqueue](#work-queue)
          - [SPI](#spi)
          - [Module split](#module-split)


# About the project

## How LED are driven

WS2812 LEDs are driven by a single data line in which 0 and 1 is represented
via different pulse width. To achieve that we utilized SPI MOSI output with 16 bit
word size. By sending a word with different amount of ones and zeros we were able
to get timings according to specification and therefore ensure the proper operation
of the panel.

Word length of 16 bits helped to get timings under controll, because it enabled
tuning timings with a good precision. Basically SPI was used as an PWM generator
with 4 bit resolution.

## How it works
Video of the working display can be found on one of the contributors youtube channel.

[![Watch the video](https://img.youtube.com/vi/OZ-iTeJUp58/maxresdefault.jpg)](https://youtu.be/OZ-iTeJUp58 "Click to watch video")

## Hardware used
- STM32MP157C-DK2 Development board

![STM32MP157C-DK2](https://docs.zephyrproject.org/2.7.5/_images/en.stm32mp157c-dk2.jpg)

- 8x8 WS2812B LED panel

![Examplary panel](https://botland.com.pl/img/art/inne/06182_3.jpg)


# Development
## About repository structure

  This repository is split into serval directories containing different parts of
  the project.

  ```sh
WS2812-panel-driver/
├── BR
├── br_board_configs
│   └── dts
├── br_configs
├── doc
├── external
│   ├── local
│   └── module
│       ├── app_src
│       │   └── external
│       ├── module_src
│       │   └── startup_script
│       └── package
│           ├── WS2812_app
│           └── WS2812_module
├── qemu
└── scripts
    └── BR_builder
```

```BR```, ```external/local``` directories are listed in .git ignore.
First one is where buildroot will unpack, and the latter is local
for user files (compile_commands, etc...).

```external/module/package``` contains buildroots external module informations
of all implemented modules and apps, including:

  - xx.mk - make files for handling package,
  - Config.in - for handling menuconfig options.

```external/module/*_src``` directories contains module and app source codes,
startup scripts, build system instructions and external libraries.

## Kernel modules

There are two main modules and one side module, that exports all common
functions.

```WS2812-panel-driver/external/module/module_src/ws2812_mod.c``` is a test module,
that will latch onto SPI bus 0 and perform all normal operations (register frame buffer,
register spi, initialise workqueue).

```WS2812-panel-driver/external/module/module_src/ws2812_spi.c``` is a SPI device driver.
All corresponding dts entries will probe this module, which will then read all necessary
dts variables values, and perform rest of the module work.

## Building setup

After cloning the repository, make sure to initialize submodules:

```sh
  git submodule update --init --recursive
```

This will pull all external github repositories containing libraries code.

Next unpack buildroot:

```sh
  cd BR
  wget https://buildroot.org/downloads/buildroot-xxxx.xx.x.tar.gz
  tar -xvf buildroot-xxxx.xx.x.tar.gz
```

or use setup script:
```sh
  chmod u+x scripts/setup.sh && ./script/setup.sh
```

setup all necessary configurations for system to work on your board.
You can use a defconfig or setup all things mannually
(follow [Buildroot quick start](https://buildroot.org/downloads/manual/manual.html#_buildroot_quick_start)).

Enable this module and all needed options in ```External options -> WS2812_panel```:

```sh
  cd BR/buildroot-xxxx.xx.x
  make menuconfig BR2_EXTERNAL=../../external/module
```

Finally you can build the system with:

```sh
  cd BR/buildroot-xxxx.xx.x
  make all
```

## Running

Run module with:

```sh
  modprobe/insmod ws2812_spi
```
or:
```sh
  modprobe/insmod ws2812_mod parameters=here
```

Run app with:

```sh
WS2812_app /dev/fbX
```

## Documentation
  Doxygen code documentation is available in gh-pahes [here](https://miszq11.github.io/WS2812-panel-driver/).

## Known bugs

  - Pixels not showing correct colors
      - SPI timings off (on STM32MP157C-DK2 board)

## Work status

Achieved functionality:

| What | Status |
| :--- | :---: |
| SPI device driver | <b style="color:green"> Ok </b> |
| DTS support | <b style="color:green"> Ok </b> |
| SPI to WS2812 communication | <b style="color:green"> Ok </b> |
| SPI signal integrity | <b style="color:green"> Ok (barely) </b> |
| SPI DMA tx buff | <b style="color:#f03000"> No, <br> testing </b> |
| convert pixel data to spi data | <b style="color:green"> Ok </b> |
| convert in work | <b style="color:green"> Ok </b> |
| User app | <b style="color:green"> Ok </b> |
| proper module deinitialisation | <b style="color:#f03000"> Partial </b>|
| framebuffer support: | <b style="color:green"> Ok </b> |
| fb buffor allocation | <b style="color:yellow"> Ok? </b> |
| pixel buffer mapping <br> to user space | <b style="color:green"> Ok </b> |
| Display panning | <b style="color:yellow"> Ok? </b> |
| Constant framerate | <b style="color:#f03000"> Not implemented </b> |
| Ioctl draw | <b style="color:green"> Ok </b> |

## Why & Hows

### Framebuffer
  Length of pixel data buffor is a product of x_virt_size and y_virt_size, for
  further panning to work. This is because of the lack of found documentation and
  example uses, of Panning functionality in fb.

  Module allows the write of whole pixel buffer (even the virtual part)
  and then utilizes xoffset, yoffset and line_length (visible line length?),
  to display currently seen pixels (xres by yres). This can be achieved
  by mapping the buffer into user space app.

  Linux system does automatically call imageblit on every framebuffers,
  so this functionality is disabled (empty function) in module. The source
  of that call is yet to be discovered, thats why this topic is
  scheduled for further investigation.

### Work queue

  Developers wanted to use workqueue for converting pixel rgb data
  into SPI transfers, just to avoid locking the kernel code execution
  for long time.

  Used LED specific gamma correction is for now hardcoded in the
  work function, but probably will be set to optional and
  implemented dynamic in further development.

### SPI

  All SPI transfers uses single spi_message, set up at the initialisation
  of the module. This is because every transfer is the same and the only
  difference is in data stored in buffor. This buffor is provided by its
  address.

  Currently there is no DMA support, but all facilities are ready to be
  implemented to use it.

  As mentioned before, SPI was used as a way of generating PWM signal needed
  by WS2812 LEDs. Using 16 bit SPI word size enbled us to
  change PWM pulse width quite easily and
  with moderate precision.

  As explained in LED [datasheet](https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf) pulse width may be changed in every
  cycle. Using plain PWM driver that may be difficult to achieve without constant
  function calls, which in consequence may cause signal timings to break. With use of SPI data as PWM signal we mitigated this problem, because one can create buffer of _n_ messages and push it to be sent without worrying much about timings (only true if one sets correct SPI clock beforehand).
### Module Split

  There are two main developers of this repository, where only one
  of them is in possesion of a board and LED panel. Thats why main
  modules are splited in two, and one common module.

  ws2812_mod is mainly designed for testing the functionality of
  framebuffer, mapping, spi data preparation, where ws2812_spi
  aims for real life testing on the device.

  ws2812_common module, exports all necessary functions that are
  used in both modules.

