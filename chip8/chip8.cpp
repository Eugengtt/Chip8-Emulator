#include "chip8.h"
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <iostream>

static const uint8_t chip8_fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

Chip8::Chip8() {
    pc = 0x200;
    I = 0;
    sp = 0;

    memset(memory, 0, sizeof(memory));
    memset(V, 0, sizeof(V));
    memset(gfx, 0, sizeof(gfx));
    memset(keys, 0, sizeof(keys));
    memset(stack, 0, sizeof(stack));

    delayTimer = 0;
    soundTimer = 0;

    for (int i = 0; i < 80; i++)
        memory[i] = chip8_fontset[i];

    drawFlag = false;
}

void Chip8::loadROM(const char* filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        std::cout << "[EROARE] Nu pot deschide ROM-ul! Verifica daca fisierul se numeste rom.ch8\n";
        return;
    }

    std::streampos size = file.tellg();
    char* buffer = new char[size];

    file.seekg(0, std::ios::beg);
    file.read(buffer, size);
    file.close();

    for (int i = 0; i < size; i++)
        memory[0x200 + i] = buffer[i];

    delete[] buffer;
    std::cout << "[SUCCES] ROM incarcat cu succes! Dimensiune: " << size << " bytes.\n";
}

void Chip8::emulateCycle() {
    opcode = memory[pc] << 8 | memory[pc + 1];
    bool pcModified = false;

    // Actualizam timerele la inceputul fiecarui ciclu (Folosim un contor bazat pe ciclurile reale)
    static int timerClock = 0;
    timerClock++;
    if (timerClock >= 8) {
        if (delayTimer > 0) delayTimer--;
        if (soundTimer > 0) soundTimer--;
        timerClock = 0;
    }

    switch (opcode & 0xF000) {

        case 0x0000:
            if (opcode == 0x00E0) {
                memset(gfx, 0, sizeof(gfx));
                drawFlag = true;
            }
            else if (opcode == 0x00EE) {
                sp--;
                pc = stack[sp];
                pcModified = true;
            }
            break;

        case 0x1000:
            pc = opcode & 0x0FFF;
            pcModified = true;
            break;

        case 0x2000:
            stack[sp] = pc + 2;
            sp++;
            pc = opcode & 0x0FFF;
            pcModified = true;
            break;

        case 0x3000: {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t nn = opcode & 0x00FF;
            if (V[x] == nn) pc += 2;
            break;
        }

        case 0x4000: {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t nn = opcode & 0x00FF;
            if (V[x] != nn) pc += 2;
            break;
        }

        case 0x5000: {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode & 0x00F0) >> 4;
            if (V[x] == V[y]) pc += 2;
            break;
        }

        case 0x6000: {
            uint8_t x = (opcode & 0x0F00) >> 8;
            V[x] = opcode & 0x00FF;
            break;
        }

        case 0x7000: {
            uint8_t x = (opcode & 0x0F00) >> 8;
            V[x] += opcode & 0x00FF;
            break;
        }

        case 0x8000: {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode & 0x00F0) >> 4;

            switch (opcode & 0x000F) {
                case 0x0: V[x] = V[y]; break;
                case 0x1: V[x] |= V[y]; break;
                case 0x2: V[x] &= V[y]; break;
                case 0x3: V[x] ^= V[y]; break;

                case 0x4: {
                    uint16_t sum = V[x] + V[y];
                    V[0xF] = sum > 255;
                    V[x] = sum & 0xFF;
                    break;
                }

                case 0x5:
                    V[0xF] = V[x] > V[y];
                    V[x] -= V[y];
                    break;

                case 0x6:
                    V[x] = V[y];
                    V[0xF] = V[x] & 1;
                    V[x] >>= 1;
                    break;

                case 0xE:
                    V[x] = V[y];
                    V[0xF] = (V[x] >> 7) & 1;
                    V[x] <<= 1;
                    break;
            }
            break;
        }

        case 0x9000: {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode & 0x00F0) >> 4;
            if (V[x] != V[y]) pc += 2;
            break;
        }

        case 0xA000:
            I = opcode & 0x0FFF;
            break;

        case 0xB000:
            pc = (opcode & 0x0FFF) + V[0];
            pcModified = true;
            break;

        case 0xC000: {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t nn = opcode & 0x00FF;
            V[x] = (rand() % 256) & nn;
            break;
        }

        case 0xD000: {
            uint8_t x = V[(opcode & 0x0F00) >> 8];
            uint8_t y = V[(opcode & 0x00F0) >> 4];
            uint8_t height = opcode & 0x000F;

            V[0xF] = 0;

            for (int row = 0; row < height; row++) {
                uint8_t pixel = memory[I + row];

                for (int col = 0; col < 8; col++) {
                    if (pixel & (0x80 >> col)) {
                        int currentX = (x + col) % 64;
                        int currentY = (y + row) % 32;
                        int index = currentX + (currentY * 64);

                        if (gfx[index] == 1)
                            V[0xF] = 1;

                        gfx[index] ^= 1;
                    }
                }
            }

            drawFlag = true;
            break;
        }

        case 0xE000: {
            uint8_t x = (opcode & 0x0F00) >> 8;

            switch (opcode & 0x00FF) {
                case 0x9E:
                    if (keys[V[x]]) pc += 2;
                    break;

                case 0xA1:
                    if (!keys[V[x]]) pc += 2;
                    break;
            }
            break;
        }

        case 0xF000: {
            uint8_t x = (opcode & 0x0F00) >> 8;

            switch (opcode & 0x00FF) {

                case 0x07:
                    V[x] = delayTimer;
                    break;

                case 0x0A: {
                    bool keyPressed = false;
                    for (int i = 0; i < 16; i++) {
                        if (keys[i]) {
                            V[x] = i;
                            keyPressed = true;
                            break;
                        }
                    }
                    if (!keyPressed) {
                        pcModified = true;
                    }
                    break;
                }

                case 0x15:
                    delayTimer = V[x];
                    break;

                case 0x18:
                    soundTimer = V[x];
                    break;

                case 0x1E:
                    I += V[x];
                    break;

                case 0x29:
                    I = V[x] * 5;
                    break;

                case 0x33:
                    memory[I]     = V[x] / 100;
                    memory[I + 1] = (V[x] / 10) % 10;
                    memory[I + 2] = V[x] % 10;
                    break;

                case 0x55:
                    for (int i = 0; i <= x; i++)
                        memory[I + i] = V[i];
                    break;

                case 0x65:
                    for (int i = 0; i <= x; i++)
                        V[i] = memory[I + i];
                    break;
            }
            break;
        }
    }

    if (!pcModified) {
        pc += 2;
    }
}
