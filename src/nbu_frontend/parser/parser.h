#ifndef PARSER_H
#define PARSER_H

#include "../lexer/lexer.h"
#include <ostream>
#include <vector>
#include <variant>
#include <memory>
#include <string>
#include <list>

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

    struct Type {
        enum class Kind {
            FLOAT32, FLOAT64,
            INT32, INT64,
            UINT32, UINT64,
            VADDR, PADDR
        };
        Kind kind;
        std::string name;
    };

    std::ostream& operator<<(std::ostream& os, const ASTNode&);

    struct Int32LiteralNode {
        int32_t value;
    };

    struct Float32LiteralNode {
        float value;
    };

    struct ReturnStmtNode {
        std::unique_ptr<ASTNode> expression;
    };

    struct BinaryOpNode {
        TokenType op;
        TokenType precision;
        std::unique_ptr<ASTNode> left;
        std::unique_ptr<ASTNode> right;
    };

    struct VariableDeclare {
        std::string name;
        TokenType type;
        std::unique_ptr<ASTNode> info;
    };

    struct VariableAccess {
        std::string name; 
    }; //Temp implementation will be changed later

    struct UnaryOpNode {
        TokenType op;
        TokenType precision;
        std::unique_ptr<ASTNode> operand;
    };

    struct IfStmtNode {
        std::unique_ptr<ASTNode> condition;
        std::unique_ptr<ASTNode> ifNode;
        std::unique_ptr<ASTNode> elseNode;
    };

    struct BlockStmtNode {
        std::list<std::unique_ptr<ASTNode>> codes;
    };

    struct FuncStmtNode {
        std::string name;
        TokenType retType;
        std::vector<std::unique_ptr<ASTNode>> parameters;
        std::unique_ptr<ASTNode> code;
    };

    struct FuncCallStmtNode {
        std::string name;
        std::vector<std::unique_ptr<ASTNode>> callParameters;
    };

    struct VariableModNode {
        std::string name;
        std::unique_ptr<ASTNode> info;
    };

    struct PromotionNode {
        TokenType topromote;
        TokenType was;
        std::unique_ptr<ASTNode> info;
    };

    struct readAddrNode {
        std::unique_ptr<ASTNode> addr;
        int8_t quantity;
    };

    struct writeAddrNode {
        std::unique_ptr<ASTNode> addr;
        int8_t quantity;
        std::unique_ptr<ASTNode> value;
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
        void print_warning(const std::string& msg);
        ASTNode parse_sentence();
        ASTNode parse_identifier_sentence();
        ASTNode parse_function_call(const std::string& name);
        ASTNode parse_return_sentence();
        ASTNode parse_local_variable_sentence();
        ASTNode parse_global_variable(const std::string& name, TokenType type);
        ASTNode parse_if_sentence();
        ASTNode parse_block();
        ASTNode parse_function(const std::string& name, TokenType retValue);
        ASTNode parse_parameter();
        ASTNode parse_expression(int precedence);
        ASTNode parse_primary();
        ASTNode parse_asm();
    };
}

#endif