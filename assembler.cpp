#include <iostream>
#include <fstream>
#include <map>
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
    std::vector<Token> tokens;

    this->pass_space();

    while( this->check() ) {
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
      else if( this->eat("fp") ) {
        token.kind = Token::Kind::Register;
        token.reg_index = 11;
      }

      // register alias
      else if( this->eat("ip") ) {
        token.kind = Token::Kind::Register;
        token.reg_index = 12;
      }

      // register alias
      else if( this->eat("sp") ) {
        token.kind = Token::Kind::Register;
        token.reg_index = 13;
      }

      // register alias
      else if( this->eat("lr") ) {
        token.kind = Token::Kind::Register;
        token.reg_index = 14;
      }

      // register alias
      else if( this->eat("pc") ) {
        token.kind = Token::Kind::Register;
        token.reg_index = 15;
      }

      // value
      else if( this->eat("#") ) {
        token.kind = Token::Kind::Value;

        if( auto&& [b, s] = this->eat_digits(); b ) {
          token.value = std::stoull(s);
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

  Assembler(std::string const& path)
    : source(open_text_file(path)),
      tokens(Lexer(this->source).lex()),
      iter(tokens.begin())
  {
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

  std::vector<Asm> assemb() {
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
      "bl",
      "b",
      "bx",
    };

    static constexpr auto get_inst_kind = [] (std::string const& name) -> std::optional<Asm::Kind> {
      for( size_t i = 0; i < std::size(instructions); i++ )
        if( instructions[i] == name )
          return static_cast<Asm::Kind>(i);

      return std::nullopt;
    };

    std::vector<Asm> ret;
    std::map<std::string, size_t> labels;

    using Tk = Token::Kind;

    while( iter != tokens.end() ) {
      // mov or cmp
      if( (iter->s == "mov" || iter->s == "cmp") ) {
        // rd, ra
        if( this->match({Tk::Ident, Tk::Register, ",", Tk::Register}) )
          ret.emplace_back(get_inst_kind(this->matched[0]->s).value(), this->matched[1]->reg_index, this->matched[3]->reg_index, 0);

        // rd, #value
        else if( this->match({Tk::Ident, Tk::Register, ",", Tk::Value}) )
          ret.emplace_back(get_inst_kind(this->matched[0]->s).value(), this->matched[1]->reg_index, 0, 0, this->matched[3]->value);
      }

      // op rd, ra, rb (op = calc)
      else if( this->match({Tk::Ident, Tk::Register, ",", Tk::Register, ",", Tk::Register}) ) {
        auto kind = get_inst_kind(this->matched[0]->s).value();

        if( Asm::Kind::Add <= kind && kind <= Asm::Kind::Rst ) {
          ret.emplace_back(kind, this->matched[1]->reg_index, this->matched[3]->reg_index, this->matched[5]->reg_index);
        }
        else
          goto __err;
      }

      // op ra, rb (op = calc)
      else if( this->match({Tk::Ident, Tk::Register, ",", Tk::Register}) ) {
        auto kind = get_inst_kind(this->matched[0]->s).value();

        if( Asm::Kind::Add <= kind && kind <= Asm::Kind::Rst ) {
          ret.emplace_back(kind, this->matched[1]->reg_index, this->matched[1]->reg_index, this->matched[3]->reg_index);
        }
        else
          goto __err;
      }

      // load or store
      else if( iter->s.length() >= 3 && (iter->s.starts_with("ldr") || iter->s.starts_with("str")) ) {
        auto& op = ret.emplace_back();

        op.kind = iter++->s == "ldr" ? Asm::Kind::Load : Asm::Kind::Store;

        if( iter->s.length() > 3 ) {
          switch( iter->s[3] ) {
            case 'u': op.data_type = Asm::DataType::Long; break;
            case 'h': op.data_type = Asm::DataType::Word; break;
            case 's': op.data_type = Asm::DataType::Harf; break;
            case 'b': op.data_type = Asm::DataType::Byte; break;

            default:
              Err("unknown data type of ldr/str");
          }
        }
        else {
          op.data_type = Asm::DataType::Long;
        }

        if( !this->match({Tk::Register, ",", "[", Tk::Register}) )
          goto __err;

        op.ra = this->matched[0]->reg_index;
        op.rb = this->matched[3]->reg_index;

        // offset
        if( this->match({",", Tk::Value}) ) {
          op.value = this->matched[1]->value;
        }

        if( iter++->s != "]" )
          goto __err;

        if( this->match({",", Tk::Value}) ) {
          op.rd = this->matched[1]->value & 0xFF;
        }
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

} // namespace metro