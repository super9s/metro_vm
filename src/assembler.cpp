#include <iostream>
#include <fstream>
#include <map>
#include <codecvt>
#include <locale>
#include <optional>
#include "metro.h"

namespace metro::assembler {

using namespace metro::vm;

[[noreturn]]
static void Err(std::string const& msg) {
  std::cout << msg << std::endl;
  std::exit(1);
}

struct Token {
  enum class Kind {
    Unknown,
    Ident,
    Register,
    Value,
    String,
    Punctuater,
  };

  Kind kind;
  std::string s;

  union {
    u64 value;
    u8 reg_index;
  };

  Token(Kind kind = Kind::Unknown)
    : kind(kind),
      value(0)
  {
  }
};

class Lexer {
  std::string source;
  size_t position;
  size_t const length;

public:
  Lexer(std::string const& source)
    : source(source),
      position(0),
      length(source.length())
  {
  }

  bool check() const {
    return this->position < this->length;
  }

  char peek() const {
    return this->source[this->position];
  }

  bool match(std::string_view s) const {
    return
      this->position + s.length() <= this->length
      && this->source.substr(this->position, s.length()) == s;
  }

  bool eat(std::string_view s) {
    if( this->match(s) ) {
      this->position += s.length();
      return true;
    }

    return false;
  }

  std::tuple<bool, std::string> eat_digits(int const base = 10) {
    size_t pos = this->position;

    while( this->check() ) {
      if( isdigit(this->peek()) || (base == 16 && isxdigit(this->peek())) )
        this->position++;
      else
        break;
    }

    return { pos != this->position, this->source.substr(pos, this->position - pos) };
  }

  std::string eat_ident() {
    size_t pos = this->position;

    while( this->check() && (isalnum(this->peek()) || this->peek() == '_') )
      this->position++;

    return this->source.substr(pos, this->position - pos);
  }

  void pass_space() {
    while( isspace(this->peek()) )
      this->position++;
  }

  std::vector<Token> lex() {
    static constexpr std::pair<char const*, int> register_aliases[] = {
      { "fp", 11 },
      { "ip", 12 },
      { "sp", 13 },
      { "lr", 14 },
      { "pc", 15 },
    };

    std::vector<Token> tokens;

    this->pass_space();

    while( this->check() ) {
      // comment
      if( this->eat("@") ) {
        while( this->check() && this->peek() != '\n' )
          this->position++;

        this->pass_space();
        continue;
      }

      auto& token = tokens.emplace_back();

      // register
      if( this->eat("r") ) {
        token.kind = Token::Kind::Register;
        
        if( auto&& [b, s] = this->eat_digits(); b ) {
          token.s = 'r' + s;

          int r = std::stoi(s);

          if( r < 0 || r >= 16 ) {
            Err("invalid register index");
          }

          token.reg_index = r & 0xFF;
        }
        else
          Err("expected register index after 'r'");
      }

      // register alias
      else if( auto [b, i] = std::make_tuple(false, 0); ({for( auto&& [x, y] : register_aliases ) if( this->eat(x) ) { b = true; i = y; break; } }), b ) {
        token.kind = Token::Kind::Register;
        token.reg_index = i;
      }

      // value (char)
      else if( this->eat("#'") ) {
        token.kind = Token::Kind::Value;
        token.value = this->peek();

        this->position++;
        if( !this->eat("'") ) {
          Err("unclosed");
        }
      }

      // value
      else if( this->eat("#") ) {
        token.kind = Token::Kind::Value;
        int base = this->eat("0x") ? 16 : 10;

        if( auto&& [b, s] = this->eat_digits(base); b ) {
          token.value = std::stoull(s, nullptr, base);
          token.s = "#" + s;
        }
        else
          Err("expected digits after '#'");
      }

      // identigier
      else if( this->peek() == '_' || isalnum(this->peek()) ) {
        token.kind = Token::Kind::Ident;
        token.s = this->eat_ident();
      }

      // string
      else if( this->eat("\"") ) {
        token.kind = Token::Kind::String;
        auto pos = ++this->position;

        while( this->check() && !this->eat("\"") )
          this->position++;
        
        token.s = this->source.substr(pos, this->position - pos - 1);
      }

      else {
        token.kind = Token::Kind::Punctuater;
        token.s = this->peek();
        this->position++;
      }

      this->pass_space();
    }

    return tokens;
  }
};

std::string open_text_file(std::string const& path) {
  std::ifstream ifs{ path };
  std::string ret;

  if( ifs.fail() ) {
    Err("metro.assembler: cannot open file '" + path + "'");
  }

  std::string line;

  while( std::getline(ifs, line) )
    ret += line + '\n';

  return ret;
}

class Assembler {

public:

