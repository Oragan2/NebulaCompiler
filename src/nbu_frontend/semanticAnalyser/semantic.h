#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "type.h"
#include "parser.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <utility>

namespace nbuFrontend {

    class Semantic {
        public:
        Semantic(std::vector<ASTNode>& nodes);
        std::pair<int, int> semanticAnalyses();
        std::vector<ASTNode>& getNodes();
        std::unordered_map<std::string, EnumVariantInfo>& getEnums() {return globalEnumRegistry;}
        std::unordered_map<std::string, StructTypeInfo>& getStruct() {return globalStructRegistery;}
        
        private:
        std::vector<ASTNode>& nodes;
        std::unordered_map<std::string, FunctionInfo> functions;
        std::unordered_map<std::string, SymboleInfo> globalSymbolTable;
        std::unordered_map<std::string, EnumVariantInfo> globalEnumRegistry;
        std::unordered_map<std::string, StructTypeInfo> globalStructRegistery;
        std::vector<std::unordered_map<std::string, SymboleInfo>> scopeStack;
        FunctionInfo currentFunc;
        unsigned int errorNumber = 0;
        unsigned int warningNumber = 0;
        unsigned int offset = 0;
        unsigned int functionCounter = 0;

        void codeSemanticAnalyses(ASTNode& node);
        Type type_precision(const ASTNode& node);
        Type resolve_type(Type left, Type right);
        SymboleInfo resolveVariable(const ASTNode& n);
        Type tryPromote(Type currentType, Type promoteTo);
        bool hasReturn(const ASTNode& n);
        void print_error(const std::string& msg);
        void print_warning(const std::string& msg);
        void structSize(StructTypeInfo& info);
    };
}

#endif