#include "parser.h"
#include "../lexer/lexer.h"
#include <cstdlib>
#include <unordered_set>
#include <variant>
#include <vector>
#include <iostream>
#include <string>

template<class... Ts> struct overloads : Ts... { using Ts::operator()...; };
template<class... Ts> overloads(Ts...) -> overloads<Ts...>;

Parser::Parser(const std::vector<Token>& tokens) : tokens{tokens}, cursor{0} {}

std::unordered_set<TokenType> typeTable {
    TokenType::INT32,
    TokenType::UINT32,
    TokenType::INT64,
    TokenType::UINT64,
    TokenType::FLOAT32,
    TokenType::FLOAT64,
    TokenType::VADDR,
    TokenType::PADDR
};

std::unordered_set<std::string> readWriteNameTable {
    "read8",
    "read16",
    "read32",
    "read64",
    "write64",
    "write32",
    "write16",
    "write8"
};

//debug function
std::ostream& operator<<(std::ostream& os, const ASTNode& node) {
    std::visit(overloads {
        [&](const Int32LiteralNode& n) {os << "Int32";},
        [&](const Float32LiteralNode& n) {os << "float32";},
        [&](const ReturnStmtNode& n) {os << "return";},
        [&](const BinaryOpNode& n) {os << "binaryOp";},
        [&](const VariableDeclare& n) {os << "varDecl";},
        [&](const VariableAccess& n) {os << "varAcc";},
        [&](const UnaryOpNode& n) {os << "unaryOp";},
        [&](const IfStmtNode& n) {os << "if";},
        [&](const BlockStmtNode& n) {os << "block";},
        [&](const FuncStmtNode& n) {os << "funcDec";},
        [&](const FuncCallStmtNode& n) {os << "funcCall";},
        [&](const VariableModNode& n) {os << "varMod";},
        [&](const PromotionNode& n) {os << "Promot";},
        [&](const readAddrNode& n) {os << "read";},
        [&](const writeAddrNode& n) {os << "write";},
        [&](const asmNode& n) {os << "asm";}
    },node);
    return os;
}

inline const Token& Parser::peek() { return tokens[cursor]; }

Token Parser::consume(TokenType expected) {
    if (expected == peek().type) {
        return tokens[cursor++];
    }
    print_error((std::string)"received "+peek().type+" Expected "+expected);
}

ASTNode Parser::parse_sentence() {
    if (peek().type == TokenType::RETURN) 
        return parse_return_sentence();        
    else if (typeTable.find(peek().type) != typeTable.end()) 
        return parse_local_variable_sentence();
    else if (peek().type == TokenType::IF)
        return parse_if_sentence();
    else if (peek().type == TokenType::IDENTIFIER)
        return parse_identifier_sentence();
    else if (peek().type == TokenType::ASM)
        return parse_asm();
    else {
        print_error("Unknow keyword : "+peek().val);
    }
}

ASTNode Parser::parse_asm() {
    asmNode ret;
    consume(TokenType::ASM);
    consume(TokenType::LBRAK);
    ret.rawAsm = peek().val;
    consume(TokenType::ASM_INSTRUCTIONS);
    consume(TokenType::RBRAK);
    return ret;
}

ASTNode Parser::parse_function_call(const std::string& name) {
    FuncCallStmtNode ret;
    while (peek().type != TokenType::RPARAM) {
        ASTNode expr = parse_expression(Precedence::LOWEST);
        ret.callParameters.push_back(std::make_unique<ASTNode>(std::move(expr)));
        if (peek().type != TokenType::RPARAM)
            consume(TokenType::COMMA);
    }
    ret.name = name;
    consume(TokenType::RPARAM);
    return ret;
}

ASTNode Parser::parse_identifier_sentence() {
    std::string name = peek().val;
    consume(TokenType::IDENTIFIER);
    if (readWriteNameTable.contains(name)) {
        consume(TokenType::LPARAM);
        ASTNode addr = parse_expression(Precedence::LOWEST);
        if (name.find("read") != std::string::npos) {
            readAddrNode ret;
            ret.quantity = std::stoi(name.data()+4);
            ret.addr = std::make_unique<ASTNode>(std::move(addr));
            consume(TokenType::RPARAM);
            consume(TokenType::SEMICOLON);
            return ret;
        }
        else {
            writeAddrNode ret;
            ret.quantity = std::stoi(name.data()+5);
            ret.addr = std::make_unique<ASTNode>(std::move(addr));
            consume(TokenType::COMMA);
            ret.value = std::make_unique<ASTNode>(std::move(parse_expression(Precedence::LOWEST)));
            consume(TokenType::RPARAM);
            consume(TokenType::SEMICOLON);
            return ret;
        }
    }
    if (TokenType::LPARAM == peek().type) {
        consume(TokenType::LPARAM);
        ASTNode ret = parse_function_call(name);
        consume(TokenType::SEMICOLON);
        return ret;
    }
    if (peek().type == TokenType::EQUAL) {
        consume(TokenType::EQUAL);
        VariableModNode ret;
        ret.name = name;
        ASTNode expr = parse_expression(Precedence::LOWEST);
        ret.info = std::make_unique<ASTNode>(std::move(expr));
        consume(TokenType::SEMICOLON);
        return ret;
    }
    else 
        print_error("Unknown identifer: " + name);
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
    ret.name = name;
    consume(TokenType::IDENTIFIER);
    if (peek().type == TokenType::EQUAL) {
        consume(TokenType::EQUAL);
        ASTNode info = parse_expression(Precedence::LOWEST);
        ret.info = std::make_unique<ASTNode>(std::move(info));
    }
    else 
        ret.info = nullptr;

    consume(TokenType::SEMICOLON);

    return ret;
}

