#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> //for unsignted ints
#include <SDL2/SDL.h> //for graphics and input 

void dumpBinaryToText(const char *inputFile, const char *outputFile) {
    FILE *in = fopen(inputFile, "rb");
    if (!in) {
        printf("Failed to open input file.\n");
        return;
    }
    FILE *out = fopen(outputFile, "w");
    if (!out) {
        printf("Failed to open output file.\n");
        fclose(in);
        return;
    }

    int byte, address = 0;
    while ((byte = fgetc(in)) != EOF) {
        fprintf(out, "%02X ", (unsigned char)byte);
        address++;
        if (address % 16 == 0) fprintf(out, "\n");
    }
    fprintf(out, "\n");
    fclose(in);
    fclose(out);
}
// https://github.com/mattmikolay/chip-8/wiki/CHIP%E2%80%908-Instruction-Set#notes, followed for Original behaviour, quirks are based on behaviour from other sources such as CowGod technical reference for chip8
int loadStoreRegQuirk = 0; //flag for register I should be incremented by X + 1 (if 0) or not incremented at all (if non 0)                                                       
int spriteWrapClipQuirk = 0; //flag for if partially drawn sprites wrap around (if 0) or get clipped (non 0), sprites drawn completely 
int bitShiftQuirk = 0; //flag for if 8XY6 and 8XYE should use register  VX and VY (0) or VX only (non 0)


uint8_t mainMemory[4096]; // Main 4096 bytes of memory, each instruction is 2 bytes (16 bits, 2 addresses)
uint16_t stack[16]; // Stack stores up to 16 return address 
uint8_t display[64][32]; // Display 64x32 pixels
uint8_t keys[16]; // Keypad with 16 keys (0x0 to 0xF)

void loadROM(char *nameROM){ //read binary file (chip8 rom, containing instructions) into memory
    FILE *rom = fopen(nameROM, "rb"); //read file in binary
    if (rom == NULL) { //make sure rom exists
        fprintf(stderr, "Failed to open ROM\n");
        exit(1);
}

    fseek(rom, 0, SEEK_END); //go to end of file
    long rom_size = ftell(rom); //get size of file
    rewind(rom); //put poin ter back to start of file

    if (rom_size > (4096 - 0x200)) { //make sure rom isn't bigger than memory
        fprintf(stderr, "ROM too large to fit in memory\n");
        fclose(rom);
        exit(1);
    }

    fread(&mainMemory[0x200], 1, rom_size, rom);  //start at address of memory array, read 1 byte at a time, rom size amount of times (each instruction is a byte so size in bytes is no. of instructions), from rom file
    fclose(rom);
    
}

void playSound(){
    printf("playing sound"); //will add real sound later
}

void handleKeyPress(SDL_Event *event) {
    int is_pressed = (event->type == SDL_KEYDOWN) ? 1 : 0; 
    switch(event->key.keysym.sym) {
        case SDLK_1: keys[0x1] = is_pressed; break;
        case SDLK_2: keys[0x2] = is_pressed; break;
        case SDLK_3: keys[0x3] = is_pressed; break;
        case SDLK_4: keys[0xC] = is_pressed; break;
        case SDLK_q: keys[0x4] = is_pressed; break;
        case SDLK_w: keys[0x5] = is_pressed; break;
        case SDLK_e: keys[0x6] = is_pressed; break;
        case SDLK_r: keys[0xD] = is_pressed; break;
        case SDLK_a: keys[0x7] = is_pressed; break;
        case SDLK_s: keys[0x8] = is_pressed; break;
        case SDLK_d: keys[0x9] = is_pressed; break;
        case SDLK_f: keys[0xE] = is_pressed; break;
        case SDLK_z: keys[0xA] = is_pressed; break;
        case SDLK_x: keys[0x0] = is_pressed; break;
        case SDLK_c: keys[0xB] = is_pressed; break;
        case SDLK_v: keys[0xF] = is_pressed; break;
    }
}
 
