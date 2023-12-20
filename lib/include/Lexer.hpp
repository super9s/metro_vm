// 
// 字句解析用汎用ライブラリ
// (C) 2023 Aoki
// 
// 概要:
//   トークンの種類と情報を複数個保有し、ソースコード内で一致する物を全て収集する
//
// 
// 

#pragma once

#include <functional>
#include <string>

namespace metro {

/*
 * TokenInfo:
 *   Lexer で識別するトークンの種類に関する情報
 */
struct TokenInfo {
  /*
   * TokenKind:
   *   一般的に使用されるようなトークンの種類
   *   自作のものを使用する場合、Custom を使う
   */
  enum class TokenKind {
    Decimal,      // 数字
    Hexadecimal,  // 数字 16 進数
    Binary,       // 数字 2 進数
    Identifier,   // 識別子
    Char,         // 文字リテラル
    String,       // 文字列リテラル
    Punctuater,   // 記号
    Custom        // 自作
  };

  /*
   * Locations:
   *   文字の位置によって有効判定を変える際使用する列挙型
   */
  enum class Locations {
    Default,    // 通常
    Begin,      // 最初
    Last,       // 最後
    Middle,     // 最初より後、最後より手前
  };

  /*
   * CharacterChecker:
   *   トークンに使用される文字として有効かどうか判定する関数へのポインタ
   */
  using CharacterChecker =
    std::function<bool(TokenInfo&, char)>;


  TokenKind kind;
  CharacterChecker  isValid;

  TokenInfo()
  {
  }

};

class Lexer {
public:



private:
};

} // namespace metro