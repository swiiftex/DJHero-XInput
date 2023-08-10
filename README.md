This is a modification of the DJHero-Lucio project.

## Dependencies
The DJ Hero Lucio program uses the [NintendoExtensionCtrl library](https://github.com/dmadison/NintendoExtensionCtrl/) ([0.7.4](https://github.com/dmadison/NintendoExtensionCtrl/releases/tag/v0.7.4)) to handle communication to the DJ Hero controller itself.
This program was compiled using version [2.1.1] of the [Arduino IDE](https://www.arduino.cc/en/Main/Software).

I've only tested this with the Arduino Pro Micro (Leonardo works the same).
It should work with all the same boards as the original DJ Hero Lucio project. It will NOT throw an error if it doesn't recognise the board, instead it defaults back to the Pro Micro (this is because XInput recognition can be fiddly).

This was made to work with the Dolphin emulator. The profile is included (djhero.ini) and you can install it in:
C:\Users\%USER%\Documents\Dolphin Emulator\Config\Profiles\Wiimote
