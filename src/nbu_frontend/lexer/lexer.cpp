#include "lexer.h"
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

std::unordered_map<std::string_view, TokenType> map{
    {"return", TokenType::RETURN},
    {"+", TokenType::PLUS},
    {"*", TokenType::STAR}};

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
  default:
    os << "TODO";
    break;
  }
  return os;
}

std::vector<Token> lexer(std::ifstream &file) {
  std::vector<Token> tokens;
  std::string word;

  State state = State::START;
  char c;

  auto flush_token = [&]() {
    if (word.empty())
      return;
    if (state == State::TEXT)
      tokens.emplace_back(map[word]);
    else if (state == State::NUMBER)
      tokens.emplace_back(TokenType::INT_SIGNED_32, std::stoi(word));
    word.clear();
    state = State::START;
  };

  while (file.get(c)) {
    switch (state) {
    case State::START:
      if (std::isspace(c))
        continue;
      if (c == ';') {
        tokens.emplace_back(TokenType::SEMICOLON);
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
          tokens.emplace_back(TokenType::SEMICOLON);
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
        if (c == ';')
          tokens.emplace_back(TokenType::SEMICOLON);
        else if (!std::isspace(c)) {
          state = std::isdigit(c) ? State::NUMBER : State::TEXT;
          word += c;
        }
      }
      break;
    }
  }

  flush_token();

  return tokens;
}
