#ifndef LC3_H // prevent multiple includes
#define LC3_H

#include <stdint.h>
#include <stdio.h> // std i/o
#include <stdlib.h> // std lib (malloc)
#include <sys/termios.h> // terminal I/O

#include <signal.h> // handling signals
// UNIX
#include <unistd.h> // LL system calls
#include <fcntl.h> // file configs
#include <sys/time.h> // time
#include <sys/types.h> // common data types
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
  OP_LDI,     // load indirect
  OP_STI,     // store indirect
  OP_JMP,     // jump
  OP_RES,     // unused
  OP_LEA,     // load effective address
  OP_TRAP     // execute trap
};

// TRAPCODES : routines for interacting with I/O devices
enum {
  TRAP_GETC = 0x20, // get char from keyboard, not echoed onto terminal
  TRAP_OUT = 0x21, // output a character
  TRAP_PUTS = 0x22, // output a word string
  TRAP_IN = 0x23, // get char from keyboard, echoed onto terminal
  TRAP_PUTSP = 0x24,// output a byte string
  TRAP_HALT = 0x25 // halt the program
};

// MEMORY MAPPED REGISTERS : indicating if and what key was pressed
enum
{
    MR_KBSR = 0xFE00, // keyboard status 
    MR_KBDR = 0xFE02  // keyboard data 
};

#define MEMORY_MAX (1 << 16) // 1 * 2^16
extern uint16_t memory[MEMORY_MAX]; // 65536 possible memory locations
extern uint16_t reg[R_COUNT];

// Extending numbers with less than 16 bits for addition
// negative numbers have problems due to two's complement
uint16_t sign_extend(uint16_t x, int bit_count);

// Updating most previous values sign added to a register
void update_flags(uint16_t r);

// Memory reading and wriitng
uint16_t mem_read(uint16_t address);
void mem_write(uint16_t address, uint16_t val);
uint16_t swap16(uint16_t x);
void read_image_file(FILE* file);
int read_image(const char* image_path);

// TRAP routines
uint16_t trap_getc();       
void trap_out();     
void trap_puts(uint16_t* c);    
uint16_t trap_in(char c);         
void trap_putsp(uint16_t* c);   
void trap_halt(int* running);    

// input buffering (copied)
extern struct termios original_tio;
void disable_input_buffering();
void restore_input_buffering();
uint16_t check_key();

void signal_buffering();

#endif