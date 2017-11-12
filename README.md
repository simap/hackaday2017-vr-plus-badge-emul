# hackaday2017-badge-emul
# Welcome to the Unofficial Hackaday 2017 Badge Emulator for all your hacking needs!

![Alt text](Hackaday2017BadgeEmul.png?raw=true "Badge Emulator Screenshot")

### Features:
- Framebuffer emulation of the display.
- Button press emulation (use number keys 1 - 5)
- Accelerometer emulation with mouse (hold shift and move the mouse to emulate X/Y movements).

Have an improvement/fix/awesomeness to add? Send us pull requests!

### Build and Run Requirements:

- SFML (https://www.sfml-dev.org/)
- brew

#### Recommended:
- CLion IDE

### Setting up:

Install SFML via brew on your Mac:

``` brew install sfml ```

TODO: Complete build / add project documentation.


### Wishlist of improvements:

- Add accelerometer support for Z axis (perhaps reading Z from mousewheel events).
- Test on Windows and Linux (SFML is portable to both platforms).
- Implement full support of printf()

### Killer features/ideas for future development:
- SFML supports webcam capturing, so we should be able to emulate the camera.
- Oscilliscope / analog input via the microphone.

