#include "parser.h"
#include "../lexer/lexer.h"
#include <stdexcept>
#include <vector>
#include <iostream>
#include <map>
#include <string>
#include <set>

Parser::Parser(const std::vector<Token>& tokens) : tokens{tokens}, cursor{0} {}

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
        return parse_local_variable_sentence();
    else if (peek().type == TokenType::IF)
        return parse_if_sentence();
    else {
        std::cerr << "Received : " << peek().type << "\n";
        std::cerr << "Error on line : " << peek().line << " column : " << peek().column << "\n";
    }
}

ASTNode Parser::parse_if_sentence() {
    IfStmtNode ret;
    consume(TokenType::IF);
    consume(TokenType::LPARAM);
    ASTNode condition = parse_expression(Precedence::LOWEST);
    ret.condition = std::make_unique<ASTNode>(std::move(condition));
    consume(TokenType::RPARAM);

    if (peek().type == TokenType::LBRAK) 
        ret.ifNode = std::make_unique<ASTNode>(std::move(parse_block()));
    else 
        ret.ifNode = std::make_unique<ASTNode>(std::move(parse_sentence()));
    
    if (peek().type == TokenType::ELSE) {
        consume(TokenType::ELSE);
        ret.elseNode = std::make_unique<ASTNode>(std::move(parse_sentence()));
    }
    else
        ret.elseNode = nullptr;

    return ret;
}

ASTNode Parser::parse_block() {
    consume(TokenType::LBRAK);
    BlockStmtNode ret;
    while (peek().type != TokenType::RBRAK) {
        ret.codes.push_back(std::move(std::make_unique<ASTNode>(parse_sentence())));
    }
    consume(TokenType::RBRAK);
    return ret;
}

ASTNode Parser::parse_local_variable_sentence() {
    VariableDeclare ret = (VariableDeclare){.type = peek().type};
    consume(peek().type);
    std::string name = peek().val; 
    consume(TokenType::IDENTIFIER);
    if (peek().type == TokenType::EQUAL) {
        consume(TokenType::EQUAL);
        ASTNode info = parse_expression(Precedence::LOWEST);
        ret.info = std::make_unique<ASTNode>(std::move(info));
    }
    else 
        ret.info = nullptr;

    consume(TokenType::SEMICOLON);

    currentFunc.LocalSymboleTable.emplace(name, (SymboleInfo){.type = ret.type, .stack_offset = 0});

    return ret;
}

ASTNode Parser::parse_return_sentence() {
    consume(TokenType::RETURN);
    ASTNode expr = parse_expression(Precedence::LOWEST);
    consume(TokenType::SEMICOLON);

    return ReturnStmtNode{std::make_unique<ASTNode>(std::move(expr))};
}

ASTNode Parser::parse_primary() {
    if (peek().type == TokenType::IDENTIFIER && (GlobalSymboleTable.find(peek().val) != GlobalSymboleTable.end() || currentFunc.LocalSymboleTable.find(peek().val) != currentFunc.LocalSymboleTable.end())) {
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
        if (token.type != TokenType::FUNC) { //global variable will be added later
            std::cerr << "Error only functions or global variable allowed at file level";
            std::cerr << "Line : " << token.line << " Column : " << token.column << "\n";
            return astnodes;
        }
        else {
            astnodes.push_back(parse_function());
        }
        token = peek();
    }
    return astnodes;
}

ASTNode Parser::parse_parameter() {
    VariableDeclare ret;
    if (typeTable.find(peek().type) == typeTable.end()) {
        std::cerr << "Unkown type : " << peek().type << "\n";
        std::cerr << "Line : " << peek().line << " Column : " << peek().column << "\n";
    }
    ret.type = peek().type;
    consume(peek().type);
    std::string name = peek().val;
    consume(TokenType::IDENTIFIER);
    if (peek().type == TokenType::EQUAL) {
        consume(TokenType::EQUAL);
        ret.info = std::make_unique<ASTNode>(std::move(parse_expression(Precedence::LOWEST)));
    }
    else
        ret.info = nullptr;
    
    currentFunc.LocalSymboleTable.emplace(name,SymboleInfo{.type = ret.type, .stack_offset = 0});
    return ret;
}

ASTNode Parser::parse_function() {
    FuncStmtNode ret;
    currentFunc = FunctionInfo{};
    consume(TokenType::FUNC);
    if (typeTable.find(peek().type) == typeTable.end()) {
        std::cerr << "Awaited a function return parameter\n";
        std::cerr << "Line : " << peek().line << " Column : " << peek().column << "\n";
    }
    ret.retType = peek().type;
    currentFunc.retType = peek().type;
    consume(peek().type);
    std::string name = peek().val; 
    consume(TokenType::IDENTIFIER);
    consume(TokenType::LPARAM);
    while (peek().type != TokenType::RPARAM) {
        ret.parameters.push_back(std::make_unique<ASTNode>(std::move(parse_parameter())));
        if (peek().type != TokenType::RPARAM)
            consume(TokenType::COMMA);
    }
    consume(TokenType::RPARAM);
    if (peek().type == TokenType::LBRAK) 
        ret.code = std::make_unique<ASTNode>(std::move(parse_block()));
    else if (peek().type == TokenType::SEMICOLON)
        ret.code = nullptr;
    else 
        ret.code = std::make_unique<ASTNode>(std::move(parse_sentence()));
    return ret; 
}
