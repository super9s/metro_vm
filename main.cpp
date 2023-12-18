#include "metro.h"

using namespace metro;

void test() {

  using namespace metro::vm;
  
  u64  val = 0;

  std::vector<Asm> codes = {
    Asm(Asm::Kind::Mov, 3, 3, 0, 0x12345678),   // mov   r3, #0x1234
    Asm(Asm::Kind::Add, 3, 3, 3),               // add   r3, r3, r3

    Asm(Asm::Kind::Mov, 0, 0, 0, (u64)&val),  // mov   r0, &val
    Asm(Asm::Kind::Store, 0, 3, 0, 0),    // strs  r3, [r0, #0], #0
    Asm(Asm::Kind::Load, 0, 1, 0, 0),      // ldrb  r1, [r0, #0], #0
  };

  Machine machine;

  machine.execute_code(codes);

  for( int i = 0; i < 16; i += 2 ) {
    printf("r%d%s  %016zX   r%d%s  %016zX\n",
      i, i < 10 ? " " : "", machine.cpu.registers[i],
      i + 1, i + 1 < 10 ? " " : "", machine.cpu.registers[i + 1]);
  }

  puts("");
  printf("val = 0x%zx\n", val);
  
}

int main(int argc, char** argv) {
  using namespace metro::vm;

  auto codes = assembler::assemble_from_file("test.txt");

  Machine machine;

  machine.execute_code(codes);

  for( int i = 0; i < 16; i += 2 ) {
    printf("r%d%s  %016zX   r%d%s  %016zX\n",
      i, i < 10 ? " " : "", machine.cpu.registers[i],
      i + 1, i + 1 < 10 ? " " : "", machine.cpu.registers[i + 1]);
  }
  
  for(int i=0;i<10;i++){
    printf("stack %p: %016zX\n", machine.stack + i, machine.stack[i]);
  }

}

