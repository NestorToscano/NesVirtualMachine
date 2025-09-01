#include <stdio.h> // std i/o
#include <stdint.h> // std int types
#include <signal.h> // handling signals
#include <stdlib.h> // std lib (malloc)
// UNIX
#include <unistd.h> // LL system calls
#include <fcntl.h> // file configs
#include <sys/time.h> // time
#include <sys/types.h> // common data types
#include <sys/termios.h> // terminal I/O
#include <sys/mman.h> // memory man.

// REGISTERS : value stored in a register are usable by the cpu
// which are then used to calculate other values into other registers
// and back into memory
enum {
  R_R0 = 0, // General Purpose Registers
  R_R1,     // = prev + 1
  R_R2,
  R_R3,
  R_R4,
  R_R5,
  R_R6,
  R_R7,
  R_PC,     // program counter
  R_COND,   // condition flags
  R_COUNT   // = 10
};

// CONDITION FLAGS : 
enum {
  FL_POS = 1 << 0, // positive = 1
  FL_ZRO = 1 << 1, // zero = 2
  FL_NEG = 1 << 2 // negative = 4
  < 2
};
// OPCODES : represents a simple task that the CPU caries out
// left 4 bits store the opcode and the rest are for parameters
// Machine code : 0001 000 000 1 00011 = ADD R0, R0, 3 
enum {
  OP_BR = 0,  // branch
  OP_ADD,     // add
  OP_LD,      // load
  OP_ST,      // store
  OP_JSR,     // jump register
  OP_AND,     // bitwise and
  OP_LDR,     // load register
  OP_STR,     // store register
  OP_RTI,     // unused
  OP_NOT,     // bitwise not
  OP_LDI,     // load indirext
  OP_STI,     // store indirect
  OP_JMP,     // jump
  OP_RES,     // unused
  OP_LEA,     // load effective address
  OP_TRAP     // execute trap
};

// Extending numbers with less than 16 bits for addition
// negative numbers have problems due to two's complement
uint16_t sign_extend(uint16_t x, int bit_count) { 
  if ((x >> (bit_count - 1)) & 1) { // checking MSB for neg
    x |= (0xFFFF << bit_count); // Filling in left bits with 1
  }
  return x;
}

// Updating most previous values sign added to a register
void update_flags(uint16_t r) {
  if (reg[r] == 0) reg[R_COND] = FL_ZRO;
  else if (reg[r] >> 15) reg[R_COND] = FL_NEG ;// MSB
  else reg[R_COND] = FL_POS;
}

#define MEMORY_MAX (1 << 16) // 1 * 2^16
uint16_t memory[MEMORY_MAX]; // 65536 possible memory locations
uint16_t reg[R_COUNT];

int main (int argc, const char* argv[]) {
  // Loading args
  if (argc < 2) {
    // show user usage string
    printf("lc3 [image-file1] ...\n");
    exit(2);
  }

  for (int j = 1; j < argc; ++j) {
    if (!read_image(argv[j])) {
      printf("failed to load image: %s\n", argv[j]);
      exit(1);
    }
  }
  // @setup
  
  // One condition flag should be set at any given time
  reg[R_COND] = FL_ZRO;

  // Setting starting positon for PC register
  // 0x3000 : default
  enum { PC_START = 0x3000 };
  reg[R_PC] = PC_START;

  int running = 1;
  while (running) {
    // Loading instruction from memory
    // Shifting PC to next location in memory
    uint16_t instr = mem_read(reg[R_PC]++);
    uint16_t op = instr >> 12; // only first four bits

    switch (op) {
      case OP_ADD: 
        {
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
        break;
      case OP_AND:
        {
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
        break;
      case OP_NOT:
        {
          uint16_t r0 = (instr >> 9) & 0x7;
          uint16_t r1 = (instr >> 6) & 0x7;

          reg[r0] = ~reg[r1];
          
          update_flags(r0);
        }
          break;
      case OP_BR:
        {
          uint16_t cond_flags = (instr >> 9) & 0x7; // n z p
          if ((cond_flags) & (reg[R_COND])) { // checking n z p against cpu cond flags
            uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
            reg[R_PC] += pc_offset;
          } 
        }
          break;
      case OP_JMP:
        {
          // R7 (111) contains the RET instructions
          uint16_t r1 = (instr >> 6) & 0x7; // BaseR

          reg[R_PC] = reg[r1]; 
        }
          break;
      case OP_JSR:
        {
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
          break;
      case OP_LD:
        {
          uint16_t r0 = (instr >> 9) & 0x7;
          uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
          reg[r0] = mem_read(reg[R_PC] + pc_offset);
          update_flags(r0);
        }
          break;
      case OP_LDI:
        {
          uint16_t r0 = (instr >> 9) & 0x7;
          uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
          reg[r0] = mem_read(mem_read(reg[R_PC] + pc_offset));;
          update_flags(r0);
        }
          break;
      case OP_LDR:
          break;
      case OP_LEA:
          break;
      case OP_ST:
          break;
      case OP_STI:
          break;
      case OP_STR:
          break;
      case OP_TRAP:
          break;
      case OP_RES:
          break;
      case OP_RTI:
          break;

      default:
        abort();
        break;     
    }

  }
  // @SHUTDOWN
}
