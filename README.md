# hackaday2017-badge-emul
# Welcome to the Unofficial Hackaday 2017 Badge Emulator for all your hacking needs!

![Alt text](Hackaday2017BadgeEmul.png?raw=true "Badge Emulator Screenshot")

### Features:
- Framebuffer emulation of the display.
- Buttons
    - click on the button with mouse.
    - shortcut keys (use number keys 1 - 5)
- Accelerometer emulation with mouse (hold shift and move the mouse to emulate X/Y movements).

Have an improvement/fix/awesomeness to add? Send us pull requests!

### Build and Run Requirements:

- brew
- cmake
- SFML (https://www.sfml-dev.org/)

#### Recommended:
- CLion IDE

### Building and running:

```

brew install cmake
brew install sfml

cmake .
make HackyBadge17

open HackyBadge17.app

```


### From CLion:

Simple create project from sources, point to the directory, build and run.


### Adding your application to the emulator

- Edit CMakeLists.txt
- At the top change breakout to the name of your app. I.e. if the source name is "yourapp.c", set to "yourapp"

Example:

```
#
# Set the name of the app that you are emulating here
#
set(EMULATION_APP "breakout")

```

### Wishlist of improvements:

- Add accelerometer support for Z axis (perhaps reading Z from mousewheel events).
- Test on Windows and Linux (SFML is portable to both platforms).
- Implement full support of printf()
- There is an issue with conflicting Makefile (cmake generates one and overwrites the PIC32 one). Ideally we'd want
  a different Makefile name for pic32 or be able to build everything from a single CMakeLists.txt

### Killer features/ideas for future development:
- SFML supports webcam capturing, so we should be able to emulate the camera.
- Oscilliscope / analog input via the microphone.