typedef struct {
    uint8_t V[16]; // 16 registers (V0 to VF (0-15), VF is flag register)
    uint16_t I; // Index register
    uint8_t DT; // Delay timer  
    uint8_t ST; // Sound timer
    uint16_t PC; // Program counter holds address of next instruction in memory unless modified by opcode
    uint8_t SP; // Stack pointer
} registers;

registers regs; // global registers

void initialiseSystem() { //set all default values and sprites
    memset(mainMemory, 0, sizeof(mainMemory));
    for (int i = 0; i < 16; i++) {
        regs.V[i] = 0; 
    }
    regs.I = 0; 
    regs.DT = 0; 
    regs.ST = 0; 
    regs.PC = 0x200; // Program counter starts at 0x200
    regs.SP = 0; 
    for (int i = 0; i < 16; i++) {
        stack[i] = 0; 
    }
    for (int y = 0; y < 32; y++) { //set all display pixels to off
        for (int x = 0; x < 64; x++) {
            display[x][y] = 0; 
        }
    }
    for (int i = 0; i < 16; i++) { //set all keys to not pressed
        keys[i] = 0; 
    }
  
    uint8_t defaultSprites[80] = { // 5x8 sprites for 0-9, A-F, starting at 0x00
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0, top row first, last is bottom row
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, //7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, //9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };
    for (int i = 0; i < 80; i++) {
        mainMemory[i] = defaultSprites[i]; // Put sprites into memory 0x00 to 0x4F
    }
}

void op_00E0() { // clear display array
    for (int i = 0; i < 64; i++) {
        for (int j = 0; j < 32; j++) {
            display[i][j] = 0;
        }
    }
}

void op_00EE() { // return from subroutine
    if (regs.SP > 0) {
        regs.PC = stack[--regs.SP]; //decrement SP by 1 and set PC
    } else {
        printf("Stack underflow\n");
    }
}

void op_0NNN(uint16_t NNN) {
    printf("Opcode not implemented\n"); //idk what this is?
}

void op_1NNN(uint16_t NNN) { // jump to address NNN
    regs.PC = NNN; // set PC to NNN
}

void op_2NNN(uint16_t NNN) { // call subroutine at NNN
    if (regs.SP < 15) {
        stack[regs.SP++] = regs.PC; // push current PC onto stack and increment SP
        regs.PC = NNN; // set PC to NNN
    } else {
        printf("Stack overflow\n");
    }
}

void op_3XNN(uint8_t X, uint8_t NN) { // skip next instruction (skip memory address, +2 PC, skip 16 bits) if register X equals NN
    if (regs.V[X] == NN) {
        regs.PC += 2; 
    }
}

void op_4XNN(uint8_t X, uint8_t NN) { // skip next instruction (skip memory address, +2 PC, skip 16 bits) if register X does not equal NN
    if (regs.V[X] != NN) {
        regs.PC += 2;
    }
}

void op_5XY0(uint8_t X, uint8_t Y) { // skip next instruction (skip memory address, +2 PC, skip 16 bits) if register X equals register Y
    if (regs.V[X] == regs.V[Y]) {
        regs.PC += 2;
    }
}

void op_6XNN(uint8_t X, uint8_t NN) { //set register X to NN
    regs.V[X] = NN;
}

void op_7XNN(uint8_t X, uint8_t NN) { //add NN to register X, C overflow matches Chip-8 spec for register overflow (Modulo 256 for values larger than 255)
    regs.V[X] += NN;
}

void op_8XY0(uint8_t X, uint8_t Y) { //set register X to register Y
    regs.V[X] = regs.V[Y];
}

void op_8XY1(uint8_t X, uint8_t Y) { //set register X to register X OR register Y
    regs.V[X] |= regs.V[Y];
}

void op_8XY2(uint8_t X, uint8_t Y) { //set register X to register X AND register Y
    regs.V[X] &= regs.V[Y];
}

void op_8XY3(uint8_t X, uint8_t Y) { //set register X to register X XOR register Y
    regs.V[X] ^= regs.V[Y];
}

void op_8XY4(uint8_t X, uint8_t Y) { //add register Y to register X, set VF to 1 if overflow, 0 if not, do flag first then register or leads to bugs
    uint16_t sum = regs.V[X] + regs.V[Y];
    if (sum > 255) {
        regs.V[0xF] = 1; //1 for overflow
    } else {
        regs.V[0xF] = 0; //0 for no overflow
    }
    regs.V[X] = regs.V[X] + regs.V[Y]; //C does wrap around if overflow occurs, could also mask sum with 0xFF or sum mod 256
}

void op_8XY5(uint8_t X, uint8_t Y) { //subtract register Y from register X, set VF to 1 if no borrow, 0 if borrow
    if (regs.V[X] > regs.V[Y]) {
        regs.V[0xF] = 1; //no borrow
    } else {
        regs.V[0xF] = 0; //borrow
    }
    regs.V[X] -= regs.V[Y];
}

void op_8XY6(uint8_t X, uint8_t Y) { //based on if bitShiftQuirk set to 0 or non 0
    if (bitShiftQuirk == 0) { //register X = register Y >> 1, set VF to least significant bit of register Y, bitShiftQuirk= 0
        regs.V[0xF] = regs.V[X] & 0x01; //set VF to least significant bit of VX
        regs.V[X] = regs.V[X] >> 1; //set VX to VX shifted right by 1
    }
    else { //register X = register X >> 1, set VF to least significant bit of register X, bitShiftQuirk = non 0
        regs.V[0xF] = regs.V[Y] & 0x01; //set VF to least significant bit of VY
        regs.V[X] = regs.V[Y] >> 1; //set VX to VY shifted right by 1
    }
}

void op_8XY7(uint8_t X, uint8_t Y) { //set register X to register Y - register X, set VF to 1 if no borrow, 0 if borrow
    if (regs.V[Y] > regs.V[X]) {
        regs.V[0xF] = 1; //no borrow
    } else {
        regs.V[0xF] = 0; //borrow
    }
    regs.V[X] = regs.V[Y] - regs.V[X];
}

void op_8XYE(uint8_t X, uint8_t Y) { //based on if bitShiftQuirk set to 0 or non 0
    if(bitShiftQuirk == 0){ //register X = register Y << 1, set VF to most significant bit of register Y, bitShiftQuirk - 0
    regs.V[0xF] = regs.V[Y] >> 7; //set VF to most significant bit of VY
    regs.V[X] = regs.V[Y] << 1; //set VX to VY shift left by 1
    }
    else{ //register X = register X << 1, set VF to most significant bit of register X, bitShiftQuirk = non 0
        regs.V[0xF] = regs.V[X] >> 7; //set VF to most significant bit of VX
    regs.V[X] = regs.V[X] << 1; //shift VX left by 1
    }
}

void op_9XY0(uint8_t X, uint8_t Y) { //skip next instruction (skip memory address, +2 PC, skip 16 bits) if register X does not equal register Y
    if (regs.V[X] != regs.V[Y]) {
        regs.PC += 2;
    }
}

void op_ANNN(uint16_t NNN) { //set index register I to NNN
    regs.I = NNN;
}

void op_BNNN(uint16_t NNN) { // jump to address NNN + V0
    regs.PC = NNN + regs.V[0];
}

void op_CXNN(uint8_t X, uint8_t NN) { //set register X to random number AND NN
    uint8_t randomByte = rand() % 256; // Generate a random byte (0-255)
    regs.V[X] = randomByte & NN; // AND with NN
}

void op_DXYN(uint8_t X, uint8_t Y, uint8_t N) { // draw sprite from address I at (V[X], V[Y] corresponding to top left most pixel) with height N(top level, top level - N) 
    regs.V[0xF] = 0; //set register VF to 0 initially when no collision
    for(int i = 0; i < N; i++){ //get each row of the sprite down to height top - N (sprites starts at lowest address)
        uint8_t spriteRow = mainMemory[regs.I + i];

        //first bit always on screen somewhere
        int firstX = regs.V[X] % 64; //wraparound X if needed for initial X coord
        int firstY = regs.V[Y] % 32; //wraparound Y if needed for initial Y coord
        uint8_t firstBit = (spriteRow >> 7);
        if (firstBit == 1 && display[firstX][firstY] == 1){ //check if value would cause collision ie both bits result in 0 when XOR so when both bits 1 before the XOR
                regs.V[0xF] = 1;
            }
        display[firstX][firstY] ^= firstBit;

        //non first bits either wrapped around or clipped depending on spriteWrapClipQuirk value
        for(int j = 1; j <= 7; j++ ){
            int currentX = firstX + j;
            int currentY = firstY + i;

            if(spriteWrapClipQuirk != 0 && (currentX > 63 | currentY > 31)){ //clip sprite if spriteWrapClipQuirk != 0
                continue; //do nothing since clipped
            }
            //if pixel doesn't clip or clip quirk = 0
            int currentX = currentX % 64; //wraparound if needed 
            int currentY = currentY % 32; //wraparound if needed
            uint8_t mask = 128 >> j; //mask to find each indiviual bit from byte row
            uint8_t bit = (spriteRow & mask) >> (7-j); //value of each bit at specific position in the byte (starting from leftmost bit)
            if (bit == 1 && display[currentX][currentY] == 1){ //check if value would cause collision ie both bits result in 0 when XOR so when both bits 1 before the XOR
                regs.V[0xF] = 1;
            }
            display[currentX][currentY] ^= bit; //current display bit XOR with new bit and update display with result
            // DOUBLE CHECK SPEC: VF collision flag set before pixel is XOR'ed, to see if that's correct order
        }

    }
} 
   
void op_EX9E(uint8_t X) { // skip next instruction (skip memory address, +2 PC, skip 16 bits) if key with value of register X is pressed
    if (keys[regs.V[X]] == 1) { //key pressed if value is 1
        regs.PC += 2;
    }
}

void op_EXA1(uint8_t X) { // skip next instruction (skip memory address, +2 PC, skip 16 bits) if key with value of register X is not pressed
    if (keys[regs.V[X]] == 0) { //key not pressed if value is 0
        regs.PC += 2;
    }
}

void op_FX07(uint8_t X) { // set register X to value of delay timer
    regs.V[X] = regs.DT;
}

void op_FX0A(uint8_t X) { // wait for key press and set register X to value of pressed key
    int keyPressed = 0;
    for (int i = 0; i < 16; i++) {
        if (keys[i] == 1) {
            regs.V[X] = i; // set register X to pressed key
            keyPressed = 1;
            break; //inconsistent if multiple keys pressed, not sure how to handle this?
        }
    }
    if (keyPressed == 0) {
        regs.PC -= 2; // if no key pressed, decrement PC so instruction repeats
    }
}

void op_FX15(uint8_t X) { // set delay timer to value of register X
    regs.DT = regs.V[X];
}

void op_FX18(uint8_t X) { // set sound timer to value of register X
    regs.ST = regs.V[X];
}

void op_FX1E(uint8_t X) { // add value of register X to index register I
    regs.I += regs.V[X]; //value x mod 4096 since 16bit register
}      

void op_FX29(uint8_t X) { // set index register I to location of sprite for digit in register X
    if (regs.V[X] < 16) { // ensure X is a valid digit (0-15)
        regs.I = regs.V[X] * 5; // sprites are 5 bytes from 0x00 to 0x4F
    } else {
        printf("Invalid digit in register V[%d]: %d\n", X, regs.V[X]);
    }
}

void op_FX33(uint8_t X) { // store BCD representation of value in register X at I, I+1, I+2
    uint8_t value = regs.V[X];
    mainMemory[regs.I] = value / 100; // integer division for 100 digit
    mainMemory[regs.I + 2] = value % 10; // one digit
    mainMemory[regs.I + 1] = (value - (mainMemory[regs.I] * 100) - mainMemory[regs.I + 2]) / 10; // tens digit
}

void op_FX55(uint8_t X) { // store registers V0 to VX inclusive in memory starting at address I
    for (int i = 0; i <= X; i++) {
        mainMemory[regs.I + i] = regs.V[i];
    }
    if(loadStoreRegQuirk == 0){
        regs.I += X + 1; // increment I by X + 1 after
    } //if loadStoreRegQuirk == 0 then set reg I to I+X+1 after, otherwise reg I stays the same after the instruction if loadStoreRegQuirk == non 0
}

void op_FX65(uint8_t X) { // read registers V0 to VX inclusive from memory starting at address I
    for (int i = 0; i <= X; i++) {
        regs.V[i] = mainMemory[regs.I + i];
    }
    if(loadStoreRegQuirk == 0){
        regs.I += X + 1; // increment I by X + 1 after
    } //if loadStoreRegQuirk == 0 then set reg I to I+X+1 after, otherwise reg I stays the same after the instruction if loadStoreRegQuirk == non 0

}

void decode(uint16_t opcode) {
    uint8_t firstFourBits = opcode >> 12;
    uint16_t NNN = opcode & 0x0FFF; // Extract NNN
    uint8_t NN = opcode & 0x00FF; // Extract NN
    uint8_t N = opcode & 0x000F; // Extract N
    uint8_t X = (opcode & 0x0F00) >> 8; // Extract X
    uint8_t Y = (opcode & 0x00F0) >> 4; // Extract Y
    
    switch (firstFourBits) {
        case 0x0: {
            if (opcode == 0x00E0) {
                op_00E0();
            } else if (opcode == 0x00EE) {
                op_00EE();
            } else {
                op_0NNN(NNN);
            }
            break;
        }
        case 0x1: {
            op_1NNN(NNN);
            break;
        }
        case 0x2: {
            op_2NNN(NNN);
            break;
        }
        case 0x3: {
            op_3XNN(X,NN);
            break;
        }
        case 0x4: {
            op_4XNN(X,NN);
            break;
        }
        case 0x5: {
            op_5XY0(X,Y);
            break;
        }
        case 0x6: {
            op_6XNN(X, NN);
            break;
        }
        case 0x7: {
            op_7XNN(X, NN);
            break;
        }
        case 0x8: {
            if (N == 1) {
                op_8XY1(X, Y);
            } else if (N == 2) {
                op_8XY2(X, Y);
            } else if (N == 3) {
                op_8XY3(X, Y);
            } else if (N == 4) {
                op_8XY4(X, Y);
            } else if (N == 5) {
                op_8XY5(X, Y);
            } else if (N == 6) { //bitShiftQuirk handled inside actual instruction
                op_8XY6(X,Y);
            } else if (N == 7) {
                op_8XY7(X, Y);
            } else if (N == 0xE) { //bitShiftQuirk handled inside actual instruction
                op_8XYEF(X, Y);
            } else {
                printf("Unknown opcode: 0x%04X\n", opcode);
            }
            break;
        }
        case 0x9: {
            op_9XY0(X, Y);
            break;
        }
        case 0xA: {
            op_ANNN(NNN);
            break;
        }   
        case 0xB: {
            op_BNNN(NNN);
            break;
        }
        case 0xC: {
            op_CXNN(X, NN);
            break;
        }
        case 0xD: {
            op_DXYN(X, Y, N); // clip or wrap Quirk handled in instruction
            break;
        }
        case 0xE: {
            if (NN == 0x9E) {
                op_EX9E(X);
            } else if (NN == 0xA1) {
                op_EXA1(X);
            } else {
                printf("Unknown opcode: 0x%04X\n", opcode);
            }
            break;
        }
        case 0xF: {
            if (NN == 0x07) {
                op_FX07(X);
            } else if (NN == 0x0A) {
                op_FX0A(X);
            } else if (NN == 0x15) {
                op_FX15(X);
            } else if (NN == 0x18) {
                op_FX18(X);
            } else if (NN == 0x1E) {
                op_FX1E(X);
            } else if (NN == 0x29) {
                op_FX29(X);
            } else if (NN == 0x33) {
                op_FX33(X);
            } else if (NN == 0x55) { //loadStoreRegQuirk handled inside instruction
                op_FX55(X);
            } else if (NN == 0x65) { //loadStoreRegQuirk handled inside instruction
                op_FX65(X);
            } else {
                printf("Unknown opcode: 0x%04X\n", opcode);
            }
            break;
        }
        default: {
            printf("Unknown opcode: 0x%04X\n", opcode);
            break;
        }
    }
}

void fetchDecodeExecute(){
    uint16_t instruction = (mainMemory[regs.PC] << 8) | mainMemory[regs.PC + 1]; //fetch each 8 bit half of instruction and concatanate, big Endian for instructions
    regs.PC += 2; //set PC to next sequential instruction, executed instruction can change this potentially
    decode(instruction); //decode into opcode and operand and execute
}

void drawDisplay(SDL_Renderer *renderer){
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); //black pixels
    SDL_RenderClear(renderer); //turn screen to black

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); //white pixels
    for (int y = 0; y < 32; y++) { //width 32 pixels, loop through display array and set white pixels when == 1
        for (int x = 0; x < 64; x++) { //height 64 pixels
            if (display[x][y] == 1) { //if display array set to 1 at that position, draw a white square (pixel) there
                SDL_Rect rect = { x * 10, y * 10, 10, 10 }; //pixels are 10x10 window pixels squares
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }
    SDL_RenderPresent(renderer); //update window with everything drawn since renderclear (white pixel)
}


int main(int argc, char *argv[]){ //for SDL
    dumpBinaryToText("Play.ch8", "Play_dump.txt"); //view game binary
    initialiseSystem(); //initalise memory/registers etc
    loadROM("Play.ch8"); //loads rom Play.ch8 into memory

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("CHIP-8", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 320, 0); //window width and height 640x320, 10x10 window to chip8 pixel
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED); //default driver gpu accelerated if possible

    uint32_t last_time = SDL_GetTicks(); //initalise time since SDL library initialised
    uint32_t timer_accumulator = 0; //initialise timer to track real time between loops

    SDL_Event event; //stores input/quit events
    int open = 1;
    while (open) {
        while (SDL_PollEvent(&event)) { //Check if user quit or not
            if (event.type == SDL_QUIT) open = 0; //SDL_QUIT is close window button
            handleKeyPress(&event); // check user input and update keys array accordingly 
        }

        for (int i = 0; i < 10; i++) { //how many chip8 instructions happen per frame, 10 in this case
            fetchDecodeExecute();  
        }

        uint32_t now = SDL_GetTicks(); //current time
        timer_accumulator += now - last_time; //amount of time since last time, keeps adding on to know how many times to decrement timer e.g. 35 ms elapsed since last loop then 35 > 16.67 * 2 so decrement timers twice
        last_time = now; //update last time for next loop
        while (timer_accumulator >= 1000 / 60) { //1000/60 = 16 ms (60fps roughly)  once 16 or greater elapsed since last frame then decrement timers
            if (regs.DT > 0) regs.DT--; //decrement delay timer by 1
            if (regs.ST > 0){ 
                regs.ST--; // decrement sound timer by 1
                playSound(); //not actually implemented yet
            }
            timer_accumulator -= 1000 / 60; //remove 16.67ms from accumulator since timers have been decremented for that time period
        }

        // 4. Render display
        drawDisplay(renderer);

        SDL_Delay(1000/60); //60 fps

    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
