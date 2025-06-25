![img](/data/logo.svg)

# tystnad

Simple program that prevents annoying buzz sounds from your receiver on macOS (and Linux?)

## Purpose

When you hook up an AV receiver or some amplifiers to a Mac with macOS, or simply Linux, you will hear annoying buzzing sounds in the background when the audio subsystem is disabled.
This program aims to fix this annoyance, by playing an empty audio file continuisely. If you do not hear annoying sounds when no audio is playing, you do not need this program.

## Installation (macOS, Apple Silicon and Intel)

Download a prebuilt binary from the 'Releases' tab.

## Installation (Linux)

If your distribution supports AppImages, you can download a prebuilt AppImage from the 'Releases' tab. 
If it doesn't, or you have a different architecture, you must build it yourself.

## Compile

If you want to compile the project, first install the dependencies:

- `brew install qt cmake gcc`

Then you can build it:

- `mkdir -p build/`
- `cd build/`
- `cmake .. -DCMAKE_BUILD_TYPE=Release`
- `cmake --build .`

Finally, to install it:

- macOS: Copy `build/tystnad.app` to `/Applications/`
- Linux: Copy `build/tystnad` to `/usr/bin` (or `/usr/local/bin`)

Note: If you don't specify your build type as Release, a .app will not be created. This is only relevant for macOS.

## License

This project is licensed under the MIT license.

Copyright (c) 2025 Jacob Nilsson
