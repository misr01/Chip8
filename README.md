Sound is not implemented except spamming the console with "sound is playing"

Controls: Space to pause, n to step through 1 instruction while paused
1,2,3,4 - 1,2,3,C
q,w,e,r - 4,5,6,D
a,s,d,f - 7,8,9,E
z,x,c,v - A,0,B,F
in qwerty keyboard layout

Quirks: click the checkboxes for spriteWrap/Clipping quirk, FX55/65 quirk and 8XYE and 8XY6 quirks

Load ROM: Defaults to loading Play.ch8 in the same folder, make sure to have a ROM renamed Play.ch8 to start. 
To play other ROMs not named Play.ch8, once opened using Play.ch8 click the LOAD ROM button and enter the name of the ROM file (make sure it's in the same folder) in the console.

IPF changer button, waits for integer to be entered in console to change the amount of instructions per frame (roughly 60fps).

Make sure arial.ttf is in the same folder as the executable.

Compile: $ gcc Chip8Emu.c -o Chip8Emu -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf
