#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "type.h"
#include <ostream>
#include <vector>
#include <variant>
#include <string>
#include <unordered_map>

namespace nbuFrontend {
    using ASTNode = std::variant<
        struct Int32LiteralNode,
        struct Float32LiteralNode,
        struct ReturnStmtNode,
        struct BinaryOpNode,
        struct VariableDeclare,
        struct VariableAccess,
        struct UnaryOpNode,
        struct IfStmtNode,
        struct BlockStmtNode,
        struct FuncStmtNode,
        struct FuncCallStmtNode,
        struct VariableModNode,
        struct PromotionNode,
        struct readAddrNode,
        struct writeAddrNode,
        struct asmNode
    >;

    std::ostream& operator<<(std::ostream& os, const ASTNode&);

    struct Int32LiteralNode {
        int32_t value;
    };

    struct Float32LiteralNode {
        float value;
    };

    struct ReturnStmtNode {
        ASTNode* expression;
    };

    struct BinaryOpNode {
        TokenType op;
        Type precision;
        ASTNode* left;
        ASTNode* right;
    };

    struct VariableDeclare {
        std::string name;
        Type type;
        ASTNode* info;
    };

    struct VariableAccess {
        std::string name; 
    }; //Temp implementation will be changed later

    struct UnaryOpNode {
        TokenType op;
        Type precision;
        ASTNode* operand;
    };

    struct IfStmtNode {
        ASTNode* condition;
        ASTNode* ifNode;
        ASTNode* elseNode;
    };

    struct BlockStmtNode {
        std::vector<ASTNode*> codes;
    };

    struct FuncStmtNode {
        std::string name;
        Type retType;
        std::vector<ASTNode*> parameters;
        ASTNode* code;
    };

    struct FuncCallStmtNode {
        std::string name;
        std::vector<ASTNode*> callParameters;
    };

    struct VariableModNode {
        std::string name;
        ASTNode* info;
    };

    struct PromotionNode {
        Type topromote;
        Type was;
        ASTNode* info;
    };

    struct readAddrNode {
        ASTNode* addr;
        int8_t quantity;
    };

    struct writeAddrNode {
        ASTNode* addr;
        int8_t quantity;
        ASTNode* value;
    };

    struct asmNode {
        std::string rawAsm;
    };

    enum Precedence {
        LOWEST,
        LOGICAL_OR,
        LOGICAL_AND,
        LOGICAL_XOR,
        BIT_OR,
        BIT_XOR,
        BIT_AND,
        EQUALITY,
        RELATIONAL,
        SHIFT,
        SUM,
        PRODUCT,
        PREFIX
    };
    
    extern std::unordered_map<std::string, Type> typeTable;

    class Parser {
        public:
        std::vector<ASTNode> parse();
        Parser(const std::vector<Token>& tokens);
        
        private:
        Parser();
        const std::vector<Token>& tokens;
        unsigned int cursor;

        inline const Token& peek();
        Token consume(TokenType expected);
        [[noreturn]] void print_error(const std::string& msg);
        ASTNode parse_sentence();
        ASTNode parse_identifier_sentence();
        ASTNode parse_function_call(const std::string& name);
        ASTNode parse_return_sentence();
        ASTNode parse_variable_sentence();
        ASTNode parse_variable_sentence(const std::string& name, Type type);
        ASTNode parse_if_sentence();
        ASTNode parse_block();
        ASTNode parse_function(const std::string& name, Type retValue);
        ASTNode parse_parameter();
        ASTNode parse_expression(int precedence);
        ASTNode parse_primary();
        ASTNode parse_asm();
    };
}

#endif