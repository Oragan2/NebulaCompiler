#include "codegen.h"
#include "../../nbu_frontend/parser/parser.h"
#include <fstream>
#include <vector>

template<class... Ts> struct overloads : Ts... { using Ts::operator()...; };
template<class... Ts> overloads(Ts...) -> overloads<Ts...>;


namespace nbuBackend {
    CodeGen::CodeGen(std::vector<nbuFrontend::ASTNode>& nodes, std::ofstream& file) : nodes{nodes}, file{file} {}
    void CodeGen::generate() {
        std::ofstream data_file("output.data.tmp");
        std::ofstream text_file("output.text.tmp");

        data_file << "section .data\n";
        text_file << "section .text\n";

        for (const auto& node : nodes) {
            std::visit(overloads {
                [&](const nbuFrontend::VariableDeclareNode& n) {
                    
                },
                [&](const nbuFrontend::FuncStmtNode& n) {

                },
                [&](const auto&) {}
            },node);
        }
    }
}