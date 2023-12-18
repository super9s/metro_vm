#include <iostream>
#include <cstring>
#include "metro.h"

namespace metro::vm {

void Machine::execute_code(std::vector<Asm> const& codes) {

  cpu.sp = this->stack;

  for( cpu.pc = 0; cpu.pc < codes.size(); ) {
    auto const& op = codes[cpu.pc];

    switch( op.kind ) {
      case Asm::Kind::Mov:
        if( op.with_value )
          cpu.registers[op.rd] = op.value;
        else
          cpu.registers[op.rd] = cpu.registers[op.ra];

        break;

      case Asm::Kind::Add:
        if( op.with_value )
          cpu.registers[op.rd] += op.value;
        else
          cpu.registers[op.rd] = cpu.registers[op.ra] + cpu.registers[op.rb];

        break;

      case Asm::Kind::Load:
        cpu.registers[op.ra] = *(u64*)(cpu.registers[op.rb] + op.value) & ~(-1UL << (static_cast<int>(op.data_type) * 8));
        cpu.registers[op.rb] += op.rd;
        break;

      case Asm::Kind::Store:
        if( op.data_type == Asm::DataType::Byte )
          *(u8*)(cpu.registers[op.rb] + op.value) = cpu.registers[op.ra] & 0xFF;
        else if( op.data_type == Asm::DataType::Harf )
          *(u16*)(cpu.registers[op.rb] + op.value) = cpu.registers[op.ra] & 0xFFFF;
        else if( op.data_type == Asm::DataType::Word )
          *(u32*)(cpu.registers[op.rb] + op.value) = cpu.registers[op.ra] & 0xFFFFFFFF;
        else if( op.data_type == Asm::DataType::Long )
          *(u64*)(cpu.registers[op.rb] + op.value) = cpu.registers[op.ra];

        cpu.registers[op.rb] += op.rd;
        break;

      case Asm::Kind::Push:
        *cpu.sp++ = cpu.registers[op.ra];
        break;

      case Asm::Kind::Pop:
        cpu.registers[op.ra] = *cpu.sp--;
        break;

      case Asm::Kind::Call:
        cpu.lr = cpu.pc + 1;

      case Asm::Kind::Jump:
        cpu.pc = op.value;
        break;

      case Asm::Kind::Jumpx:
        cpu.pc = cpu.registers[op.ra];
        break;

      /* ignore */
      case Asm::Kind::Data:
        break;
    }

    cpu.pc++;
  }



}

} // namespace metro::vm