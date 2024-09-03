# WS2812 LED panel driver

The aim of this project is to create Linux Kernel driver for WS2812 LED panel.
Main objectives are to:
* create an external module loadable to the Kernel on startup,
* create app which will utilize said driver,
* create sample animations with at least one with panning,
* utilize Buildroot tools,
* utilize framebuffer.



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
Video of the working display can be found on one of the contributors youtube channel (subscribe!).

[![Watch the video](https://img.youtube.com/vi/VwPaDlaS9uE?si=0EJWLzdlCJsrLGLw/maxresdefault.jpg)](https://youtu.be/VwPaDlaS9uE?si=0EJWLzdlCJsrLGLw)

## Hardware used
STM32MP157C-DK2 Development board
![STM32MP157C-DK2](https://docs.zephyrproject.org/2.7.5/_images/en.stm32mp157c-dk2.jpg)
8x8 WS2812B LED panel
![Examplary panel](https://botland.com.pl/img/art/inne/06182_3.jpg)
