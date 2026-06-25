#include "parser.h"
#include "../lexer/lexer.h"
#include <cstddef>
#include <cstdlib>
#include <unordered_set>
#include <vector>
#include <iostream>
#include <map>
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

inline const Token& Parser::peek() { return tokens[cursor]; }

Token Parser::consume(TokenType expected) {
    if (expected == peek().type) {
        return tokens[cursor++];
    }
    print_error((std::string)"Received : "+peek().type+" Expected "+expected);
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
    else {
        print_error("Unknow keyword : "+peek().val);
    }
}

ASTNode Parser::parse_function_call(const std::string& name) {
    const FunctionInfo& func = functions[name];
    FuncCallStmtNode ret;
    for (int i = 0; i < func.paramType.size(); ++i) {
        ASTNode expr = parse_expression(Precedence::LOWEST);
        TokenType precision = type_precision(expr);
        if (precision != func.paramType[i]) {
            TokenType promoted = tryPromote(precision, func.paramType[i]);
            if (promoted == precision)
                print_error("Argument type given is : "+precision+" but expected : "+func.paramType[i]);
            else {
                print_warning("Type "+precision+" was promoted to a "+promoted+" to fit the functions parameters");
                ret.callParameters.push_back(std::make_unique<ASTNode>(std::move(PromotionNode{.topromote = promoted, .was = precision, .info = std::make_unique<ASTNode>(std::move(expr))})));
            }
        }
        else 
            ret.callParameters.push_back(std::make_unique<ASTNode>(std::move(expr)));
        if (i+1 != func.paramType.size())
            consume(TokenType::COMMA);
    }
    ret.name = name;
    consume(TokenType::RPARAM);
    return ret;
}

ASTNode Parser::parse_identifier_sentence() {
    std::string name = peek().val;
    if (!(GlobalSymboleTable.contains(name) || currentFunc.LocalSymboleTable.contains(name) || functions.contains(name))) {
        print_error("Symbole : "+peek().val+" unknown");
    } 
    consume(TokenType::IDENTIFIER);
    if (peek().type == TokenType::LPARAM) {
        consume(TokenType::LPARAM);
        ASTNode ret = parse_function_call(name);
        consume(TokenType::SEMICOLON);
        return ret;
    }
    else {
        consume(TokenType::EQUAL);
        VariableModNode ret;
        ret.name = name;
        ASTNode expr = parse_expression(Precedence::LOWEST);
        TokenType precision = type_precision(expr);
        SymboleInfo a = GlobalSymboleTable.contains(name) ? GlobalSymboleTable.at(name) : currentFunc.LocalSymboleTable.at(name);
        if (precision != a.type) {
            TokenType promoted = tryPromote(precision, a.type);
            if (promoted == precision)
                print_error("Missmatched type for "+name+" of type "+a.type+" but given "+precision);
            else {
                print_warning("Type "+precision+" was promoted to a "+promoted+" to fit the variable value change");
                ret.info = std::make_unique<ASTNode>(std::move(PromotionNode{.topromote = promoted, .was = precision, .info = std::make_unique<ASTNode>(std::move(expr))}));
            }
        }
        else
            ret.info = std::make_unique<ASTNode>(std::move(expr));
        consume(TokenType::SEMICOLON);
        return ret;
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
        TokenType precision = type_precision(info);
        if (ret.type != precision) {
            TokenType promoted = tryPromote(precision, ret.type);
            if (promoted == precision)
                print_error("Can't asign a "+precision+" to a "+ret.type+" variable");
            else {
                print_warning("Promoted the type from a "+precision+" to a "+promoted+" to allow the variable initialisation");
                ret.info = std::make_unique<ASTNode>(std::move(PromotionNode{.topromote = promoted, .was = precision, .info = std::make_unique<ASTNode>(std::move(info))}));
            }
        }
        else
            ret.info = std::make_unique<ASTNode>(std::move(info));
    }
    else 
        ret.info = nullptr;

    consume(TokenType::SEMICOLON);

    currentFunc.LocalSymboleTable.emplace(name, SymboleInfo{.type = ret.type, .stack_offset = 0});

    return ret;
}

ASTNode Parser::parse_return_sentence() {
    consume(TokenType::RETURN);
    ASTNode expr = parse_expression(Precedence::LOWEST);
    consume(TokenType::SEMICOLON);
    TokenType retType = type_precision(expr);

    if (retType != currentFunc.retType) {
        TokenType promotedType = tryPromote(retType,currentFunc.retType);
        if (promotedType == retType)
            print_error("Return type mismatch Given : "+currentFunc.retType+" Expected : "+retType); // TODO will be changed to a warning later when type promotion will be taken in effect
        else {   
            print_warning("Type "+retType+" was promoted to a "+promotedType+" to fit the functions returntype"); //TODO will probably be removed or made into a possible warning like -Wpromote
            expr = PromotionNode{.topromote = promotedType, .was = retType, .info = std::make_unique<ASTNode>(std::move(expr))};
        }
    }
    return ReturnStmtNode{std::make_unique<ASTNode>(std::move(expr))};
}

