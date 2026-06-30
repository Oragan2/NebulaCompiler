#ifndef CODEGEN_H
#define CODEGEN_H

#include "../../nbu_frontend/parser/parser.h"
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace nbuBackend {
    class CodeGen {
        public:
        CodeGen(std::vector<nbuFrontend::ASTNode>& nodes, std::ofstream& file, std::unordered_map<std::string, nbuFrontend::StructTypeInfo>& structs, std::unordered_map<std::string, nbuFrontend::EnumVariantInfo>& enums);
        void generate();

        private:
        CodeGen();
        int calcStructSize(nbuFrontend::StructTypeInfo& info);
        std::string nodeCalcString(const nbuFrontend::ASTNode& n);
        std::vector<nbuFrontend::ASTNode>& nodes;
        std::unordered_map<std::string, nbuFrontend::StructTypeInfo>& structs;
        std::unordered_map<std::string, int> localOffsetMap;
        int localOffsetCursor;
        std::unordered_set<std::string> globalVariable;
        std::unordered_map<std::string, nbuFrontend::EnumVariantInfo>& enums;
        std::ofstream& file;
    };
}

#endif