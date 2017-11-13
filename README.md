# supercon2017_vr + hackaday2017-badge-emul
**This project is a fork of the amazing badge emulator with my VR Badge Hack code. See below for instructions on building and compiling.**


# 2017 Hackaday Supercon VR Badge Hack

This was my entry to the badge hack contest.

It is a wolfenstein/doom style raycaster with trippy procedural textures and a cycling color palette. The map is a maze, and there is a hidden treasure in the room with an X.

I didn't write any of this before badge pickup on Friday for the early badge hacking party, but I have to give huge credit to [Lode Vandevenne's website and code examples on raycasting](http://lodev.org/cgtutor/).

![No spoilers](images/demo.gif "No spoilers")

## Trying It out for Yourself

There is a for of the [awesome badge emulator](https://github.com/graphiq/hackaday2017-badge-emul) that can runs the maze with minimal hackery. 

You might still find one (or two!) of these amazing badges on [Tindie](https://www.tindie.com/products/hackadaystore/2017-hackaday-superconference-badge)

## Inspiration

I knew ahead of time that the badge had a 128x128 screen and a camera, so I had planned to do some kind of AR thing. I brought along one of those phone VR headset things so I could hack the optics for some kind of headset.

Then Supercon announced that they had made too many badges and were selling on Tindie. I knew then that I must do some kind of stereo vision thing and got a second one.

## 3D in 3Days

I recently learned of the raycasting technique that is very characteristic of the Wolfenstien and Doom games. I knew this 3D method would be very efficient since this was used for old DOS games on 286/386 PCs, and I'd need something fast if I wanted to render 3D on the badge. I started googling around for raycasting code that I could adapt. 

I found [Lode Vandevenne's website and code examples on raycasting](http://lodev.org/cgtutor/). These are really great, and he does an awesome job explaining how raycasting works. I was able to port his textured example over to ANSI C which the badge hacking environment wanted. I removed all of the SDL stuff, adapted memory to overlay with cambuffer (all heap memory was taken by other badge code), replace all of the c++ stuff, and wrote my own procedural textures.

It still uses floating point types and seems to perform well enough that I didn't bother porting it to fixed-point math, though I imagine that would speed things up considerably as the PIC32MX170F256D used in the badge doesn't have an FPU.


## But Stereo Vision, 2 Badges?

To generate a stereo image, I support 3 modes: solo, right side, and left side. 

Solo mode works without another badge. 

When in Right side mode the badge calculates new world and orientation position as normal, then sends this data over UART2 @ 100kbps before rending the next frame. I only needed to send 6 32-bit doubles to keep worlds in sync. I added a start and end byte to create a frame that could aid in synchronizing serial data streams and preventing accidental garbage. Later I added another variable to color cycle the palette.

When in left side mode, the badge reads this data from UART2 instead of reading accelerometer inputs. It then sidesteps the camera over slightly (as long as its not inside a wall) to produce an image offset for the left eye. This gives it a stereoscopic true 3D feel. It then sends an ack byte so that both sides are synchronized and the frame is rendered at more or less the same time.

When rendering the left side, not only is the camera offset, but the rendered image must be rotated 180° so that the badge can be placed close enough to the other screen to line up with the optics.

## VR Headset

I didn't bring a 3d printer, but I did bring a handful of leftover filament and a hot air rework station. This let me melt and goop PLA into the (presumably) ABS headset to create badge support structures to keep the 2 badges well aligned. Even being off a few degrees can really mess with the effect. One badge can slide a bit to adjust for pupillary distance.

The badges needed to overlap slightly to get close enough, so I swapped one battery to the other side of the PCB. The whole thing was still too large for the lid to close, so I made a quick and dirty wire post tie to keep the whole thing together.

## Every Maze Must Have Its Treasure

I added a high resolution texture to the treasure room, and needed to tweak the engine's math to allow textures >= the screen size. This 128x128 texture is a bitmap stored in flash. The other textures are 32x32 and draw from RAM.

## Crazy Maze

With the trippy textures and color cycling palette, the maze is quite disorienting and makes finding your way around the maze that much more challenging.

To aid in finding the treasure room, I added a map overlay with player position.

<hr>


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

### Controls:

- Accelerometer: Hold **Left-Shift** and **move mouse** around to simulate X/Y accelerometer axis.
- Buttons: Number **Keys 1-5** send button events, or click the mouse on the buttons.

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

