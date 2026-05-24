#ifndef CHIP8_H
#define CHIP8_H

#include <cstdint>

class Chip8 {
public:
    Chip8();

    void loadROM(const char* filename);
    void emulateCycle();

    // Ecranul CHIP-8 (64x32 pixeli)
    uint8_t gfx[64 * 32];
    bool drawFlag;

    // Tastatura simplificata pentru terminal
    uint8_t keys[16];

    uint16_t I;
    uint16_t pc;

private:
    uint8_t memory[4096];
    uint8_t V[16];
    uint16_t stack[16];
    uint16_t sp;

    uint8_t delayTimer;
    uint8_t soundTimer;

    uint16_t opcode;
};

#endif
