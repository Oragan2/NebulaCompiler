#include "lexer.h"
#include <cctype>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

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
    {',', TokenType::COMMA}
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
    {"func", TokenType::FUNC}
};

std::ostream &operator<<(std::ostream &os, TokenType token) {
  switch (token) {
  case TokenType::INT_SIGNED_32:
    os << "valInt32";
    break;
  case TokenType::STAR:
    os << "*";
    break;
  case TokenType::PLUS:
    os << "+";
    break;
  case TokenType::RETURN:
    os << "return";
    break;
  case TokenType::SEMICOLON:
    os << ";";
    break;
  case TokenType::EOFTOKEN:
    os << "EOF";
    break;
  case TokenType::EXCLAMATION:
    os << "!";
    break;
  case TokenType::AND:
    os << "&";
    break;
  case TokenType::MINUS:
    os << "-";
    break;
  case TokenType::SLASH:
    os << "/";
    break;
  case TokenType::PERCENT:
    os << "%";
    break;
  case TokenType::OR:
    os << "|";
    break;
  case TokenType::XOR:
    os << "^";
    break;
  case TokenType::NOT:
    os << "~";
    break;
  case TokenType::SHIFTL:
    os << "<<";
    break;
  case TokenType::SHIFTR:
    os << ">>";
    break;
  case TokenType::IDENTIFIER:
    os << "identifier";
    break;
  case TokenType::INT32:
    os << "int32";
    break;
  case TokenType::UINT32:
    os << "uint32";
    break;
  case TokenType::EQUAL:
    os << "=";
    break;
  case TokenType::EQUALEQUAL:
    os << "==";
    break;
  case TokenType::DIFFERENT:
    os << "!=";
    break;
  case TokenType::LT:
    os << "<";
    break;
  case TokenType::MT:
    os << ">";
    break;
  case TokenType::LTE:
    os << "<=";
    break;
  case TokenType::MTE:
    os << ">=";
    break;
  case TokenType::ANDAND:
    os << "&&";
    break;
  case TokenType::OROR:
    os << "||";
    break;
  case TokenType::XORXOR:
    os << "^^";
    break;
  case TokenType::IF:
    os << "if";
    break;
  case TokenType::ELSE:
    os << "else";
    break;
  case TokenType::LPARAM:
    os << "(";
    break;
  case TokenType::RPARAM:
    os << ")";
    break;
  case TokenType::LBRAK:
    os << "{";
    break;
  case TokenType::RBRAK:
    os << "}";
    break;
  case TokenType::FUNC:
    os << "func";
    break;
  case TokenType::COMMA:
    os << ",";
    break;
  default:
    os << "TODO";
    break;
  }
  return os;
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
    else if (state == State::NUMBER) 
      tokens.emplace_back(TokenType::INT_SIGNED_32,word,column,line);
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
      if (std::isdigit(c))
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
