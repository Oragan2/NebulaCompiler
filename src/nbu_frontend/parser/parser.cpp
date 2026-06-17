#include "parser.h"
#include "../lexer/lexer.h"
#include <stdexcept>
#include <vector>
#include <iostream>
#include <map>
#include <string>
#include <set>

Parser::Parser(const std::vector<Token>& tokens) : tokens{tokens}, cursor{0} {}

std::map<std::string, SymboleInfo> SymboleTable;

std::set<TokenType> typeTable {
    TokenType::INT32,
    TokenType::UINT32
};

inline Token Parser::peek() { return tokens[cursor]; }

Token Parser::consume(TokenType expected) {
    if (expected == peek().type) {
        return tokens[cursor++];
    }
    std::cerr << "Received : " << peek().type << "\n";
    std::cerr << "Expected : " << expected << "\n";
    std::cerr << "Error on line : " << peek().line << " column : " << peek().column << "\n";
}

ASTNode Parser::parse_sentence() {
    if (peek().type == TokenType::RETURN)
        return parse_return_sentence();
    else if (typeTable.find(peek().type) != typeTable.end())
        return parse_variable_sentence();    
    else {
        std::cerr << "Received : " << peek().type << "\n";
        std::cerr << "Error on line : " << peek().line << " column : " << peek().column << "\n";
    }
}

ASTNode Parser::parse_variable_sentence() {
    VariableDeclare ret = (VariableDeclare){.type = peek().type};
    consume(peek().type);
    std::string name = peek().val; 
    consume(TokenType::IDENTIFIER);
    consume(TokenType::EQUAL);
    ASTNode info = parse_expression(Precedence::LOWEST);
    consume(TokenType::SEMICOLON);
    ret.info = std::make_unique<ASTNode>(std::move(info));

    SymboleTable.emplace(name, (SymboleInfo){.type = ret.type, .stack_offset = 0});

    return ret;
}

ASTNode Parser::parse_return_sentence() {
    consume(TokenType::RETURN);
    ASTNode expr = parse_expression(Precedence::LOWEST);
    consume(TokenType::SEMICOLON);

    return ReturnStmtNode{std::make_unique<ASTNode>(std::move(expr))};
}

ASTNode Parser::parse_primary() {
    if (peek().type == TokenType::IDENTIFIER && SymboleTable.find(peek().val) != SymboleTable.end()) {
        return (VariableAccess){.name = consume(TokenType::IDENTIFIER).val};
    }
    else if (peek().type == TokenType::IDENTIFIER) {
        std::cerr << "Symbole : " << peek().val << " not declared\n";
        std::cerr << "Error line : " << peek().line << " column : " << peek().column << "\n";
    }
    else
        return (Int32LiteralNode){.value = std::stoi(consume(TokenType::INT_SIGNED_32).val)};
}

Precedence get_token_precedence(TokenType type) {
    switch(type) {
        case TokenType::PLUS:
        case TokenType::MINUS:
            return Precedence::SUM;
        case TokenType::STAR:
        case TokenType::SLASH:
        case TokenType::PERCENT:
            return Precedence::PRODUCT;
        case TokenType::SHIFTL:
        case TokenType::SHIFTR:
            return Precedence::SHIFT;
        case TokenType::EXCLAMATION:
        case TokenType::NOT:
            return Precedence::PREFIX;
        case TokenType::OR:
            return Precedence::BIT_OR;
        case TokenType::AND:
            return Precedence::BIT_AND;
        case TokenType::XOR:
            return Precedence::BIT_XOR;
        case TokenType::OROR:
            return Precedence::LOGICAL_OR;
        case TokenType::ANDAND:
            return Precedence::LOGICAL_AND;
        case TokenType::XORXOR:
            return Precedence::LOGICAL_XOR;
        case TokenType::EQUALEQUAL:
        case TokenType::DIFFERENT:
            return Precedence::EQUALITY;
        case TokenType::LT:
        case TokenType::LTE:
        case TokenType::MT:
        case TokenType::MTE:
            return Precedence::RELATIONAL;
        default:
            return Precedence::LOWEST;
    }
}

ASTNode Parser::parse_expression(int precedence) {
    Token token = peek();
    ASTNode left;

    if (token.type == TokenType::EXCLAMATION ||
        token.type == TokenType::NOT ||
        token.type == TokenType::MINUS) {
        
        consume(token.type);

        ASTNode operand = parse_expression(Precedence::PREFIX);

        left = UnaryOpNode{token.type, std::make_unique<ASTNode>(std::move(operand))};
    } else {
        left = parse_primary();
    }

    while (precedence < get_token_precedence(peek().type)) {
        Token op = peek();
        consume(op.type);

        ASTNode right = parse_expression(get_token_precedence(op.type));

        left = BinaryOpNode{op.type,std::make_unique<ASTNode>(std::move(left)),std::make_unique<ASTNode>(std::move(right))};
    }

    return left;
}

std::vector<ASTNode> Parser::parse() {
    std::vector<ASTNode> astnodes;
    Token token = peek();
    while (token.type != TokenType::EOFTOKEN) {
        astnodes.push_back(parse_sentence());
        token = peek();
    }
    return astnodes;
}
