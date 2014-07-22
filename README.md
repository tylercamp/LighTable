LighTable
=========
A personal project inspired by a previous Arduino-based visualizer. I originally bought a DIY Ambient Backlight kit from Adafruit, only to find that its poor performance didn't make it worth the cost. I instead decided to repurpose it into something a bit more fun.

This version captures Windows audio output, processes the audio on the PC, and streams LED color data to the arduino for rendering.

Intended for use with the Adafruit "12mm Diffused Thin Digital RGB LED Pixels (Strand of 25)":
https://www.adafruit.com/products/322

Uses a modified version of the Arduino program from the Adafruit DIY Ambient Monitor Backlight project:
https://github.com/adafruit/Adalight/blob/master/Arduino/LEDstream/LEDstream.pde


Binaries can be downloaded by downloading the "Executable.zip" file.



Currently uses very basic processing (only via signal amplitude) to change lighting to the music. There are only two modes for color selection:

- Mono-Color Random: A random RGB color is selected and the LEDs move towards that color based on the "pace" of the music. All LEDs are the same color. Brightness is determined by overall loudness of the music.
- Hue-Cycle Diverse: HSV colors are employed, LEDs maintain constant saturation and cycle through hues based on the "pace" of the music. The hue of each LED is relatively varied from one to the next. Brightness is determined by overall loudness of the music.

Future versions are intended to support much more dynamic behaviors, as is mentioned in the file 'LighTable/VisualizationStateController.h'.
