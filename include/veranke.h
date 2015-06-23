/*
 * The MIT License (MIT)
 *
 * Copyright © 2015 Allen Goodman
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the “Software”),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS,” WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef VERANKE_H

#define VERANKE_H

#include <array>
#include <cstdlib>

class Veranke {
public:
  Veranke(): delay_timer(0), index(0), program_counter(0x200), sound_timer(0), stack_pointer(0) {
    std::array<std::uint8_t, 80> fontset = {
      0xF0 ,0x90, 0x90, 0x90, 0xF0, // 0
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

    for (size_t i = 0; i < 80; ++i) {
      memory[i] = fontset[i];
    }
  }

  std::uint16_t fetch(void) {
    return memory[program_counter] << 8 | memory[program_counter + 1];
  }

  void decode_and_execute(void) {
    std::uint16_t opcode = fetch();

    std::uint16_t x;
    std::uint16_t y;

    switch (opcode & 0xF000) {
        case 0x0000:
          switch (opcode & 0x00FF) {
            /*
             * CLS
             *
             * Clear the display.
             */
            case 0x00E0:
              video_memory.fill(0);

              program_counter += 2;

              break;

            /*
             * RET
             *
             * Return from a subroutine.
             *
             * The interpreter sets the program counter to the address at
             * the top of the stack, then subtracts 1 from the stack
             * pointer.
             */
            case 0x00EE:
              program_counter = stack[--stack_pointer];

              program_counter = (std::uint16_t) (program_counter + 2);

              break;

            default:
              break;
          }

          break;

        /*
         * JP addr
         *
         * Jump to location nnn.
         *
         * The interpreter sets the program counter to nnn.
         */
        case 0x1000:
          program_counter = (uint16_t) (opcode & 0x0FFF);

          break;

        /*
         * CALL addr
         *
         * Call subroutine at nnn.
         *
         * The interpreter increments the stack pointer, then puts the
         * current PC on the top of the stack. The PC is then set to nnn.
         */
        case 0x2000: {
          uint16_t addr = (uint16_t) (opcode & 0x0FFF);

          stack[stack_pointer++] = program_counter;

          program_counter = addr;
        }

          break;

        /*
         * SE Vx, byte
         *
         * Skip next instruction if Vx = kk.
         *
         * The interpreter compares register Vx to kk, and if they are
         * equal, increments the program counter by 2.
         */
        case 0x3000: {
          x = (uint16_t) ((opcode & 0x0F00) >> 0x8);

          uint16_t byte = (uint16_t) (opcode & 0x00FF);

          if (registers[x] == byte) {
            program_counter += 2;
          }

          program_counter += 2;
        }

          break;

        /*
         * SNE Vx, byte
         *
         * Skip next instruction if Vx != kk.
         *
         * The interpreter compares register Vx to kk, and if they are
         * not equal, increments the program counter by 2.
         */
        case 0x4000: {
          x = (uint16_t) ((opcode & 0x0F00) >> 0x8);

          uint16_t byte = (uint16_t) (opcode & 0x00FF);

          if (registers[x] != byte) {
            program_counter += 2;
          }

          program_counter += 2;
        }

          break;

        /*
         * SE Vx, Vy
         *
         * Skip next instruction if Vx = Vy.
         *
         * The interpreter compares register Vx to register Vy, and if
         * they are equal, increments the program counter by 2.
         */
        case 0x5000: {
          x = (uint16_t) ((opcode & 0x0F00) >> 0x8);

          y = (uint16_t) ((opcode & 0x00F0) >> 0x4);

          if ((opcode & 0x000F) != 0) {
            break;
          }

          if (registers[x] == registers[y]) {
            program_counter += 2;
          }

          program_counter += 2;
        }

          break;

        /*
         * LD Vx, byte
         *
         * Set Vx = kk.
         *
         * The interpreter puts the value kk into register Vx.
         */
        case 0x6000: {
          x = (uint16_t) ((opcode & 0x0F00) >> 0x8);

          uint16_t byte = (uint16_t) (opcode & 0x00FF);

          registers[x] = (uint8_t) byte;

          program_counter += 2;

          break;
        }

        /*
         * ADD Vx, byte
         *
         * Set Vx = Vx + kk.
         *
         * Adds the value kk to the value of register Vx, then stores
         * the result in Vx.
         */
        case 0x7000: {
          x = (uint16_t) ((opcode & 0x0F00) >> 0x8);

          uint16_t byte = (uint16_t) (opcode & 0x00FF);

          registers[x] += byte;

          program_counter += 2;
        }

          break;

        case 0x8000: {
          x = (uint16_t) ((opcode & 0x0F00) >> 0x8);
          y = (uint16_t) ((opcode & 0x00F0) >> 0x4);

          switch (opcode & 0x000F) {
            /*
             * LD Vx, Vy
             *
             * Set Vx = Vy.
             *
             * Stores the value of register Vy in register Vx.
             */
            case 0x0000:
              registers[x] = registers[y];

              program_counter += 2;

              break;

            /*
             * OR Vx, Vy
             *
             * Set Vx = Vx OR Vy.
             *
             * Performs a bitwise OR on the values of Vx and Vy, then
             * stores the result in Vx. A bitwise OR compares the
             * corrseponding bits from two values, and if either bit is
             * 1, then the same bit in the result is also 1. Otherwise,
             * it is 0.
             */
            case 0x0001:
              registers[x] = registers[x] | registers[y];

              program_counter += 2;

              break;

            /*
             * AND Vx, Vy
             *
             * Set Vx = Vx AND Vy.
             *
             * Performs a bitwise AND on the values of Vx and Vy, then
             * stores the result in Vx. A bitwise AND compares the
             * corrseponding bits from two values, and if both bits are
             * 1, then the same bit in the result is also 1. Otherwise,
             * it is 0.
             */
            case 0x0002:
              registers[x] = registers[x] & registers[y];

              program_counter += 2;

              break;

            /*
             * XOR Vx, Vy
             *
             * Set Vx = Vx XOR Vy.
             *
             * Performs a bitwise exclusive OR on the values of Vx and
             * Vy, then stores the result in Vx. An exclusive OR
             * compares the corrseponding bits from two values, and if
             * the bits are not both the same, then the corresponding
             * bit in the result is set to 1. Otherwise, it is 0.
             */
            case 0x0003:
              registers[x] = registers[x] ^ registers[y];

              program_counter += 2;

              break;

            /*
             * ADD Vx, Vy
             *
             * Set Vx = Vx + Vy, set VF = carry.
             *
             * The values of Vx and Vy are added together. If the result
             * is greater than 8 bits (i.e., > 255,) VF is set to 1,
             * otherwise 0. Only the lowest 8 bits of the result are
             * kept, and stored in Vx.
             */
            case 0x0004:
              if (registers[y] > (0xFF - registers[x])) {
                registers[0xF] = 1;
              } else {
                registers[0xF] = 0;
              }

              registers[x] += registers[y];

              program_counter += 2;

              break;

            /*
             * SUB Vx, Vy
             *
             * Set Vx = Vx - Vy, set VF = NOT borrow.
             *
             * If Vx > Vy, then VF is set to 1, otherwise 0. Then Vy is
             * subtracted from Vx, and the results stored in Vx.
             */
            case 0x0005:
              if (registers[x] > registers[y]) {
                registers[0xF] = 0x1;
              } else {
                registers[0xF] = 0x0;
              }

              registers[x] = registers[x] - registers[y];

              program_counter += 2;

              break;

            /*
             * SHR Vx {, Vy}
             *
             * Set Vx = Vx SHR 1.
             *
             * If the least-significant bit of Vx is 1, then VF is set
             * to 1, otherwise 0. Then Vx is divided by 2.
             */
            case 0x0006:
              if (registers[x] & 0x1) {
                registers[0xF] = 1;
              } else {
                registers[0x0] = 0;
              }

              registers[x] /= 2;

              program_counter += 2;

              break;

            /*
             * SUBN Vx, Vy
             *
             * Set Vx = Vy - Vx, set VF = NOT borrow.
             *
             * If Vy > Vx, then VF is set to 1, otherwise 0. Then Vx is
             * subtracted from Vy, and the results stored in Vx.
             */
            case 0x0007:
              if (registers[y] > registers[x]) {
                registers[0xF] = 0x1;
              } else {
                registers[0xF] = 0x0;
              }

              registers[x] = registers[y] - registers[x];

              program_counter += 2;

              break;

            /*
             * SHL Vx {, Vy}
             *
             * Set Vx = Vx SHL 1.
             *
             * If the most-significant bit of Vx is 1, then VF is set to
             * 1, otherwise to 0. Then Vx is multiplied by 2.
             */
            case 0x000E:
              if (registers[x] & 0x80) {
                registers[0xF] = 1;
              } else {
                registers[0xF] = 0;
              }

              registers[x] *= 2;

              program_counter += 2;

              break;

            default:
              break;
          }
        }

          break;

        /*
         * SNE Vx, Vy
         *
         * Skip next instruction if Vx != Vy.
         *
         * The values of Vx and Vy are compared, and if they are not
         * equal, the program counter is increased by 2.
         */
        case 0x9000: {
          uint16_t x = (uint16_t) ((opcode & 0x0F00) >> 0x8);
          uint16_t y = (uint16_t) ((opcode & 0x00F0) >> 0x4);

          if ((opcode & 0x000F) != 0) {
            break;
          }

          if (registers[x] != registers[y]) {
            program_counter += 2;
          }

          program_counter += 2;
        }

          break;

        /*
         * LD I, addr
         *
         * Set I = nnn.
         *
         * The value of register I is set to nnn.
         */
        case 0xA000: {
          index = (uint16_t) (opcode & 0x0FFF);

          program_counter += 2;
        }

          break;

        /*
         * JP V0, addr
         *
         * Jump to location nnn + V0.
         *
         * The program counter is set to nnn plus the value of V0.
         */
        case 0xB000:
          program_counter = (uint16_t) ((opcode & 0x0FFF) + registers[0]);

          break;

        /*
         * RND Vx, byte
         *
         * Set Vx = random byte AND kk.
         *
         * The interpreter generates a random number from 0 to 255,
         * which is then ANDed with the value kk. The results are stored
         * in Vx. See instruction 8xy2 for more information on AND.
         */
        case 0xC000: {
          uint16_t x = (uint16_t) ((opcode & 0x0F00) >> 0x8);

          uint16_t byte = (uint16_t) (opcode & 0x00FF);

          registers[x] = (uint8_t) ((rand() % 255) & byte);

          program_counter += 2;
        }

          break;

        /*
         * DRW Vx, Vy, nibble
         *
         * Display n-byte sprite starting at memory location I at
         * (Vx, Vy), set VF = collision.
         *
         * The interpreter reads n bytes from memory, starting at the
         * address stored in I. These bytes are then displayed as
         * sprites on screen at coordinates (Vx, Vy). Sprites are XORed
         * onto the existing screen. If this causes any pixels to be
         * erased, VF is set to 1, otherwise it is set to 0. If the
         * sprite is positioned so part of it is outside the coordinates
         * of the display, it wraps around to the opposite side of the
         * screen. See instruction 8xy3 for more information on XOR, and
         * section 2.4, Display, for more information on the Chip-8
         * screen and sprites.
         */
        case 0xD000: {
          x = registers[(opcode & 0x0F00) >> 0x8];

          y = registers[(opcode & 0x00F0) >> 0x4];

          uint16_t nibble = (uint16_t) (opcode & 0x000F);

          uint16_t pixel;

          registers[0xF] = 0;

          for (size_t i = 0; i < nibble; ++i) {
            pixel = memory[index + i];

            for (size_t j = 0; j < 8; ++j) {
              if ((pixel & (0x80 >> j)) != 0) {
                if (video_memory[x + j + ((y + i) * 64)] != 0) {
                  registers[0xF] = 1;
                }

                video_memory[x + j + ((y + i) * 64)] ^= 1;
              }
            }
          }

          program_counter += 2;
        }

          break;

        case 0xE000:
          switch (opcode & 0x00FF) {
            /*
             * SKP Vx
             *
             * Skip next instruction if key with the value of Vx is
             * pressed.
             *
             * Checks the keyboard, and if the key corresponding to the
             * value of Vx is currently in the down position, PC is
             * increased by 2.
             */
            case 0x009E: {
              x = (uint16_t) ((opcode & 0x0F00) >> 0x8);

              if (keypad[registers[x]] != 0) {
                program_counter += 2;
              }

              program_counter += 2;
            }

              break;

            /*
             * SKNP Vx
             *
             * Skip next instruction if key with the value of Vx is not
             * pressed.
             *
             * Checks the keyboard, and if the key corresponding to the
             * value of Vx is currently in the up position, PC is
             * increased by 2.
             */
            case 0x00A1: {
              x = (uint16_t) ((opcode & 0x0F00) >> 0x8);

              if (keypad[registers[x]] == 0) {
                program_counter += 2;
              }

              program_counter += 2;
            }

              break;

            default:
              break;
          }

          break;

        case 0xF000:
          switch (opcode & 0x00FF) {
            /*
             * LD Vx, DT
             *
             * Set Vx = delay timer value.
             *
             * The value of DT is placed into Vx.
             */
            case 0x0007: {
              x = (uint16_t) ((opcode & 0x0F00) >> 0x8);

              registers[x] = delay_timer;

              program_counter += 2;
            }

              break;

            /*
             * LD Vx, K
             *
             * Wait for a key press, store the value of the key in Vx.
             *
             * All execution stops until a key is pressed, then the
             * value of that key is stored in Vx.
             */
            case 0x000A: {
              x = (std::uint16_t) (opcode & 0x0F00) >> 0x0008;

              for (std::uint16_t i = 0; i < 16; i++) {
                if (keys[i] == 1) {
                  registers[x] = (unsigned char) i;
                }
              }

              program_counter += 2;
            }

              break;

            /*
             * LD DT, Vx
             *
             * Set delay timer = Vx.
             *
             * DT is set equal to the value of Vx.
             */
            case 0x0015: {
              x = (uint16_t) ((opcode & 0x0F00) >> 0x8);

              delay_timer = registers[x];

              program_counter += 2;
            }

              break;

            /*
             * LD ST, Vx
             *
             * Set sound timer = Vx.
             *
             * ST is set equal to the value of Vx.
             */
            case 0x0018: {
              x = (uint16_t) ((opcode & 0x0F00) >> 0x8);

              sound_timer = registers[x];

              program_counter += 2;
            }

              break;

            /*
             * ADD I, Vx
             *
             * Set I = I + Vx.
             *
             * The values of I and Vx are added, and the results are
             * stored in I.
             */
            case 0x001E: {
              x = (uint16_t) ((opcode & 0x0F00) >> 0x8);

              index += registers[x];

              program_counter += 2;
            }

              break;

            /*
             * LD F, Vx
             *
             * Set I = location of sprite for digit Vx.
             *
             * The value of I is set to the location for the hexadecimal
             * sprite corresponding to the value of Vx. See section 2.4,
             * Display, for more information on the Chip-8 hexadecimal
             * font.
             */
            case 0x0029: {
              x = (uint16_t) ((opcode & 0x0F00) >> 0x8);

              index = (uint16_t) (registers[x] * 0x5);

              program_counter += 2;
            }

              break;

            /*
             * LD B, Vx
             *
             * Store BCD representation of Vx in memory locations I,
             * I+1, and I+2.
             *
             * The interpreter takes the decimal value of Vx, and places
             * the hundreds digit in memory at location in I, the tens
             * digit at location I+1, and the ones digit at location I+2.
             */
            case 0x0033: {
              x = (uint16_t) ((opcode & 0x0F00) >> 0x8);

              uint8_t tmp = (uint8_t) (registers[x] % 100);

              memory[index] = (uint8_t) (registers[x] / 100);

              memory[index + 1] = (uint8_t) ((registers[x] / 10) % 10);

              memory[index + 2] = (uint8_t) (tmp % 10);

              program_counter += 2;
            }

              break;

            /*
             * LD [I], Vx
             *
             * Store registers V0 through Vx in memory starting at
             * location I.
             *
             * The interpreter copies the values of registers V0 through
             * Vx into memory, starting at the address in I.
             */
            case 0x0055: {
              x = (uint16_t) ((opcode & 0x0F00) >> 0x8);

              for (size_t i = 0; i <= x; ++i) {
                memory[index + i] = registers[i];
              }

              program_counter += 2;
            }

              break;

            /*
             * LD Vx, [I]
             *
             * Read registers V0 through Vx from memory starting at
             * location I.
             *
             * The interpreter reads values from memory starting at
             * location I into registers V0 through Vx.
             */
            case 0x0065: {
              x = (uint16_t) ((opcode & 0x0F00) >> 0x8);

              for (size_t i = 0; i <= x; ++i) {
                registers[i] = memory[index + i];
              }

              program_counter += 2;
            }

              break;

            default:
              break;
          }

          break;

        default:
          break;
      }
  }

  std::array<std::uint8_t, 16> keypad;

  std::array<std::uint8_t, 4096> memory;

  std::array<std::uint8_t, 2048> video_memory;

  std::uint8_t delay_timer;

  std::array<std::uint8_t, 16> registers;

  std::uint16_t index;

  std::uint16_t program_counter;

  std::uint8_t sound_timer;

  std::uint8_t stack_pointer;

  std::array<std::uint16_t, 16> stack;

  std::array<std::uint8_t, 16> keys;
};

#endif
