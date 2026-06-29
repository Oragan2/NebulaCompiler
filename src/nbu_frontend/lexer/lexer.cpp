#include "lexer.h"
#include <cctype>
#include <iostream>
#include <string>
#include <unordered_map>

namespace nbuFrontend {
  std::unordered_map<char, TokenType> SymboleTable{
      {'+', TokenType::PLUS},
      {'*', TokenType::STAR},
      {'-', TokenType::MINUS},
      {'/', TokenType::SLASH},
      {'%', TokenType::PERCENT},
      {'&', TokenType::AND},
      {'|', TokenType::OR},
      {'^', TokenType::XOR},
      {'~', TokenType::NOT},
      {'!', TokenType::EXCLAMATION},
      {'=', TokenType::EQUAL},
      {';', TokenType::SEMICOLON},
      {'<', TokenType::LT},
      {'>', TokenType::MT},
      {'(', TokenType::LPARAM},
      {')', TokenType::RPARAM},
      {'{', TokenType::LBRAK},
      {'}', TokenType::RBRAK},
      {',', TokenType::COMMA},
      {'.', TokenType::DOT}
  };

  std::unordered_map<std::string, TokenType> KeywordMap{
      {"return", TokenType::RETURN},
      {"<<", TokenType::SHIFTL},
      {">>", TokenType::SHIFTR},
      {"int32", TokenType::INT32},
      {"uint32", TokenType::UINT32},
      {"&&", TokenType::ANDAND},
      {"||", TokenType::OROR},
      {"^^", TokenType::XORXOR},
      {"<=", TokenType::LTE},
      {">=", TokenType::MTE},
      {"==", TokenType::EQUALEQUAL},
      {"!=", TokenType::DIFFERENT},
      {"if", TokenType::IF},
      {"else", TokenType::ELSE},
      {"float32", TokenType::FLOAT32},
      {"float64", TokenType::FLOAT64},
      {"int64", TokenType::INT64},
      {"uint64", TokenType::UINT64},
      {"vaddr", TokenType::VADDR},
      {"paddr", TokenType::PADDR},
      {"asm", TokenType::ASM},
      {"enum", TokenType::ENUM},
      {"struct", TokenType::STRUCT},
      {"::", TokenType::DOUBLEDOT}
  };

  std::ostream &operator<<(std::ostream &os, TokenType token) {
    os << token_to_str(token);
    return os;
  }

  std::string operator+(const std::string& str, TokenType token) {
    return str+token_to_str(token);
  }

  std::string token_to_str(TokenType token) {
    switch (token) {
    case TokenType::INT_SIGNED_32:
      return "valInt32";
    case TokenType::STAR:
      return "*";
    case TokenType::PLUS:
      return "+";
    case TokenType::RETURN:
      return "return";
    case TokenType::SEMICOLON:
      return ";";
    case TokenType::EOFTOKEN:
      return "EOF";
    case TokenType::EXCLAMATION:
      return "!";
    case TokenType::AND:
      return "&";
    case TokenType::MINUS:
      return "-";
    case TokenType::SLASH:
      return "/";
    case TokenType::PERCENT:
      return "%";
    case TokenType::OR:
      return "|";
    case TokenType::XOR:
      return "^";
    case TokenType::NOT:
      return "~";
    case TokenType::SHIFTL:
      return "<<";
    case TokenType::SHIFTR:
      return ">>";
    case TokenType::IDENTIFIER:
      return "identifier";
    case TokenType::INT32:
      return "int32";
    case TokenType::UINT32:
      return "uint32";
    case TokenType::EQUAL:
      return "=";
    case TokenType::EQUALEQUAL:
      return "==";
    case TokenType::DIFFERENT:
      return "!=";
    case TokenType::LT:
      return "<";
    case TokenType::MT:
      return ">";
    case TokenType::LTE:
      return "<=";
    case TokenType::MTE:
      return ">=";
    case TokenType::ANDAND:
      return "&&";
    case TokenType::OROR:
      return "||";
    case TokenType::XORXOR:
      return "^^";
    case TokenType::IF:
      return "if";
    case TokenType::ELSE:
      return "else";
    case TokenType::LPARAM:
      return "(";
    case TokenType::RPARAM:
      return ")";
    case TokenType::LBRAK:
      return "{";
    case TokenType::RBRAK:
      return "}";
    case TokenType::COMMA:
      return ",";
    case TokenType::FLOAT32:
      return "float32";
    case TokenType::FLOAT64:
      return "float64";
    case TokenType::INT64:
      return "int64";
    case TokenType::UINT64:
      return "uint64";
    case TokenType::FLOAT_SIGNED_32:
      return "valFloat32";
    case TokenType::VADDR:
      return "vaddr";
    case TokenType::PADDR:
      return "paddr";
    case TokenType::ASM:
      return "asm";
    case TokenType::ASM_INSTRUCTIONS:
      return "asm instructions";
    case TokenType::ENUM:
      return "enum";
    case TokenType::STRUCT:
      return "struct";
    case TokenType::DOT:
      return ".";
    case TokenType::DOUBLEDOT:
      return "::";
    default:
      return "TODO";
    }
  }

