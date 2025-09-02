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

void op_add(uint16_t instr) {
  // Destination Register (DR) : 11-9
  uint16_t r0 = (instr >> 9) & 0x7;
  // First operand (SR1) : 8-6
  uint16_t r1 = (instr >> 6) & 0x7;
  // Mode (immediate/Register) : 5
  uint16_t imm_flag = (instr >> 5) & 0x1;

  if (imm_flag) { // immediate : 4-0
      uint16_t imm5 = sign_extend(instr & 0x1F, 5); // 5b -> 16b
      reg[r0] = reg[r1] + imm5;
  }
  else {
      uint16_t r2 = instr & 0x7;
      reg[r0] = reg[r1] + reg[r2];
  }

  update_flags(r0);
}

void op_and(uint16_t instr) {
  uint16_t r0 = (instr >> 9) & 0x7;
  uint16_t r1 = (instr >> 6) & 0x7;
  uint16_t imm_flag = (instr >> 5) & 0x1;

  if (imm_flag) {
      uint16_t imm5 = sign_extend(instr & 0x1F, 5);
      reg[r0] = reg[r1] & imm5;
  }
  else {
      uint16_t r2 = instr & 0x7;
      reg[r0] = reg[r1] & reg[r2];
  }
  update_flags(r0);
}

void op_not(uint16_t instr) {
  uint16_t r0 = (instr >> 9) & 0x7;
  uint16_t r1 = (instr >> 6) & 0x7;

  reg[r0] = ~reg[r1];

  update_flags(r0);
}

void op_br(uint16_t instr) {
  uint16_t cond_flags = (instr >> 9) & 0x7; // n z p
  if ((cond_flags) & (reg[R_COND])) { // checking n z p against cpu cond flags
      uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
      reg[R_PC] += pc_offset;
  }
}

void op_jmp(uint16_t instr) {
  // R7 (111) contains the RET instructions
  uint16_t r1 = (instr >> 6) & 0x7;

  reg[R_PC] = reg[r1];
}

void op_jsr(uint16_t instr) {
  reg[R_R7]=  reg[R_PC];

  uint16_t cond_flag = (instr >> 11) & 0x1;

  if (!cond_flag) {  // JSRR
      uint16_t r1 = (instr >> 6) & 0x7;
      reg[R_PC] = reg[r1];
  }
  else { // JSR
      uint16_t pc_offset  = sign_extend(instr & 0x7FF, 11);
      reg[R_PC] += pc_offset;
  }
}

void op_ld(uint16_t instr) {
  uint16_t r0 = (instr >> 9) & 0x7;
  uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
  reg[r0] = mem_read(reg[R_PC] + pc_offset);
  update_flags(r0);
}

void op_ldi(uint16_t instr) {
  uint16_t r0 = (instr >> 9) & 0x7;
  uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
  reg[r0] = mem_read(mem_read(reg[R_PC] + pc_offset));
  update_flags(r0);
}

void op_ldr(uint16_t instr) {
  uint16_t r0 = (instr >> 9) & 0x7;
  uint16_t r1 = (instr >> 6) & 0x7;
  uint16_t offset = sign_extend(instr & 0x3F, 6);
  reg[r0] = mem_read(reg[r1] + offset);
  update_flags(r0);
}

void op_lea(uint16_t instr) {
  uint16_t r0 = (instr >> 9) & 0x7;
  uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
  reg[r0] = reg[R_PC] + pc_offset;
  update_flags(r0);
}

void op_st(uint16_t instr) {
  uint16_t r0 = (instr >> 9) & 0x7;
  uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
  // writing value in register into memory at address
  mem_write(reg[R_PC] + pc_offset, reg[r0]);
}

void op_sti(uint16_t instr) {
  uint16_t r0 = (instr >> 9) & 0x7;
  uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
  // writing value in register into memory address held by another adress
  mem_write(mem_read(reg[R_PC] + pc_offset), reg[r0]);
}

void op_str(uint16_t instr) {
  uint16_t r0 = (instr >> 9) & 0x7;
  uint16_t r1 = (instr >> 6) & 0x7;
  uint16_t offset = sign_extend(instr & 0x3F, 6);
  mem_write(reg[r1] + offset, reg[r0]);
}
void op_trap(int* running, uint16_t instr) {
          reg[R_R7] = reg[R_PC];
          uint16_t trap_vector = instr & 0xFF;

          switch (trap_vector) {
            case TRAP_GETC:
              {
                trap_getc();
              }
              break;
            case TRAP_OUT:
              {
                trap_out();
              }
              break;
            case TRAP_PUTS:
              {
                uint16_t* c = memory + reg[R_R0]; // creating pointer to memory with offset
                trap_puts(c);
              }
              break;
            case TRAP_IN:
              {
                printf("Enter a character: ");
                char c = getchar();
                trap_in(c);
              }
              break;
            case TRAP_PUTSP:
              {
                uint16_t* c = memory + reg[R_R0];
                trap_putsp(c);
              } 
              break;
            case TRAP_HALT:
              {
                trap_halt(running);
              }
              break;      
          }
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