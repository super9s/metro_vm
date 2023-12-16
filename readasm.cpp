#include <iostream>
#include <fstream>
#include "metro.h"

std::string open_text_file(std::string const& path) {
  std::ifstream ifs{ path };
std::string ret,line;

  if(ifs.fail()) {
    std::cout<<"cannot open file '"<<path<<"'"<<std::endl;
    std::exit(1);
  }

while(std::getline(ifs,line))
ret+=line+"\n";

return ret;

}

struct Token {
  enum Kind {
    Unknown,
    Ident,
    Register, // r0 ~ r15 or ip, fp, sp, lr, pc
    Digits,
    Char,     //
    String,   // UTF-16
    Label,
    Attribute, // .data とかのやつ
  };

  Kind kind;
  std::string str;
  size_t pos;

  std::u16string val_str;

  union {
    u16  val_int;
    char16_t  val_char;
  };

  Token(Kind kind, std::string const& s = "")
    : kind(kind),
      str(s),
      pos(0),
      val_int(0)
  {
  }
};

class c_lexer {
  std::string const& source;
  size_t position;
  size_t const length;

public:
  c_lexer(std::string const& source)
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
    return this->position + s.length() <= this->length && this->source.substr(this->position, s.length()) == s;
  }

  void pass_space() {
    while( this->check() && isspace(this->peek()) )
      this->position++;
  }

  std::vector<Token> lex() {
    std::vector<Token> ret;

    this->pass_space();

    while( this->check() ) {
      auto& tok = ret.emplace_back(Token::Unknown);
      auto s = this->source.data() + this->position;
      size_t pos = this->position;

      // hex
      if( this->match("0x") ) {
        tok.kind = Token::Digits;
        tok.val_int = std::stoi(s, &pos, 16);
        this->position = pos;
      }

      // dec
      else if( isdigit(this->peek()) ) {
        tok.kind = Token::Digits;
        tok.val_int = std::stoi(s, &pos);
        this->position = pos;
      }

      // identifier
      else if( isalpha(this->peek()) ) {
      __ident:
        tok.kind = Token::Ident;
        
        while( isalnum(this->peek()) || this->peek() == '_' )
          this->position++;

        tok.str = { s + pos, s + this->position };

        if( this->peek() == ':' ) {
          tok.kind = Token::Label;
          this->position++;
        }
      }
    }

    return ret;
  }
};

/*
 *  make_codes_from_file():
 *
 *  テキストファイルを読み込んでアセンブルする
 *  成功したら 0 を返す
 *  
 * phases
 *  1. ソース読み込み
 *  2. 字句解析
 *  3. アセンブル
 */
int make_codes_from_file(std::vector<Asm>& out, std::string const& path) {

  std::vector<std::string> tokens;

  auto source= open_text_file(path);

  // 空のソース => ダメ
  if(source.empty()){ 
    return 1;
  } 

  /* 空白、改行文字で分割する 
   Python の split(' ') で各部品に strip() したやつってかんじ*/
std::string tok;
  for(size_t pos=0;pos<source.length();){
    // 空白飛ばす
    while( source[pos] == ' ' )
      pos++;

    // 改行
    if( source[pos] == '\n' )
      tokens.emplace_back("\n");
    
    // 改行でなければ =>  読みとって tokens に追加
    else {
      while(!isspace(source[pos]))
        tok += source[pos++];
      
      tokens.emplace_back(tok);
      tok.clear();
    }
  }

// めんどうだから正しいこと前提で書く; try 使います
try{
  /*
   アセンブル? する*/
  for(auto tok=tokens.begin();tok!=tokens.end();){
    auto& op = out.emplace_back();

    std::vector<std::string> line; // 一行ずつ読み取る

    while( *tok != "\n" ) {
      line.emplace_back(*tok++);
    }

    // 空行は無視
    if( line.empty() ) {
      continue;
    }

    auto const& inst = *line.begin();
    auto it = line.begin() + 1;

    // ra, rb
    // ra, <value>
    if( inst == "mov" ) {

    }



    // 知らない命令もしくは無効な文字  => 意味わかんないのでエラー
    return 2;

  }

}catch(...){ // なんか起きたw
  return 999;
}

  return 0; // ok

}