  std::vector<Token> lexer(std::ifstream &file) {
    std::vector<Token> tokens;
    std::string word;
    unsigned int column = 1;
    unsigned int line = 1;

    State state = State::START;
    char c;

    auto flush_token = [&]() {
      if (word.empty())
        return;
      if (state == State::TEXT) {
        if (KeywordMap.find(word) == KeywordMap.end())
          tokens.emplace_back(TokenType::IDENTIFIER,word,column,line);
        else
          tokens.emplace_back(KeywordMap[word],word,column,line);
      }
      else if (state == State::NUMBER) {
        if (word.find('.') > word.length())
          tokens.emplace_back(TokenType::INT_SIGNED_32,word,column,line);
        else
          tokens.emplace_back(TokenType::FLOAT_SIGNED_32,word,column,line);
      }
      column += word.length();
      word.clear();
      state = State::START;
    };

    auto handle_symbole = [&]() {
      char tmpc;
      if (file.get(tmpc)) {
        std::string tmp = std::string(1,c)+tmpc;
        if (KeywordMap.find(tmp) != KeywordMap.end()) {
          tokens.emplace_back(KeywordMap[tmp], tmp, column, line);
          column += 2;
          return;
        }
      }
      tokens.emplace_back(SymboleTable[c],std::string(1,c),column,line);
      ++column;
      file.unget();
    };

    while (file.get(c)) {
      if (c == '\n') {
        column = 1;
        line += 1;
      }
      switch (state) {
      case State::START:
        if (std::isspace(c)) {
          continue;
        }
        if (SymboleTable.find(c) != SymboleTable.end()) {
          handle_symbole();
          continue;
        }
        state = std::isdigit(c) ? State::NUMBER : State::TEXT;
        word += c;
        break;
      case State::NUMBER:
        if (std::isdigit(c) || c == '.')
          word += c;
        else {
          flush_token();

          if (SymboleTable.find(c) != SymboleTable.end())
            handle_symbole();
          else if (!std::isspace(c)) {
            state = State::TEXT;
            word += c;
          }
        }
        break;
      case State::TEXT:
        if (std::isalnum(c))
          word += c;
        else if (c == ':') {
          char next_c = file.get();
          if (c == next_c) {
            flush_token();
            tokens.emplace_back(TokenType::DOUBLEDOT, "::",column, line);
            if (SymboleTable.find(c) != SymboleTable.end())
              handle_symbole();
            else if (!std::isspace(c)) {
              state = std::isdigit(c) ? State::NUMBER : State::TEXT;
            }
          }
          else {
            file.unget();
          }
        }
        else if (word == "asm") {
          tokens.emplace_back(TokenType::ASM, word, column, line);
          column += 3;
          word.clear();

          char next_c;
          while (file.get(next_c) && std::isspace(next_c)) 
            if (next_c == '\n') {column = 1; line++;} else {column++;}
          
          if (next_c == '{') {
            tokens.emplace_back(TokenType::LBRAK, "{", column++, line);

            std::string raw_asm;
            while (file.get(next_c) && next_c != '}') {
              raw_asm += next_c;
              if (next_c == '\n') {column = 1; line++;} else {column++;};
            }

            tokens.emplace_back(TokenType::ASM_INSTRUCTIONS, raw_asm, column, line);
            tokens.emplace_back(TokenType::RBRAK, "}", column, line);

            state = State::START;
            continue;
          }
        }
        else {
          flush_token();
          if (SymboleTable.find(c) != SymboleTable.end())
            handle_symbole();
          else if (!std::isspace(c)) {
            state = std::isdigit(c) ? State::NUMBER : State::TEXT;
            word += c;
          }
        }
        break;
      }
    }

    flush_token();

    tokens.emplace_back(TokenType::EOFTOKEN,"\0",column,line);
    
    return tokens;
  }
}