LighTable
=========
A personal project inspired by a previous Arduino-based visualizer. I originally bought a DIY Ambient Backlight kit from Adafruit, only to find that its poor performance didn't make it worth the cost. I instead decided to repurpose it into something a bit more fun.

This version captures Windows audio output, processes the audio on the PC, and streams LED color data to the arduino for rendering.

Intended for use with the Adafruit "12mm Diffused Thin Digital RGB LED Pixels (Strand of 25)":
https://www.adafruit.com/products/322

Uses the Arduino program from the Adafruit DIY Ambient Monitor Backlight project:
https://learn.adafruit.com/adalight-diy-ambient-tv-lighting/download-and-install
...
https://github.com/adafruit/Adalight/blob/master/Arduino/LEDstream/LEDstream.pde


Binaries can be found in Release/Debug. There is currently no intent to support platforms other than Windows.



Currently uses very basic processing (only via signal amplitude and random color selection) to change lighting to the music. Future versions are intended to support much more dynamic behaviors, as is mentioned in the file 'LighTable/VisualizationStateController.h'.