ASTNode Parser::parse_primary() {
    if (GlobalSymboleTable.contains(peek().val) || currentFunc.LocalSymboleTable.contains(peek().val)) {
        return (VariableAccess){.name = consume(TokenType::IDENTIFIER).val};
    }
    else if (functions.contains(peek().val)) {
        const std::string& name = peek().val;
        consume(TokenType::IDENTIFIER);
        consume(TokenType::LPARAM);
        return parse_function_call(name);
    }
    else if (peek().type == TokenType::IDENTIFIER) {
        print_error("Symbole : "+peek().val+" unknown");
    }
    else {
	if (peek().type == TokenType::INT_SIGNED_32)
        return Int32LiteralNode{.value = std::stoi(consume(TokenType::INT_SIGNED_32).val)};
	else
	 	return Float32LiteralNode{.value = std::stof(consume(TokenType::FLOAT_SIGNED_32).val)}; 
    }
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

        TokenType precision = type_precision(operand);

        left = UnaryOpNode{token.type, precision, std::make_unique<ASTNode>(std::move(operand))};
    } else {
        left = parse_primary();
    }

    while (precedence < get_token_precedence(peek().type)) {
        Token op = peek();
        consume(op.type);

        ASTNode right = parse_expression(get_token_precedence(op.type));

        TokenType precision = resolve_type(type_precision(left), type_precision(right));
        left = BinaryOpNode{op.type,precision,std::make_unique<ASTNode>(std::move(left)),std::make_unique<ASTNode>(std::move(right))};
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
	GlobalSymboleTable.emplace(name, SymboleInfo{.type = type, .stack_offset = 0});
	if (peek().type != TokenType::EQUAL) {
		consume(TokenType::SEMICOLON);
		return VariableDeclare{.type = type, .info = nullptr};
	}
	consume(TokenType::EQUAL);
	ASTNode info = parse_expression(Precedence::LOWEST);
    TokenType precision = type_precision(info);
    if (type != precision) {
        TokenType promoted = tryPromote(precision, type);
        if (promoted == precision)
            print_error("Can't asign a "+precision+" to a "+type+" variable");
        else
            print_warning("Promoted the type from a "+precision+" to a "+promoted+" to allow the variable initialisation");
    }
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

ASTNode Parser::parse_function(const std::string& name, TokenType retValue) {
    FuncStmtNode ret;
    ret.retType = retValue;
    currentFunc = FunctionInfo{.retType = retValue};
    consume(TokenType::LPARAM);
    while (peek().type != TokenType::RPARAM) {
        currentFunc.paramType.push_back(peek().type);
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
    functions.emplace(name, currentFunc);
    return ret; 
}

void Parser::print_error(const std::string& msg) {
    std::cerr << "Error : " << msg << std::endl;
    std::cerr << "Line : " << peek().line << " Column : " << peek().column << std::endl;
    std::exit(1);
}

void Parser::print_warning(const std::string& msg) {
    std::cerr << "Warning : " << msg << std::endl;
    std::cerr << "Line : " << peek().line << " Column : " << peek().column << std::endl;
}

TokenType Parser::type_precision(const ASTNode& node) {
    return std::visit(overloads {
        [this](const Int32LiteralNode& n) {return TokenType::INT32;},
        [this](const Float32LiteralNode& n) {return TokenType::FLOAT32;},
        [this](const VariableAccess& n) {
            if (currentFunc.LocalSymboleTable.contains(n.name))
                return currentFunc.LocalSymboleTable.at(n.name).type;
            else
                return GlobalSymboleTable.at(n.name).type;
        },
        [this](const BinaryOpNode& n) {return resolve_type(type_precision(*n.left), type_precision(*n.right));},
        [this](const UnaryOpNode& n) {return type_precision(*n.operand);},
        [this](const FuncCallStmtNode& n) {return functions.at(n.name).retType;},
        [this](const auto&) {print_error("Uh?"); return TokenType::EOFTOKEN;}
    }, node);
}

TokenType Parser::resolve_type(TokenType left, TokenType right) {
    if (left == right) return left;
    if (left == TokenType::FLOAT64 || right == TokenType::FLOAT64) return TokenType::FLOAT64; 
    if (left == TokenType::FLOAT32 || right == TokenType::FLOAT32) return TokenType::FLOAT32; 
    if (left == TokenType::UINT64 || right == TokenType::UINT64) return TokenType::UINT64; 
    if (left == TokenType::INT64 || right == TokenType::INT64) return TokenType::INT64; 
    if (left == TokenType::UINT32 || right == TokenType::UINT32) return TokenType::UINT32; 
    return TokenType::INT32;
}

TokenType Parser::tryPromote(TokenType currentType, TokenType promoteTo) {
    if (resolve_type(currentType, promoteTo) == promoteTo)
        return promoteTo;
    if (currentType == TokenType::UINT32 && promoteTo == TokenType::INT32) return promoteTo;
    if (currentType == TokenType::UINT64 && promoteTo == TokenType::INT64) return promoteTo;
    return currentType;
}