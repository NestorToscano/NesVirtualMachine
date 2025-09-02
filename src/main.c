#include "lc3.h"

int main(int argc, const char* argv[]) {
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
  signal_buffering();
  
  // One condition flag should be set at any given time
  reg[R_COND] = FL_ZRO;

  // Setting starting positon for PC register to leave room for trap routines
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
          op_add(instr);
        }
        break;
      case OP_AND:
        {
          op_and(instr);
        }
        break;
      case OP_NOT:
        {
          op_not(instr);
        }
        break;
      case OP_BR:
        {
          op_br(instr);
        }
        break;
      case OP_JMP:
        {
          op_jmp(instr);
        }
        break;
      case OP_JSR:
        {
          op_jsr(instr);
        }
        break;
      case OP_LD:
        {
          op_ld(instr);
        }
        break;
      case OP_LDI:
        {
          op_ldi(instr);
        }
        break;
      case OP_LDR:
        {
          op_ldr(instr);
        }
        break;
      case OP_LEA:
        {
          op_lea(instr);
        }  
        break;
      case OP_ST:
        {
          op_st(instr);
        }
        break;
      case OP_STI:
        {
          op_sti(instr);
        }
        break;
      case OP_STR:
        {
          op_str(instr);
        }
        break;
      case OP_TRAP:
        {
          op_trap(&running, instr);
        }
        break;
      case OP_RES:
      case OP_RTI:
      default:
        abort();
        break;     
    }
  }
  restore_input_buffering();
}
