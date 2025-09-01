#include "lc3.h"


uint16_t memory[MEMORY_MAX]; // 65536 possible memory locations
uint16_t reg[R_COUNT];

uint16_t sign_extend(uint16_t x, int bit_count) {
  if ((x >> (bit_count - 1)) & 1) { // checking MSB for neg
    x |= (0xFFFF << bit_count); // Filling in left bits with 1
  }
  return x;
}

void update_flags(uint16_t r) {
  if (reg[r] == 0) reg[R_COND] = FL_ZRO;
  else if (reg[r] >> 15) reg[R_COND] = FL_NEG ;// MSB
  else reg[R_COND] = FL_POS;
}
uint16_t swap16(uint16_t x) {
  return (x << 8) | (x >> 8);
}
void read_image_file(FILE* file) {
  uint16_t origin; // first 16 bits of program designate program start
  fread(&origin, sizeof(origin), 1, file);
  origin = swap16(origin); // big-endian (swap first half with second half)

  uint16_t max_read = MEMORY_MAX - origin;
  uint16_t* p = memory + origin;
  size_t read = fread(p, sizeof(uint16_t), max_read, file);

  while (read > 0) { 
    *p = swap16(*p); // swap each word back to little endian
    ++p;
    --read;
  }
}
int read_image(const char* image_path) { // string path for convenience
    FILE* file = fopen(image_path, "rb");
    if (!file) { return 0; };
    read_image_file(file);
    fclose(file);
    return 1;
}
uint16_t mem_read(uint16_t address) {
  if (address == MR_KBSR) { // keyboard input
    if (check_key()) {
      memory[MR_KBSR] = (1 << 15);
      memory[MR_KBDR] = getchar();
    }
    else {
      memory[MR_KBSR] = 0;
    }
  }
  return memory[address];
}
void mem_write(uint16_t address, uint16_t val) {
  memory[address] = val;
}

// TRAP routines
uint16_t trap_getc() {
  reg[R_R0] = (uint16_t)getchar();
  update_flags(R_R0);
}   
void trap_out() {
  putc((char)reg[R_R0], stdout);
  fflush(stdout);
} 
void trap_puts(uint16_t* c) {
  while(*c != 0x0000) {
    putc((char)*c, stdout);
    ++c;
  }
  fflush(stdout);
} 
uint16_t trap_in(char c) {
  putc(c, stdout);
  fflush(stdout);
  reg[R_R0] = (uint16_t)c;
  update_flags(R_R0);
}    
void trap_putsp(uint16_t* c) {
  while (*c != 0x0000) { // each word is 16 bytes but holds two 8 byte chars
    char char1 = (*c) & 0xFF;
    putc(char1, stdout);
    char char2 = (*c) >> 8;
    if (char2) putc(char2, stdout);
    fflush(stdout);
  }
}  
void trap_halt(int* running) {
  puts("HALT");
  fflush(stdout);
  *running = 0;
}

struct termios original_tio;
void disable_input_buffering() {
    tcgetattr(STDIN_FILENO, &original_tio);
    struct termios new_tio = original_tio;
    new_tio.c_lflag &= ~ICANON & ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
}
void restore_input_buffering() {
    tcsetattr(STDIN_FILENO, TCSANOW, &original_tio);
}
uint16_t check_key() {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    return select(1, &readfds, NULL, NULL, &timeout) != 0;
}
void handle_interrupt(int signal) {
    restore_input_buffering();
    printf("\n");
    exit(-2);
}

void signal_buffering() {
  signal(SIGINT, handle_interrupt);
  disable_input_buffering();
}