# Chip8Emu

A CHIP-8 emulator written in C with SDL2 for graphics and input.  
Sound is not implemented yet — the console will print `"sound is playing"` instead.

---

## Features
- Runs CHIP-8 ROMs at ~60 FPS.
- Configurable instructions-per-frame (IPF).
- Toggle common CHIP-8 quirks:
  - Sprite wrapping / clipping
  - FX55/FX65 behavior
  - 8XYE / 8XY6 behavior
- Simple console-based ROM loader.
- Step-through debugging support.

---

## Controls
- **Space** → Pause / Resume
- **N** → Step through 1 instruction (while paused)
- **1,2,3,4** → CHIP-8 keys `1,2,3,C`
- **Q,W,E,R** → CHIP-8 keys `4,5,6,D`
- **A,S,D,F** → CHIP-8 keys `7,8,9,E`
- **Z,X,C,V** → CHIP-8 keys `A,0,B,F`

(Uses the QWERTY layout for mapping.)

---

## Quirks
Use the on-screen checkboxes to toggle:
- **Sprite Wrap / Clipping**
- **FX55 / FX65 behavior**
- **8XYE / 8XY6 shifts**

---

## ROMs
- By default, the emulator loads `Play.ch8` from the same folder.  
- To play another ROM:
  1. Place the ROM file in the same folder.
  2. Launch the emulator (using `Play.ch8` as a placeholder).
  3. Click **LOAD ROM** and enter the ROM’s filename in the console.

---

## Instructions Per Frame (IPF)
- The **IPF Changer** button allows adjusting how many instructions are executed per frame.  
- After clicking, enter an integer in the console.  
- Defaults to running at ~60 FPS.

---

## Requirements
Make sure these files are in the same folder as the executable, or SDL2 and SDL2_ttf are installed on your system + arial.ttf is in the folder:
- `arial.ttf`
- `SDL2.dll`
- `SDL2_ttf.dll`
- `zlib1.dll`
- `freetype.dll`

---

## Building
Compile with GCC (MinGW on Windows):

```bash
gcc Chip8Emu.c -o Chip8Emu -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf

