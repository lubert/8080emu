#include <stdio.h>
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
    state->b = opcode[1];
    state->c = opcode[2];
    state->pc += 2;
    break;
  case 0x02:
    // STAX B
    state->b = state->a
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
  case 0x80:
    // ADD B "Register Form"
    // A <- A + B
    // Use higher precision so that we can toggle the carry flag
    // 0xff as a mask to only look at last 8 bits
    uint16_t answer = (uint16_t) state->a + (uint16_t) state->b;
    state->cc.z = answer & 0xff == 0; // Zero flag
    state->cc.s = answer & 0x80 != 0; // Sign flag
    state->cc.cy = answer > 0xff;     // Carry
    state->cc.p = Parity(answer & 0xff);
    state->a = answer & 0xff;
  case 0xC6:
    // ADI D8 "Immediate Form"
    // A <- A + byte
    uint16_t answer = (uint16_t) state->a + (uint16_t) opcode[1];
    state->cc.z = answer & 0xff == 0; // Zero flag
    state->cc.s = answer & 0x80 != 0; // Sign flag
    state->cc.cy = answer > 0xff;     // Carry
    state->cc.p = Parity(answer & 0xff);
    state->a = answer & 0xff;
    state->pc += 1;
  case 0x86:
    // ADD M "Memory Form"
    // A <- A + (HL)
    // Get H, then shift it one byte left, and then combine it with L
    uint16_t offset = (state->h<<8) | (state->l);
    uint16_t answer = (uint16_t) state->a + state->memory[offset];
    state->cc.z = answer & 0xff == 0; // Zero flag
    state->cc.s = answer & 0x80 != 0; // Sign flag
    state->cc.cy = answer > 0xff;     // Carry
    state->cc.p = Parity(answer & 0xff);
    state->a = answer & 0xff;
  default:   UnimplementedInstruction(state); break;
    // Notes:
    // ADC, ACI, SBB, SUI use the carry bit per the data book
    // INX and DCX do not affect the flags
    // DAD only affects the carry flag
    // INR and DCR do not affect the carry flag
  }
  state->pc += 1 // for the opcode
}
