# WS2812 LED panel driver

The aim of this project is to create Linux Kernel driver for WS2812 LED panel.
Main objectives are to:
* create an external module loadable to the Kernel on startup,
* create app which will utilize said driver,
* create sample animations with at least one with panning,
* utilize Buildroot tools,
* utilize framebuffer.


Utilized hardware:
* STM32MP157C-DK2 Development board
![STM32MP157C-DK2](https://www.st.com/bin/ecommerce/api/image.PF267415.en.feature-description-include-personalized-no-cpn-large.jpg)
* 8x8 WS2812B LED panel
![Examplary panel](https://botland.com.pl/img/art/inne/06182_3.jpg)

## How LED are driven

WS2812 LEDs are driven by a single data line in which 0 and 1 is represented
via different pulse width. To achieve that we utilized SPI MOSI output with 16 bit
word size. By sending a word with different amount of ones and zeros we were able
to get timings according to specification and therefore ensure the proper operation
of the panel.

Word length of 16 bits helped to get timings under controll, because it enabled
tuning timings with a good precision. Basically SPI was used as an PWM generator
with 4 bit resolution.

