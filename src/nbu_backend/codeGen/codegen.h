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
        void calcOffsets(const nbuFrontend::ASTNode& n);
        void runOffsetWalker(const nbuFrontend::ASTNode& n);
        void structOffsets(const nbuFrontend::Type& n, std::string name);
        std::string fieldPrint(const nbuFrontend::StructTypeInfo& info, nbuFrontend::Type type, std::string name);
        std::string strWordType(nbuFrontend::Type type);
        std::string strRegistery(nbuFrontend::Type type);
        std::string strDivision(nbuFrontend::Type type);
        bool isConstant(const nbuFrontend::ASTNode& node);
        std::vector<nbuFrontend::ASTNode>& nodes;
        std::unordered_map<std::string, nbuFrontend::StructTypeInfo>& structs;
        std::unordered_map<std::string, int> localOffsetMap;
        int localOffsetCursor;
        int alignOffset;
        std::unordered_set<std::string> globalVariable;
        std::unordered_map<std::string, nbuFrontend::EnumVariantInfo>& enums;
        std::ofstream& file;
    };
}

#endif