ASTNode Parser::parse_return_sentence() {
    consume(TokenType::RETURN);
    ASTNode expr = parse_expression(Precedence::LOWEST);
    consume(TokenType::SEMICOLON);
    return ReturnStmtNode{std::make_unique<ASTNode>(std::move(expr))};
}

ASTNode Parser::parse_primary() {
    if (readWriteNameTable.contains(peek().val)) {
        std::string name = peek().val;
        consume(TokenType::IDENTIFIER);
        consume(TokenType::LPARAM);
        ASTNode addr = parse_expression(Precedence::LOWEST);
        if (name.find("read") != std::string::npos) {
            readAddrNode ret;
            ret.quantity = std::stoi(name.data()+4);
            ret.addr = std::make_unique<ASTNode>(std::move(addr));
            consume(TokenType::RPARAM);
            return ret;
        }
        else
            print_error("Write can't be used as an expression");
    }
    else if (TokenType::IDENTIFIER == peek().type) {
        const std::string& name = peek().val;
        consume(TokenType::IDENTIFIER);
        if (TokenType::LPARAM == peek().type) {
            consume(TokenType::LPARAM);
            return parse_function_call(name);
        }
        else 
            return VariableAccess{.name = name};
    }
    else {
        if (peek().type == TokenType::INT_SIGNED_32)
            return Int32LiteralNode{.value = std::stoi(consume(TokenType::INT_SIGNED_32).val)};
        if (peek().type == TokenType::FLOAT_SIGNED_32)
            return Float32LiteralNode{.value = std::stof(consume(TokenType::FLOAT_SIGNED_32).val)};
    }
    print_error("Unknown type: "+peek().type);
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

        left = UnaryOpNode{token.type, TokenType::EOFTOKEN, std::make_unique<ASTNode>(std::move(operand))};
    } else {
        left = parse_primary();
    }

    while (precedence < get_token_precedence(peek().type)) {
        Token op = peek();
        consume(op.type);

        ASTNode right = parse_expression(get_token_precedence(op.type));

        left = BinaryOpNode{.op = op.type,.precision=TokenType::EOFTOKEN,.left = std::make_unique<ASTNode>(std::move(left)),.right = std::make_unique<ASTNode>(std::move(right))};
    }

    return left;
}

std::vector<ASTNode> Parser::parse() {
    std::vector<ASTNode> astnodes;
    Token token = peek();
    while (token.type != TokenType::EOFTOKEN) {
        if (typeTable.find(peek().type) == typeTable.end()) { // support variable & function declarations
            print_error("Only variables or functions can be declared at file root level");
        }
        else {
	    TokenType type = peek().type;
	    consume(peek().type);
	    std::string name = peek().val;
	    consume(TokenType::IDENTIFIER);
	    if (peek().type == TokenType::LPARAM)
            astnodes.push_back(parse_function(name, type));
	    else
	     	astnodes.push_back(parse_global_variable(name, type));
        }
        token = peek();
    }
    return astnodes;
}

ASTNode Parser::parse_global_variable(const std::string& name, TokenType type) {
	if (peek().type != TokenType::EQUAL) {
		consume(TokenType::SEMICOLON);
		return VariableDeclare{ .name = name, .type = type, .info = nullptr};
	}
	consume(TokenType::EQUAL);
	ASTNode info = parse_expression(Precedence::LOWEST);
	consume(TokenType::SEMICOLON);
	return VariableDeclare{.type = type, .info = std::make_unique<ASTNode>(std::move(info))};
}

ASTNode Parser::parse_parameter() {
    VariableDeclare ret;
    if (typeTable.find(peek().type) == typeTable.end()) {
        print_error("Unknown type : "+peek().val);
    }
    ret.type = peek().type;
    consume(peek().type);
    std::string name = peek().val;
    ret.name = name;
    consume(TokenType::IDENTIFIER);
    if (peek().type == TokenType::EQUAL) {
        consume(TokenType::EQUAL);
        ret.info = std::make_unique<ASTNode>(std::move(parse_expression(Precedence::LOWEST)));
    }
    else
        ret.info = nullptr;

    return ret;
}

ASTNode Parser::parse_function(const std::string& name, TokenType retValue) {
    FuncStmtNode ret;
    ret.retType = retValue;
    ret.name = name;
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

[[noreturn]] void Parser::print_error(const std::string& msg) {
    std::cerr << "Error : " << msg << std::endl;
    std::cerr << "Line : " << peek().line << " Column : " << peek().column << std::endl;
    std::exit(1);
}

void Parser::print_warning(const std::string& msg) {
    std::cerr << "Warning : " << msg << std::endl;
    std::cerr << "Line : " << peek().line << " Column : " << peek().column << std::endl;
}