#include <stdio.h>
#include <stdlib.h>

/**
 * unsigned char *codebuffer - this is just a pointer to a string
 * int pc - index into the string
 */
int Disassemble8080Op(unsigned char *codebuffer, int pc) {
  unsigned char *code = &codebuffer[pc];
  int opbytes = 1;
  // x for lowercase hex
  // 04 for 4 width, left padded with 0
  printf("%04x ", pc);
  // $ means hex
  // # is a literal number
  switch (*code) {
  case 0x00:
    printf("NOP");
    break;
  case 0x01:
    printf("LXI    B,#$%02x%02x", code[2], code[1]);
    opbytes = 3;
    break;
  case 0x02:
    printf("STAX   B");
    break;
  case 0x03:
    printf("INX    B");
    break;
  case 0x04:
    printf("INR    B");
    break;
  case 0x05:
    printf("DCR    B");
    break;
  case 0x06:
    printf("MVI    B,#$%02x", code[1]);
    opbytes = 2;
    break;
  default:
    printf("?");
    break;
  }

  printf("\n");
  return opbytes;
}

/**
 * int argc - the # of args passed into the program, always at least 1, since
 * the first arg is the call to the program itself
 *
 * char **argv - pointer to a char *, since C-style strings are just char *
 * arrays, this could be also written as char *argv[] (array of char *)
 *
 */
int main(int argc, char **argv) {
  // "rb" means "read binary"
  FILE *f= fopen(argv[1], "rb");
  if (f == NULL) {
    printf("error: Couldn't open %s\n", argv[1]);
    exit(1);
  }

  // Get the file size and read it into a memory buffer
  // fseek sets the file position of the stream
  // int fseek(FILE *stream, long int offset, int whence)
  fseek(f, 0L, SEEK_END); // Sets stream to end of file
  int fsize = ftell(f); // returns the current file position
  fseek(f, 0L, SEEK_SET); // Sets stream back to the beginning

  // Allocate enough for the entire file
  unsigned char *buffer=malloc(fsize);

  // fread is for binary i/o
  // fread(
  //   void *ptr,
  //   size_t size_of_elements,
  //   size_t number_of_elements,
  //   FILE *a_file
  // )
  // Basically to read the whole file, it's:
  // fread(buffer, MAX_FILE_SIZE, 1, f)
  fread(buffer, fsize, 1, f);
  fclose(f);

  int pc = 0;

  while (pc < fsize) {
    // Returns how many instructions to increment by
    pc += Disassemble8080Op(buffer, pc);
  }

  return 0;
}