  struct Pattern {
    enum Type {
      TokenKind,
      String
    };

    Type type;
    Token::Kind k;
    std::string_view s;

    Pattern(Token::Kind k)
      : type(Type::TokenKind),
        k(k)
    {
    }

    Pattern(char const* s)
      : type(Type::String),
        k(Token::Kind::Unknown),
        s(s)
    {
    }
  };

  std::string source;
  std::vector<Token> tokens;
  std::vector<Token>::iterator iter;

  std::vector<std::vector<Token>::iterator> matched;

  std::map<std::string, size_t> labels;

  Assembler(std::string const& path)
    : source(open_text_file(path)),
      tokens(Lexer(this->source).lex()),
      iter(tokens.begin())
  {
    // for(auto&&t:tokens)std::cout<<t.s<<std::endl;
  }

  bool match(std::vector<Pattern> const& patterns) {
    auto it = this->iter;

    this->matched.clear();

    for( auto&& p : patterns ) {
      this->matched.emplace_back(it);

      if( p.type == Pattern::Type::TokenKind && it++->kind != p.k )
        return false;
      else if( p.type == Pattern::Type::String && it++->s != p.s )
        return false;
    }

    this->iter = it;
    return true;
  }

  bool eat(std::string const& s) {
    if( this->iter->s == s ) {
      this->iter++;
      return true;
    }

    return false;
  }

  void expect(std::string const& s) {
    if( !this->eat(s) )
      Err("expected '" + s + "'");
  }

