#include <iostream>
#include <cstring>
#include "metro.h"

namespace metro::vm {

void Machine::execute_code(std::vector<Asm> const& codes) {

  cpu.sp = this->stack;
  cpu.lr = (u64)-1;

  for( cpu.pc = 0; cpu.pc != (u64)-1 && cpu.pc < codes.size(); ) {
    auto const& op = codes[cpu.pc];

    switch( op.kind ) {
      case Asm::Kind::Mov:
        if( op.with_value )
          cpu.registers[op.rd] = op.value;
        else
          cpu.registers[op.rd] = cpu.registers[op.ra];

        break;

      case Asm::Kind::Cmp: {

        

        break;
      }

      case Asm::Kind::Add:
        if( op.with_value )
          cpu.registers[op.rd] = cpu.registers[op.ra] + op.value;
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

      case Asm::Kind::Push: {
        for( int i = 15; i >= 0; i-- ) {
          if( op.reglist & (1 << i) )
            *cpu.sp++ = cpu.registers[i];
        }

        break;
      }

      case Asm::Kind::Pop: {
        for( int i = 0; i < 16; i++ ) {
          if( op.reglist & (1 << i) )
            cpu.registers[i] = *--cpu.sp;
        }

        break;
      }

      case Asm::Kind::Call:
        cpu.lr = cpu.pc + 1;

      case Asm::Kind::Jump: {
        for( size_t i = 0; i < codes.size(); i++ ) {
          if( codes[i].kind == Asm::Kind::Label && codes[i].str == op.str ) {
            cpu.pc = i;
            goto xxx;
          }
        }

        std::cout << "undefined label name '" << op.str << "'" << std::endl;
        std::exit(1);

      xxx:;
        break;
      }

      case Asm::Kind::Jumpx:
        cpu.pc = cpu.registers[op.ra];
        break;

      /* ignore */
      case Asm::Kind::Data:
      case Asm::Kind::Label:
        break;
    }

    cpu.pc++;
  }



}

} // namespace metro::vm