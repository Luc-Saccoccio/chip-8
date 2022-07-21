#include "chip8.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static uint8_t chip8_font[80] = {
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
static uint16_t opcode;
static uint8_t memory[4096];
static uint8_t V[16];
static uint16_t I;
static uint16_t PC;

uint8_t display[64 * 32];
uint8_t draw;

static uint8_t delay_timer;
static uint8_t sound_timer;

static uint16_t stack[16];
static uint16_t stack_pointer;

uint8_t key[16];

static void debug(void) {
  for (int i = 0; i < 4096; i++)
    fprintf(stdout, "%X ", memory[i]);
  fprintf(stdout, "\n");
}

void vm_init(void) {
  srand(time(NULL));
  int i;
  PC = 0x200;
  opcode = I = stack_pointer = 0;
  delay_timer = sound_timer = 0;

  for (i = 0; i < 2048; i++)
    display[i] = 0;
  draw = 0;

  for (i = 0; i < 16; i++) {
    stack[i] = 0;
    key[i] = V[i] = 0;
  }

  for (i = 0; i < 80; i++)
    memory[i] = chip8_font[i];
  for (; i < 4096; i++)
    memory[i] = 0;
}

int load_program(char *file) {
  FILE *fp = fopen(file, "rb");
  if (fp == NULL) {
    fprintf(stderr, "[ERROR] Couldn't open file %s\n", file);
    return 1;
  }

  fseek(fp, 0, SEEK_END);
  long length = ftell(fp);
  rewind(fp);
  fprintf(stdout, "Filesize: %d\n", (int)length);
  char *buffer = (char *)malloc(sizeof(char) * length);
  if (buffer == NULL) {
    fputs("[ERROR] Couldn't allocate buffer to read the ROM\n", stderr);
    fclose(fp);
    return 1;
  }

  size_t res = fread(buffer, 1, length, fp);
  if (res != length) {
    fputs("[ERROR] while reading the file\n", stderr);
    fclose(fp);
    return 1;
  }

  if (length < 3584)
    for (int i = 0; i < length; i++)
      memory[i + 512] = buffer[i];
  else
    fputs("[ERROR] ROM too big for memory\n", stderr);

  fclose(fp);
  free(buffer);
  return 0;
}

