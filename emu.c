#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define A state->a
#define B state->b
#define C state->c
#define BC (state->b<<8) | (state->c)
#define D state->d
#define E state->e
#define DE (state->d<<8) | (state->e)
#define H state->h
#define L state->l
#define HL (state->h<<8) | (state->l)
#define M state->memory[(state->h<<8) | (state->l)]
#define SP state->sp
#define PC state->pc
#define CY state->cc.cy

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

int Parity(int num) {
  int count = 0;
  while (num != 0) {
    if ((num & 1) == 1) count++;
    num = num >> 1;
  }
  return count % 2 == 0;
}

/*
 * Arithmetic Group
 */

void ArithFlags(uint16_t answer, State8080* state) {
  state->cc.z = (answer & 0xff) == 0; // Zero flag
  state->cc.s = (answer & 0x80) != 0; // Sign flag
  state->cc.cy = answer > 0xff;       // Carry
  state->cc.p = Parity(answer & 0xff);
}

void ADD(uint8_t addend, State8080* state) {
  uint16_t sum = (uint16_t) A + (uint16_t) addend;
  A = sum & 0xff; // 0xff as a mask to only look at last 8 bits
  ArithFlags(sum, state);
}

void ADC(uint8_t addend, State8080* state) {
  uint16_t sum = (uint16_t) A + (uint16_t) addend + (uint16_t) CY;
  A = sum & 0xff;
  ArithFlags(sum, state);
}

void SUB(uint8_t subtrahend, State8080* state) {
  uint16_t sum = (uint16_t) A - (uint16_t) subtrahend;
  A = sum & 0xff;
  ArithFlags(sum, state);
}

void SBB(uint8_t subtrahend, State8080* state) {
  uint16_t sum = (uint16_t) A - (uint16_t) subtrahend - (uint16_t) CY;
  A = sum & 0xff;
  ArithFlags(sum, state);
}

void DAD(uint16_t addend, State8080* state) {
  uint32_t sum = (uint16_t) HL + addend;
  H = (sum & 0xff00) >> 8;
  L = sum & 0xff;
  CY = sum > 0xffff; // Carry
}

int Emulate8080Op(State8080* state) {
  unsigned char *opcode = &state->memory[state->pc];

  switch (*opcode) {
  // NOP
  case 0x00: break;
  // LXI D16
  case 0x01: B = opcode[1]; C = opcode[2]; PC += 2; break;
  case 0x11: D = opcode[1]; E = opcode[2]; PC += 2; break;
  case 0x21: H = opcode[1]; L = opcode[2]; PC += 2; break;
  case 0x31: SP = (opcode[1] << 8) | opcode[2]; PC += 2; break;
  // INX
  case 0x03: C++; if (C == 0) B++; break;
  case 0x13: E++; if (E == 0) D++; break;
  case 0x23: L++; if (L == 0) H++; break;
  case 0x33: SP++; break;
  // INR
  case 0x04: B++; ArithFlags(B, state); break;
  case 0x0c: C++; ArithFlags(C, state); break;
  case 0x14: D++; ArithFlags(D, state); break;
  case 0x1c: E++; ArithFlags(E, state); break;
  case 0x24: H++; ArithFlags(H, state); break;
  case 0x2c: L++; ArithFlags(L, state); break;
  /* case 0x34: L++; ArithFlags(L, state); break; */
  case 0x3c: A++; ArithFlags(A, state); break;
  // DCX
  case 0x0b: C--; if (C == 0xff) B--; break;
  case 0x1b: E--; if (E == 0xff) D--; break;
  case 0x2b: L--; if (L == 0xff) H--; break;
  case 0x3b: SP--; break;
  // DAD
  case 0x09: DAD(BC, state); break;
  case 0x19: DAD(DE, state); break;
  case 0x29: DAD(HL, state); break;
  case 0x39: DAD(SP, state); break;
  // ADD
  case 0x80: ADD(B, state); break;
  case 0x81: ADD(C, state); break;
  case 0x82: ADD(D, state); break;
  case 0x83: ADD(E, state); break;
  case 0x84: ADD(H, state); break;
  case 0x85: ADD(L, state); break;
  case 0x86: ADD(M, state); break;
  case 0x87: ADD(A, state); break;
  // ADC
  case 0x88: ADC(B, state); break;
  case 0x89: ADC(C, state); break;
  case 0x8a: ADC(D, state); break;
  case 0x8b: ADC(E, state); break;
  case 0x8c: ADC(H, state); break;
  case 0x8d: ADC(L, state); break;
  case 0x8e: ADC(M, state); break;
  case 0x8f: ADC(A, state); break;
  // SUB
  case 0x90: SUB(B, state); break;
  case 0x91: SUB(C, state); break;
  case 0x92: SUB(D, state); break;
  case 0x93: SUB(E, state); break;
  case 0x94: SUB(H, state); break;
  case 0x95: SUB(L, state); break;
  case 0x96: SUB(M, state); break;
  case 0x97: SUB(A, state); break;
  // SBB
  case 0x98: SBB(B, state); break;
  case 0x99: SBB(C, state); break;
  case 0x9a: SBB(D, state); break;
  case 0x9b: SBB(E, state); break;
  case 0x9c: SBB(H, state); break;
  case 0x9d: SBB(L, state); break;
  case 0x9e: SBB(M, state); break;
  case 0x9f: SBB(A, state); break;
  // ADI D8
  case 0xc6: ADD(opcode[1], state); break;
  // SUI D8
  case 0xd6: SUB(opcode[1], state); break;
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
    state->a = -state->a; // why is this "-" in the x? shouldn't it be "!"
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
      state->cc.p = Parity(x);
      state->cc.cy = 0;
      state->a = x;
      state->pc += 1;
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
      state->cc.p = Parity(x);
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
