#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct ConditionCodes {
  // zero, set when result == 0
  uint8_t   z:1;
  // sign, set when bit 7 (most sig bit) is set
  uint8_t   s:1;
  // parity, set when answer has even parity
  uint8_t   p:1;
  // carry, set when instruction resulted in a carry or borrow, such as when
  // adding 255 + 255 using 8 bit registers, 510 is 9 bits 1_1111_1110
  uint8_t   cy:1;
  // auxillary carry, used for binary coded decimal math
  // Not used in Space Invaders
  uint8_t   ac:1;
  uint8_t   pad:3;
} ConditionCodes;

typedef struct State8080 {
  uint8_t   a;
  uint8_t   b;
  uint8_t   c;
  uint8_t   d;
  uint8_t   e;
  uint8_t   h;
  uint8_t   l;
  uint16_t  sp;
  uint16_t  pc;
  uint8_t   *memory;
  struct    ConditionCodes cc;
  uint8_t   int_enable;
} State8080;

void UnimplementedInstruction(State8080* state) {
  // pc will have advanced one, so undo that
  printf("Error: Unimplemented instruction\n");
  exit(1);
}

int Emulate8080Op(State8080* state) {
  unsigned char *opcode = &state->memory[state->pc];

  switch(*opcode) {
  case 0x00:
    // NOP
    break;
  case 0x01:
    // LXI B, D16
    // B <- byte 3, C <- byte 2
    state->b = opcode[1];
    state->c = opcode[2];
    state->pc += 2;
    break;
  case 0x02:
    // STAX B
    // (BC) < A
    state->b = state->a;
    break;
  case 0x0f:
    // RRC "Rotate A right"
    // A = A >> 1; bit 7 = prev bit 0; CY = prev bit 0
    {
      uint8_t x = state->a;
      // bit shift right 1, bit 0 shift left 7
      state->a = (x >> 1) | ((x & 1) << 7);
      state->cc.cy = (1 == (x & 1));
    }
    break;
  case 0x1f:
    // RAR "Rotate A right through carry"
    // A = A >> 1; bit 7 = CY; CY = prev bit 0
    {
      uint8_t x = state->a;
      state->a = (x >> 1) | (state->cc.cy << 7);
      state->cc.cy = (1 == (x & 1));
    }
    break;
  case 0x2f:
    // CMA
    // A <- !A
    state->a = ~state->a;
    break;
  case 0x41:
    // MOV B,C
    state->b = state->c;
    break;
  case 0x42:
    // MOV B,D
    state->b = state->d;
    break;
  case 0x43:
    // MOV B,E
    state->b = state->e;
    break;
  case 0x80:
    // ADD B "Register Form"
    // A <- A + B
    {
      // Use higher precision so that we can toggle the carry flag
      // 0xff as a mask to only look at last 8 bits
      uint16_t answer = (uint16_t) state->a + (uint16_t) state->b;
      state->cc.z = (answer & 0xff) == 0; // Zero flag
      state->cc.s = (answer & 0x80) != 0; // Sign flag
      state->cc.cy = answer > 0xff;       // Carry
      state->cc.p = Parity(answer & 0xff);
      state->a = answer & 0xff;
    }
    break;
  case 0x86:
    // ADD M "Memory Form"
    // A <- A + (HL)
    {
      // Get H, then shift it one byte left, and then combine it with L
      uint16_t offset = (state->h<<8) | (state->l);
      uint16_t answer = (uint16_t) state->a + state->memory[offset];
      state->cc.z = (answer & 0xff) == 0; // Zero flag
      state->cc.s = (answer & 0x80) != 0; // Sign flag
      state->cc.cy = answer > 0xff;     // Carry
      state->cc.p = Parity(answer & 0xff);
      state->a = answer & 0xff;
    }
    break;
  case 0xc1:
    // POP B
    // C <- (sp); B <- (sp + 1); sp <- sp + 2
    {
      state->c = state->memory[state->sp];
      state->b = state->memory[state->sp + 1];
      state->sp += 2;
    }
    break;
  case 0xc2:
    // JNZ addr
    if (0 == state->cc.z) // "Not-Z"
      state->pc = opcode[2] << 8 | opcode[1];
    else
      state->pc += 2;
    break;
  case 0xc3:
    // JMP addr
    state->pc = opcode[2] << 8 | opcode[1];
    break;
  case 0xc5:
    // PUSH B
    // (sp - 2) <- C; (sp - 1) <- B; sp <- sp - 2
    {
      state->memory[state->sp - 2] = state->c;
      state->memory[state->sp - 1] = state->b;
      state->sp -= 2;
    }
    break;
  case 0xc6:
    // ADI D8 "Immediate Form"
    // A <- A + byte
    {
      uint16_t answer = (uint16_t) state->a + (uint16_t) opcode[1];
      state->cc.z = (answer & 0xff) == 0; // Zero flag
      state->cc.s = (answer & 0x80) != 0; // Sign flag
      state->cc.cy = answer > 0xff;       // Carry
      state->cc.p = Parity(answer & 0xff);
      state->a = answer & 0xff;
      state->pc += 1;
    }
    break;
  case 0xc9:
    // RET
    // Get address from stack and update stack pointer
    state->pc = (state->memory[state->sp + 1] << 8) | state->memory[state->sp];
    state->sp += 2;
    break;
  case 0xcd:
    // CALL addr
    {
      uint16_t ret = state->pc + 2; // Address of the next instruction
      // Put address on the stack
      // 8080 is little-endian, so it stores it "backwards"
      state->memory[state->sp - 1] = (ret >> 8) & 0xff; // First byte
      state->memory[state->sp - 2] = ret & 0xff; // Last byte
      state->sp = state->sp - 2; // Move stack pointer
      state->pc = (opcode[2] << 8) | opcode[1];
    }
    break;
  case 0xe6:
    // ANI D8
    // A <- A & data
    {
      uint8_t x = state->a & opcode[1];
      state->cc.z = x == 0;
      // This is related to two's complement
      // If a byte is signed and the highest bit is 1, it's negative
      state->cc.s = (0x80 == (x & 0x80));
      state->cc.p = Parity(x, 8);
      state->cc.cy = 0;
      state->a = x;
      state->pc += 1;
    }
    break;
  case 0xf1:
    // POP PSW
    // flags <- (sp); A <- (sp + 1); sp <- sp + 2
    {
      state->a = state->memory[state->sp+1];
      // "Program Status Word"
      // Saves the flags using a bit array
      uint8_t psw = state->memory[state->sp];
      state->cc.z  = (0x01 == (psw & 0x01));
      state->cc.s  = (0x02 == (psw & 0x02));
      state->cc.p  = (0x04 == (psw & 0x04));
      state->cc.cy = (0x08 == (psw & 0x08));
      state->cc.ac = (0x10 == (psw & 0x10));
      state->sp += 2;
    }
    break;
  case 0xf5:
    // PUSH PSW
    // (sp - 2) <- flags; (sp - 1) <- A; sp <- sp - 2
    {
      state->memory[state->sp-1] = state->a;
      uint8_t psw = (state->cc.z |
                     state->cc.s  << 1 |
                     state->cc.p  << 2 |
                     state->cc.cy << 3 |
                     state->cc.ac << 4 );
      state->memory[state->sp-2] = psw;
      state->sp -= 2;
    }
    break;
  case 0xfe:
    // CPI D8 "Compare immediate with A"
    // A - data
    {
      // Sets the flags but doesn't store the result
      uint8_t x = state->a - opcode[1];
      state->cc.z = (x == 0); // Two numbers are equal
      state->cc.s = (0x80 == (x & 0x80));
      // Databook is unclear on how to handle parity
      state->cc.p = Parity(x, 8);
      // If A is greater, CY cleared since no borrow
      // If A is less, CY set since A had to borrow
      state->cc.cy = (state->a < opcode[1]);
      state->pc++;
    }
    break;
  default:   UnimplementedInstruction(state); break;
    // Arithmetic group notes:
    // ADC, ACI, SBB, SUI use the carry bit per the data book
    // INX and DCX do not affect the flags
    // DAD only affects the carry flag
    // INR and DCR do not affect the carry flag

    // Branch group notes:
    // PCHL unconditionally jumps to address in HL register
    // RST is part of this group
  }
  state->pc += 1; // for the opcode
}