// TODO: Some error not appearing
void vm_cycle(void) {
  opcode = memory[PC] << 8 | memory[PC + 1];
  fprintf(stdout, "opcode: %X\n", opcode);
  // debug();
  switch (opcode & 0xF000) {
  case 0x0000:
    switch (opcode & 0x00FF) {
    case 0x00E0: /* CLS */
      for (int i = 0; i<64; i++)
        for (int j = 0; j<32; j++)
          display[i+(j*64)] = 0x0;
      draw = 1;
      PC += 2;
      break;
    case 0x00EE: /* RET */
      stack_pointer--;
      PC = stack[stack_pointer];
      PC += 2;
      break;
    default:
      fprintf(stderr, "[ERROR] Unknown opcode [0x0000]: 0x%X\n", opcode);
    }
    break;
  case 0x1000: /* JP addr */
    PC = opcode & 0x0FFF;
    break;
  case 0x2000: /* CALL addr */
    stack[stack_pointer] = PC;
    stack_pointer++;
    PC = opcode & 0x0FFF;
    break;
  case 0x3000: /* SE Vx, byte */
    if (V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF))
      PC += 4;
    else
      PC += 2;
    break;
  case 0x4000: /* SNE Vx, byte */
    if (V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF))
      PC += 4;
    else
      PC += 2;
    break;
  case 0x5000: /* SE Vx, Vy */
    if (V[opcode & 0x0F00 >> 8] == ((opcode & 0x00F0) >> 4))
      PC += 4;
    else
      PC += 2;
    break;
  case 0x6000: /* LD Vx, byte */
    V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
    PC += 2;
    break;
  case 0x7000: /* ADD Vx, byte */
    V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
    PC += 2;
    break;
  case 0x8000:
    switch (opcode & 0x000F) {
    case 0x0000: /* LD Vx, Vy */
      V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
      PC += 2;
      break;
    case 0x0001: /* OR Vx, Vy */
      V[(opcode & 0x0F00) >> 8] |= V[(opcode & 0x00F0) >> 4];
      PC += 2;
      break;
    case 0x0002: /* AND Vx, Vy */
      V[(opcode & 0x0F00) >> 8] &= V[(opcode & 0x00F0) >> 4];
      PC += 2;
      break;
    case 0x0003: /* XOR Vx, Vy */
      V[(opcode & 0x0F00) >> 8] ^= V[(opcode & 0x00F0) >> 4];
      PC += 2;
      break;
    case 0x0004: /* ADD Vx, Vy */
      if (V[(opcode & 0x00F0) >> 4] > (0xFF - V[(opcode & 0x0F00) >> 8]))
        V[0xF] = 1;
      else
        V[0xF] = 0;
      V[(opcode & 0x0F00) >> 8] += V[(opcode & 0x00F0) >> 4];
      PC += 2;
      break;
    case 0x0005: /* SUB Vx, Vy */
      if (V[(opcode & 0x00F0) >> 4] > (V[(opcode & 0x0F00) >> 8]))
        V[0xF] = 0;
      else
        V[0xF] = 1;
      V[(opcode & 0x0F00) >> 8] -= V[(opcode & 0x00F0) >> 4];
      PC += 2;
      break;
    case 0x0006: /* SHR Vx {, Vy} */
      V[0xF] = V[(opcode & 0x0F00) >> 8] & 0x1;
      V[(opcode & 0x0F00) >> 8] >>= 1;
      PC += 2;
      break;
    case 0x0007: /* SUBN Vx, Vy */
      if (V[(opcode & 0x0F00) >> 8] > V[(opcode & 0x00F0) >> 4])
        V[0xF] = 0;
      else
        V[0xF] = 1;
      V[(opcode & 0x0F00) >> 8] =
          V[(opcode & 0x00F0) >> 4] - V[(opcode & 0x0F00) >> 8];
      PC += 2;
      break;
    case 0x000E: /* SHL Vx {, Vy} */
      V[0xF] = V[(opcode & 0x0F00) >> 8] >> 7;
      V[(opcode & 0x0F00) >> 8] <<= 1;
      PC += 2;
      break;
    default:
      fprintf(stderr, "[ERROR] Unknown opcode [0x8000]: 0x%X\n", opcode);
      break;
    }
    break;
  case 0x9000: /* SNE Vx, Vy */
    if (V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4])
      PC += 4;
    else
      PC += 2;
    break;
  case 0xA000: /* LD I, addr */
    I = opcode & 0x0FFF;
    PC += 2;
    break;
  case 0xB000: /* JP V0, addr */
    PC = (opcode & 0x0FFF) + V[0x0];
    break;
  case 0xC000: /* RND Vx, byte */
    V[(opcode & 0x0F00) >> 8] = (rand() % 0xFF) & (opcode & 0x00FF);
    PC += 2;
    break;
  case 0xD000: /* DRW Vx, Vy, nibble */
  {
    uint8_t x = V[(opcode & 0x0F00) >> 8];
    uint8_t y = V[(opcode & 0x00F0) >> 4];
    uint8_t n = opcode & 0x000F;
    uint8_t pixel;

    V[0xF] = 0;
    for (int i = 0; i < n; i++) {
      pixel = memory[I + i];
      for (int j = 0; j < 8; j++) {
        if ((pixel & (0x80 >> j)) != 0) {
          if (display[x + j + ((y + i) * 64)] != 0)
            V[0xF] = 1;
          display[x + j + ((y + i) * 64)] ^= 1;
        }
      }
    }
    draw = 1;
    PC += 2;
  } break;
  case 0xE000:
    switch (opcode & 0x00FF) {
    case 0x009E: /* SKP Vx */
      if (key[V[(opcode & 0x0F00) >> 8]] != 0)
        PC += 4;
      else
        PC += 2;
      break;
    case 0x00A1: /* SKP Vx */
      if (key[V[(opcode & 0x0F00) >> 8]] == 0)
        PC += 4;
      else
        PC += 2;
      break;
    default:
      fprintf(stderr, "[ERROR] Unknown opcode [0xE000]: 0x%X\n", opcode);
      break;
    }
    break;
  case 0xF000:
    switch (opcode & 0x00FF) {
    case 0x0007: /* LD Vx, DT */
      V[(opcode & 0x0F00) >> 8] = delay_timer;
      PC += 2;
      break;
    case 0x000A: /* LD Vx, K */
    {
      int pressed = 0;
      for (int i = 0; i < 16; i++)
        if (key[i] != 0) {
          V[(opcode & 0x0F00) >> 8] = 1;
          pressed = 1;
        }
      if (!pressed)
        return;
      PC += 2;
    } break;
    case 0x0015: /* LD DT, Vx */
      delay_timer = V[(opcode & 0x0F00) >> 8];
      PC += 2;
      break;
    case 0x0018: /* LD ST, Vx */
      sound_timer = V[(opcode & 0x0F00) >> 8];
      PC += 2;
      break;
    case 0x001E: /* ADD I, Vx */
      if ((I += V[(opcode & 0x0F00) >> 8]) > 0xFFF)
        V[0xF] = 1;
      else
        V[0xF] = 0;
      PC += 2;
      break;
    case 0x0029: /* LD F, Vx */
      I = V[(opcode & 0x0F00) >> 8] * 0x5;
      PC += 2;
      break;
    case 0x0033: /* LD B, Vx */
      memory[I] = V[(opcode & 0x0F00) >> 8] / 100;
      memory[I + 1] = (V[(opcode & 0x0F00) >> 8] / 10) % 10;
      memory[I + 2] = (V[(opcode & 0x0F00) >> 8] % 100) % 10;
      PC += 2;
      break;
    case 0x0055: /* LD [I], Vx */
      for (int i = 0; i <= ((opcode & 0x0F00) >> 8); i++)
        memory[I + i] = V[i];
      // In Chip-8 & Chip-48 implmentations, I is left incremented.
      // In SCHIP, it should be unmodified,
      I += ((opcode & 0x0F00) >> 8) + 1;
      PC += 2;
      break;
    case 0x0065: /* LD Vx, [I] */
      for (int i = 0; i <= ((opcode & 0x0F00) >> 8); i++)
        V[i] = memory[I + i];
      // In Chip-8 & Chip-48 implmentations, I is left incremented.
      // In SCHIP, it should be unmodified,
      I += ((opcode & 0x0F00) >> 8) + 1;
      PC += 2;
      break;
    default:
      fprintf(stderr, "[ERROR] Unknown opcode [0xF000]: 0x%X\n", opcode);
      break;
    }
    break;

  default:
    fprintf(stderr, "[ERROR] Unknown opcode: 0x%X\n", opcode);
  }

  if (delay_timer > 0)
    delay_timer--;
  if (sound_timer > 0) {
    // TODO: SOUND
    fputs("BEEP:\n", stdout);
    sound_timer--;
  }
}
