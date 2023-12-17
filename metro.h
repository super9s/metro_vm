#pragma once

#include <cstdint>
#include <string>
#include <vector>

#define  GETMASK(T)  (~((uint64_t)-1 << sizeof(T)))

typedef int8_t    i8;
typedef int16_t   i16;
typedef int32_t   i32;
typedef int64_t   i64;

typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;

namespace metro::vm {

/*
 *  An assembly instruction for virtual machine of metro.
 *  metro のバーチャルマシン向けのアセンブリを表現する構造体です
 * 
 *  ストア・ロード命令向けビット幅指定記号:
 *    u   = 64bit  // default
 *    h   = 32bit
 *    s   = 16bit
 *    b   = 8bit
 * 
 */
struct Asm {
  enum class Kind {
    Mov,        // mov    rd, ra
    Cmp,

    Add,        // add    rDest, rA, rB   @ rDest = rA + rB
    Sub,        // sub    rDest, rA, rB   @ rDest = rA - rB
    Mul,        // mul    rDest, rA, rB   @ rDest = rA * rB
    Div,        // div    rDest, rA, rB   @ rDest = rA / rB
    Mod,        // mod    rDest, rA, rB   @ rDest = rA % rB

    // shift
    Lst,        // lst    rDest, rA, rB   @ rDest = rA << rB
    Rst,        // rst    rDest, rA, rB   @ rDest = rA >> rB

    /*
     * load / store
     *   rA   = .ra
     *   rB   = .rb
     *   offs = .value
     *   N    = .rd
     */
    Load,       // ldr    rA, [rB, #offs], #N  @ rA = *(rB + offs); rB += N
    Store,      // str    rA, [rB, #offs], #N  @ *(rB + offs) = rA; rB += N

    /* push / pop */
    Push,       // push   rA
    Pop,        // pop    rA

    /* Branch */
    Call,       // bl   <label>
    Jump,       // b    <label>
    Jumpx,      // bx   rA

    /*
     * Place a data.
     *
     * 
     */
    Data,


  };

  enum class DataType {
    Byte,     // u8
    Harf,     // u16
    Word,     // u32
    Long,     // u64
    String,   // string literal of UTF-16
  };


  Kind    kind;
  u8      rd;
  u8      ra;
  u8      rb;
  u64     value;  // or label
  bool    with_value;
  u8      width;

  DataType  data_type;
  void*     data;  // uintN_t or char16_t*

  Asm(Kind kind, u8 rd, u8 ra, u8 rb)
    : kind(kind),
      rd(rd),
      ra(ra),
      rb(rb),
      value(0),
      with_value(false),
      width(0),
      data_type(DataType::Byte),
      data(nullptr)
  {
  }

  Asm()
    : Asm(Kind::Mov, 0, 0, 0)
  {
  }

  Asm(Kind kind, u8 rd, u8 ra, u8 rb, u64 value, u8 width = 64)
    : Asm(kind, rd, ra, rb)
  {
    this->with_value = true;
    this->value = value;
    this->width = width;
  }

};

struct VCPU {
  union {
    u64   registers[16] { };

    struct {
      u64   ra[4];
      u64   rv[8];
      u64   ip;
      u64*  sp;
      u64   lr;
      u64   pc;
    };
  };

  VCPU()
  {
  }
};

class Machine {
public:

  Machine()
  {
  }

  ~Machine()
  {
  }

  /*
   * execute asm operations.
   */
  void execute_code(std::vector<Asm> const& codes);


//private:

  VCPU cpu;

  u64 stack[0x1000];



};


bool assemble_from_file(std::string const& path);


} // namespace metro::vm

