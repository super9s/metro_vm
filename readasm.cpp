#include <iostream>
#include <fstream>
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
    size_t pos = 0;

    while( this->check() && (isalnum(this->peek()) || this->peek() == '_') )
      this->position++;

    return this->source.substr(pos, this->position - pos);
  }

  void pass_space() {
    while( this->peek() == ' ' )
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

      // value
      else if( this->eat("#") ) {
        token.kind = Token::Kind::Value;

        if( auto&& [b, s] = this->eat_digits(); b ) {
          token.value = std::stoull(s);
        }
        else
          Err("expected digits after '#'");
      }

      // identigier
      else if( this->peek() == '_' || isalnum(this->peek()) ) {
        token.kind = Token::Kind::Ident;
        token.s = this->eat_ident();
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
    std::cout << "metro.assembler: cannot open file '" << path << "'" << std::endl;
    std::exit(1);
  }

  std::string line;

  while( std::getline(ifs, line) )
    ret += line + '\n';

  return ret;
}

class Assembler {

public:

  struct Pattern {
    enum class Type {
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

    Pattern(std::string_view s)
      : type(Type::String),
        k(Token::Kind::Unknown),
        s(s)
    {
    }
  };

  std::string source;
  std::vector<Token> tokens;
  std::vector<Token>::iterator iter;

  Assembler(std::string const& path)
    : source(open_text_file(path)),
      tokens(Lexer(this->source).lex()),
      iter(tokens.begin())
  {
  }

  bool match(std::vector<Pattern> patterns) {
    auto it = this->iter;

    for( auto&& p : patterns ) {
      if( p.type == Pattern::Type::TokenKind && it++->kind != p.k )
        return false;
      else if( p.type == Pattern::Type::String && it++->s != p.s )
        return false;
    }

    this->iter = it;
    return true;
  }

  std::vector<Asm> assemb() {
    std::vector<Asm> ret;

    while( iter != tokens.end() ) {
      
    }

    return ret;
  }
};

std::vector<Asm> assemble_from_file(std::string const& path) {
  return Assembler(path).assemb();
}

} // namespace metro