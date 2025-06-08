#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> //for unsignted ints
#include <SDL2/SDL.h> //for graphics and input 


uint8_t mainMemory[4096]; // Main 4096 bytes of memory, each instruction is 2 bytes (16 bits, 2 addresses)
uint16_t stack[16]; // Stack stores up to 16 return address 
uint8_t display[64][32]; // Display 64x32 pixels
uint8_t keys[16]; // Keypad with 16 keys (0x0 to 0xF)

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
        0xF1, 0x90, 0xF0, 0x10, 0xF0, //9
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

void op_8XY6(uint8_t X, uint8_t Y) { //register X = register Y >> 1, set VF to least significant bit of register Y
    regs.V[0xF] = regs.V[Y] & 0x01; //set VF to least significant bit
    regs.V[X] = regs.V[Y] >> 1; //shift right by 1
}

void op_8XY7(uint8_t X, uint8_t Y) { //set register X to register Y - register X, set VF to 1 if no borrow, 0 if borrow
    if (regs.V[Y] > regs.V[X]) {
        regs.V[0xF] = 1; //no borrow
    } else {
        regs.V[0xF] = 0; //borrow
    }
    regs.V[X] = regs.V[Y] - regs.V[X];
}

void op_8XYE(uint8_t X, uint8_t Y) { //register X = register Y << 1, set VF to most significant bit of register Y
    regs.V[0xF] = regs.V[Y] >> 7; //set VF to most significant bit
    regs.V[X] = regs.V[Y] << 1; //shift left by 1
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
        uint8_t posX = regs.V[X];
        uint8_t posY = regs.V[Y];
        for(int j = 0; j <= 7; j++ ){
            uint8_t mask = 128 >> j; //mask to find each indiviual bit from byte row
            uint8_t bit = (spriteRow & mask) >> (7-j); //value of each bit at specific position in the byte (starting from leftmost bit)
            int newX = (posX + j) % 64; //wraparound X
            int newY = (posY + i) % 32; //wraparound Y
            if (bit == 1 && display[newX][newY] == 1){ //check if value would cause collision ie both bits result in 0 when XOR so when both bits 1 before the XOR
                regs.V[0xF] = 1;
            }
            display[newX][newY] ^= bit; // DOUBLE CHECK SPEC: VF collision flag set before pixel is XOR'ed, to see if that's correct order
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
    regs.I += X + 1; // increment I by X + 1 after
}

void op_FX65(uint8_t X) { // read registers V0 to VX inclusive from memory starting at address I
    for (int i = 0; i <= X; i++) {
        regs.V[i] = mainMemory[regs.I + i];
    }
    regs.I += X + 1; // increment I by X + 1 after
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
            } else if (N == 6) {
                op_8XY6(X,Y);
            } else if (N == 7) {
                op_8XY7(X, Y);
            } else if (N == 0xE) {
                op_8XYE(X, Y);
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
            op_DXYN(X, Y, N);
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
            } else if (NN == 0x55) {
                op_FX55(X);
            } else if (NN == 0x65) {
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
