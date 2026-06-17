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
    {'!', TokenType::EXCLAMATION}
};

std::unordered_map<std::string_view, TokenType> map{
    {"return", TokenType::RETURN},
    {"<<", TokenType::SHIFTL},
    {">>", TokenType::SHIFTR}
};

std::ostream &operator<<(std::ostream &os, TokenType token) {
  switch (token) {
  case TokenType::INT_SIGNED_32:
    os << "int";
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
  default:
    os << "TODO";
    break;
  }
  return os;
}

std::vector<Token> lexer(std::ifstream &file) {
  std::vector<Token> tokens;
  std::string word;
  unsigned int column;
  unsigned int line;

  State state = State::START;
  char c;

  auto flush_token = [&]() {
    if (word.empty())
      return;
    if (state == State::TEXT) {
      if (map.find(word) == map.end())
        throw std::runtime_error("Unknown keyword");
      tokens.emplace_back(map[word],word,column,line);
    }
    else if (state == State::NUMBER) 
      tokens.emplace_back(TokenType::INT_SIGNED_32,word,column,line);
    column += word.length();
    word.clear();
    state = State::START;
  };

  while (file.get(c)) {
    if (c == '\n') {
      column = 0;
      line += 1;
    }
    switch (state) {
    case State::START:
      if (std::isspace(c))
        continue;
      if (c == ';') {
        tokens.emplace_back(TokenType::SEMICOLON,word,column,line);
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

        if (c == ';')
          tokens.emplace_back(TokenType::SEMICOLON,word,column,line);
        else if (!std::isspace(c)) {
          state = State::TEXT;
          word += c;
        }
      }
      break;
    case State::TEXT:
      if (std::isalpha(c))
        word += c;
      else {
        flush_token();
        if (c == ';')
          tokens.emplace_back(TokenType::SEMICOLON,word,column,line);
        else if (!std::isspace(c)) {
          state = std::isdigit(c) ? State::NUMBER : State::TEXT;
          word += c;
        }
      }
      break;
    }
  }

  flush_token();

  tokens.emplace_back(TokenType::EOFTOKEN,"",column,line);

  return tokens;
}
