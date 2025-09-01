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

    switch (op) { // make functions and migrate to lc3.c for readability
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
          reg[r0] = mem_read(mem_read(reg[R_PC] + pc_offset));
          update_flags(r0);
        }
          break;
      case OP_LDR:
        {
          uint16_t r0 = (instr >> 9) & 0x7;
          uint16_t r1 = (instr >> 6) & 0x7;
          uint16_t offset = sign_extend(instr & 0x3F, 6);
          reg[r0] = mem_read(reg[r1] + offset);
          update_flags(r0);
        }
          break;
      case OP_LEA:
        {
          uint16_t r0 = (instr >> 9) & 0x7;
          uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
          reg[r0] = reg[R_PC] + pc_offset;
          update_flags(r0);
        }  
          break;
      case OP_ST:
        {
          uint16_t r0 = (instr >> 9) & 0x7;
          uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
          // writing value in register into memory at address
          mem_write(reg[R_PC] + pc_offset, reg[r0]); 
        }
          break;
      case OP_STI:
        {
          uint16_t r0 = (instr >> 9) & 0x7;
          uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
          // writing value in register into memory address held by another adress
          mem_write(mem_read(reg[R_PC] + pc_offset), reg[r0]);
        }
          break;
      case OP_STR:
        {
          uint16_t r0 = (instr >> 9) & 0x7;
          uint16_t r1 = (instr >> 6) & 0x7;
          uint16_t offset = sign_extend(instr & 0x3F, 6);
          mem_write(reg[r1] + offset, reg[r0]);
        }
          break;
      case OP_TRAP:
        {
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
                trap_halt(&running);
              }
              break;      
          }
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