  std::vector<Asm> assemb() {
    using Tk = Token::Kind;

    static constexpr char const* instructions[] = {
      "mov",
      "cmp",
      "add",
      "sub",
      "mul",
      "div",
      "mod",
      "lst",
      "rst",
      "ldr",
      "str",
      "push",
      "pop",
      "call",
      "jmp",
      "jmpx",
    };

    static constexpr auto get_inst_kind = [] (std::string const& name) -> std::optional<Asm::Kind> {
      for( size_t i = 0; i < std::size(instructions); i++ )
        if( instructions[i] == name )
          return static_cast<Asm::Kind>(i);

      return std::nullopt;
    };

    std::vector<Asm> ret;
    auto& M = this->matched;

    while( this->iter != tokens.end() ) {

      // label
      if( this->match({Tk::Ident, ":"}) ) {
        ret.emplace_back(Asm::Kind::Label).str = M[0]->s;
      }

      // data
      else if( this->match({".", Tk::Ident}) ) {
        static char const* dtypes[] = {
          "byte",
          "harf",
          "word",
          "long",
          "string",
        };

        auto& op = ret.emplace_back(Asm::Kind::Data, 0, 0, 0);

        for( size_t i = 0; i < std::size(dtypes); i++ ) {
          if( M[1]->s == dtypes[i] ) {
            op.data_type = static_cast<Asm::DataType>(i);
            goto _found;
          }
        }

        if( op.data_type == Asm::DataType::String ) {
          std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> conv;

          if( this->iter->kind != Tk::String ) {
            Err("expected string literal");
          }

          auto str = conv.from_bytes(this->iter++->s);

          auto data = new char16_t[str.length() + 1];

          memcpy(data, str.data(), str.size());

          op.data = (void*)data;
        }
        else {
          if( this->iter->kind != Tk::Value ) {
            Err("expected digits");
          }

          op.value = this->iter++->value;

          // check data size
          if( op.data_type == Asm::DataType::Byte && op.value <= 0xFF );
          else if( op.data_type == Asm::DataType::Harf && op.value <= 0xFFFF );
          else if( op.data_type == Asm::DataType::Word && op.value <= 0xFFFFFFFF );
          else if( op.data_type == Asm::DataType::Long && op.value <= 0xFFFFFFFFFFFFFFFF );
          else
            Err("overflow");
        }

        Err("unknown data type '" + M[1]->s + "'");
      _found:;
      }

      // call
      else if( this->match({"call", Tk::Ident}) ) {
        ret.emplace_back(Asm::Kind::Call).str = M[1]->s;
      }

      // jmp
      else if( this->match({"jmp", Tk::Ident}) ) {
        ret.emplace_back(Asm::Kind::Jump).str = M[1]->s;
      }

      // jx
      else if( this->match({"jx", Tk::Register}) ) {
        ret.emplace_back(Asm::Kind::Jumpx).ra = M[1]->reg_index;
      }

      // syscall
      else if( this->match({"sys", Tk::Value}) ) {
        ret.emplace_back(Asm::Kind::SysCall).value = M[1]->value;
      }

      /*
       * op rd, ra
       *    rd, #value
       *    rd, ra, rb
       *    rd, ra, #value
       */
      else if( auto k = get_inst_kind(this->iter->s); k && k.value() <= Asm::Kind::Rst ) {
        this->iter++;

        // rd
        if( !this->match({Tk::Register, ","}) )
          goto __err;

        auto& op = ret.emplace_back(k.value(), M[0]->reg_index, M[0]->reg_index, 0);

        // ra, rb
        if( this->match({Tk::Register, ",", Tk::Register}) ) {
          op.ra = M[0]->reg_index;
          op.rb = M[2]->reg_index;
        }

        // ra, #value
        else if( this->match({Tk::Register, ",", Tk::Value}) ) {
          op.ra = M[0]->reg_index;
          op.value = M[2]->value;
          op.with_value = true;
        }

        // #value
        else if( this->match({Tk::Value}) ) {
          op.value = M[0]->value;
          op.with_value = true;
        }

        // ra
        else if( this->match({Tk::Register}) ) {
          op.ra = M[0]->reg_index;
        }
        else
          goto __err;
      }

      // load or store
      else if( this->iter->s.length() >= 3 && (this->iter->s.starts_with("ldr") || this->iter->s.starts_with("str")) ) {
        auto& op = ret.emplace_back();

        op.kind = this->iter->s.starts_with("ldr") ? Asm::Kind::Load : Asm::Kind::Store;

        if( this->iter->s.length() > 3 ) {
          switch( this->iter->s[3] ) {
            case 'u': op.data_type = Asm::DataType::Long; break;
            case 'w': op.data_type = Asm::DataType::Word; break;
            case 'h': op.data_type = Asm::DataType::Harf; break;
            case 'b': op.data_type = Asm::DataType::Byte; break;

            default:
              Err("'" + this->iter->s.substr(3) + "' is not a data type of ldr/str");
          }
        }
        else {
          op.data_type = Asm::DataType::Long;
        }

        if( !this->match({Tk::Ident, Tk::Register, ",", "[", Tk::Register}) )
          goto __err;

        op.ra = M[1]->reg_index;
        op.rb = M[4]->reg_index;

        // offset
        if( this->match({",", Tk::Value}) ) {
          op.value = M[1]->value;
        }

        if( this->iter++->s != "]" )
          goto __err;

        if( this->match({",", Tk::Value}) ) {
          op.rd = M[1]->value & 0xFF;
        }
      }

      // push / pop
      else if( this->iter->s == "push" || this->iter->s == "pop" ) {
        auto& op = ret.emplace_back(
          this->iter++->s == "push" ? Asm::Kind::Push : Asm::Kind::Pop, 0, 0, 0);

        this->expect("{");

        do {
          if( this->match({Tk::Register, "-", Tk::Register}) ) {
            auto begin = M[0]->reg_index;
            auto end = M[2]->reg_index;

            if( begin >= end )
              goto __err;
  
            while( begin < end )
              op.reglist |= 1 << (begin++);

            continue;
          }

          if( this->iter->kind == Tk::Register )
            op.reglist |= 1 << this->iter++->reg_index;
          else
            goto __err;
        } while( this->eat(",") );

        this->expect("}");
      }

      else {
      __err:
        Err("invalid syntax");
      }
    }

    return ret;
  }
};

std::vector<Asm> assemble_from_file(std::string const& path) {
  return Assembler(path).assemb();
}

bool assemble_full(std::vector<u8>& out, std::vector<vm::Asm> const& codes) {

  // todo

  return false;
}

} // namespace metro::assembler