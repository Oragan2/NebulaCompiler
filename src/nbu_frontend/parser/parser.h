#ifndef PARSER_H
#define PARSER_H

#include "../lexer/lexer.h"
#include "../type/type.h"
#include <ostream>
#include <vector>
#include <variant>
#include <string>

namespace nbuFrontend {
    using ASTNode = std::variant<
        struct Int32LiteralNode,
        struct Float32LiteralNode,
        struct ReturnStmtNode,
        struct BinaryOpNode,
        struct VariableDeclareNode,
        struct VariableAccessNode,
        struct UnaryOpNode,
        struct IfStmtNode,
        struct BlockStmtNode,
        struct FuncStmtNode,
        struct FuncCallStmtNode,
        struct VariableModNode,
        struct PromotionNode,
        struct readAddrNode,
        struct writeAddrNode,
        struct asmNode,
        struct EnumDeclNode,
        struct EnumAccessNode,
        struct StructDeclNode,
        struct StructAccessNode
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

    struct VariableDeclareNode {
        std::string name;
        Type type;
        ASTNode* info;
        SymboleInfo vInfo;
    };

    struct VariableAccessNode {
        std::string name; 
        Type precision;
        SymboleInfo info;
    };

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
        Type retType;
        size_t id;
    };

    struct VariableModNode {
        ASTNode* variable;
        ASTNode* info;
        Type precision;
        SymboleInfo vInfo;
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

    struct EnumDeclNode {
        std::string name;
        std::vector<std::pair<std::string, int>> members;
    };

    struct EnumAccessNode {
        std::string enumName;
        std::string enumMember;
    };

    struct StructDeclNode {
        std::string structName;
        std::vector<std::pair<std::string,Type>> fields; // temporary until I find a better way to have preinitialized values
    };
    
    struct StructAccessNode {
        Type baseType;
        ASTNode* firstPart;
        std::string fieldName;
        StructField info;
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
        ASTNode parse_enum();
        ASTNode parse_struct();
    };
}

#endif
