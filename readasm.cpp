#include "metro.h"

namespace metro::vm {

struct Token {
  enum class Kind {
    Unknown,
    Ident,
    Register,
    
  };

  Kind kind;

  Token(Kind kind = Kind::Unknown)
    : kind(kind)
  {
  }
};

class Lexer {
  std::string const& source;
  size_t position;
  size_t const length;

public:
  Lexer(std::string& source)
    : source(source),
      position(0),
      length(0)
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
      if( this->match("r") ) {
        
      }


      this->pass_space();
    }

    return tokens;
  }
};


template <class T, class... Types>
bool match(std::vector<Token>::iterator& iter, std::initializer_list<Types...> il) {
}

/*

// ra, value
if( match(it) ) {
  ...
}

*/


bool assemble_from_file(std::string const& path) {
  


}

} // namespace metro::vm