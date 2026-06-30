#ifndef CODEGEN_H
#define CODEGEN_H

#include "../../nbu_frontend/parser/parser.h"
#include <fstream>
#include <vector>

namespace nbuBackend {
    class CodeGen {
        public:
        CodeGen(std::vector<nbuFrontend::ASTNode>& nodes,std::ofstream& file);
        void generate();

        private:
        CodeGen();
        std::vector<nbuFrontend::ASTNode>& nodes;
        std::ofstream& file;
    };
}

#endif