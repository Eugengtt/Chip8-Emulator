#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <vector>
#include "chip8.h"

#ifdef _WIN32
#include <windows.h>

void resetCursor() {
    COORD cursorPosition = {0, 0};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), cursorPosition);
}

void setupTerminal() {
    SetConsoleOutputCP(CP_UTF8);

    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(out, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(out, &cursorInfo);

    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hIn, &mode);
    SetConsoleMode(hIn, mode & ~ENABLE_ECHO_INPUT & ~ENABLE_LINE_INPUT);

    std::system("cls");
}

void updateKeyboard(Chip8& chip) {
    chip.keys[0x1] = (GetAsyncKeyState('1') & 0x8000) != 0;
    chip.keys[0x2] = (GetAsyncKeyState('2') & 0x8000) != 0;
    chip.keys[0x3] = (GetAsyncKeyState('3') & 0x8000) != 0;
    chip.keys[0xC] = (GetAsyncKeyState('4') & 0x8000) != 0;

    chip.keys[0x4] = (GetAsyncKeyState('Q') & 0x8000) != 0;
    chip.keys[0x5] = (GetAsyncKeyState('W') & 0x8000) != 0;
    chip.keys[0x6] = (GetAsyncKeyState('E') & 0x8000) != 0;
    chip.keys[0xD] = (GetAsyncKeyState('R') & 0x8000) != 0;

    chip.keys[0x7] = (GetAsyncKeyState('A') & 0x8000) != 0;
    chip.keys[0x8] = (GetAsyncKeyState('S') & 0x8000) != 0;
    chip.keys[0x9] = (GetAsyncKeyState('D') & 0x8000) != 0;
    chip.keys[0xE] = (GetAsyncKeyState('F') & 0x8000) != 0;

    chip.keys[0xA] = (GetAsyncKeyState('Z') & 0x8000) != 0;
    chip.keys[0x0] = (GetAsyncKeyState('X') & 0x8000) != 0;
    chip.keys[0xB] = (GetAsyncKeyState('C') & 0x8000) != 0;
    chip.keys[0xF] = (GetAsyncKeyState('V') & 0x8000) != 0;
}
#else
void resetCursor() { std::cout << "\033[H"; }
void setupTerminal() { std::cout << "\033[?25l"; std::system("clear"); }
void updateKeyboard(Chip8& chip) {}
#endif

int main() {
    // Ne asiguram ca ecranul e curat pentru meniu
    SetConsoleOutputCP(CP_UTF8);
    std::system("cls");

    std::vector<std::string> jocuri;

#ifdef _WIN32
    // Scanare folder nativa pe Windows dupa *.ch8
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA("./*.ch8", &findData);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            // Adaugam numele fisierului gasit in lista
            jocuri.push_back(findData.cFileName);
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }
#endif

    if (jocuri.empty()) {
        std::cout << "[EROARE] Nu am gasit niciun fisier .ch8 in folderul curent!\n";
        std::cout << "Asigura-te ca ai pus jocurile (.ch8) exact in folderul unde e codul tau.\n\n";
        std::system("pause");
        return 1;
    }

    // Afisare meniu jocuri
    std::cout << "=================================\n";
    std::cout << "    SELECTEAZA UN JOC CHIP-8     \n";
    std::cout << "=================================\n\n";
    for (size_t i = 0; i < jocuri.size(); i++) {
        std::cout << " [" << i + 1 << "] -> " << jocuri[i] << "\n";
    }
    std::cout << "\nIntrodu numarul jocului ales: ";

    size_t alegere = 0;
    std::cin >> alegere;

    if (alegere < 1 || alegere > jocuri.size()) {
        std::cout << "Numar invalid!\n";
        std::system("pause");
        return 1;
    }

    std::string jocSelectat = jocuri[alegere - 1];

    // Pornim emulatorul cu jocul ales
    setupTerminal();

    Chip8 chip;
    chip.loadROM(jocSelectat.c_str());

    const int FPS = 60;
    const auto frameDelay = std::chrono::milliseconds(1000 / FPS);

    while (true) {
        auto frameStart = std::chrono::steady_clock::now();

        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            break;
        }

        if (GetAsyncKeyState('P') & 0x8000) {
            chip = Chip8();
            chip.loadROM(jocSelectat.c_str());
            std::system("cls");
        }

        updateKeyboard(chip);

        for (int i = 0; i < 8; i++) {
            chip.emulateCycle();
        }

        if (chip.drawFlag) {
            chip.drawFlag = false;
            resetCursor();

            std::string outputBuffer = "";
            for (int y = 0; y < 32; y += 2) {
                for (int x = 0; x < 64; x++) {
                    bool topPixel = chip.gfx[y * 64 + x];
                    bool bottomPixel = chip.gfx[(y + 1) * 64 + x];

                    if (topPixel && bottomPixel) {
                        outputBuffer += "█";
                    } else if (topPixel && !bottomPixel) {
                        outputBuffer += "▀";
                    } else if (!topPixel && bottomPixel) {
                        outputBuffer += "▄";
                    } else {
                        outputBuffer += " ";
                    }
                }
                outputBuffer += "\n";
            }
            std::cout << outputBuffer;
        }

        auto frameTime = std::chrono::steady_clock::now() - frameStart;
        if (frameTime < frameDelay) {
            std::this_thread::sleep_for(frameDelay - frameTime);
        }
    }

    std::cout << "\033[?25h\n[CHIP-8] Joc oprit.\n";
    return 0;
}
