#include "codegen.h"
#include "../../nbu_frontend/parser/parser.h"
#include <fstream>
#include <string>
#include <vector>
#include <cstdio>

template<class... Ts> struct overloads : Ts... { using Ts::operator()...; };
template<class... Ts> overloads(Ts...) -> overloads<Ts...>;


namespace nbuBackend {
    CodeGen::CodeGen(std::vector<nbuFrontend::ASTNode>& nodes, std::ofstream& file, std::unordered_map<std::string, nbuFrontend::StructTypeInfo>& structs, std::unordered_map<std::string, nbuFrontend::EnumVariantInfo>& enums) : nodes{nodes}, file{file}, structs{structs}, enums{enums} {}
    void CodeGen::generate() {
        std::ofstream data_file("output.data.tmp");
        std::ofstream bss_file("output.bss.tmp");
        std::ofstream text_file("output.text.tmp");

        data_file << "section .data\n";
        bss_file << "section .bss\n";
        text_file << "section .text\n";

        for (const auto& node : nodes) {
            std::visit(overloads {
                [&](const nbuFrontend::VariableDeclareNode& n) {
                    if (n.type.kind != nbuFrontend::Type::Kind::STRUCT) {
                        data_file << "\t" + n.name + ": d";
                        switch(nbuFrontend::typeSize[n.type.kind]) {
                            case 1:
                                data_file << "b ";
                                break;
                            case 2:
                                data_file << "w ";
                                break;
                            case 4:
                                data_file << "d ";
                                break;
                            case 8:
                                data_file << "q ";
                                break;
                        }
                        if (n.info == nullptr)
                            data_file << "0\n";
                        else {
                            data_file << std::visit(overloads {
                                [](const nbuFrontend::Int32LiteralNode& n) {
                                    return std::to_string(n.value);
                                },
                                [](const nbuFrontend::Float32LiteralNode& n) {
                                    std::cout << n.value << std::endl;
                                    return std::to_string(n.value);
                                },
                                [](const auto&) {
                                    return std::string{"0"};
                                }
                            }, *n.info);
                            data_file << "\n";
                        }
                    }
                    else {
                        bss_file << "\t" << n.name << ": resb " << calcStructSize(structs[n.type.name]) << "\n";
                    }
                    globalVariable.emplace(n.name);
                },
                [&](const nbuFrontend::FuncStmtNode& n) {
                    if (n.name != "main") {
                        text_file << "\tglobal " << n.name << "\n";
                        text_file << n.name << ":\n";
                    }
                    else {
                        text_file << "\tglobal _start" << "\n";
                        text_file << "_start:\n";
                    }
                    text_file << nodeCalcString(*n.code);
                    text_file << "\n";
                },
                [&](const auto&) {}
            },node);
        }
        data_file.close();
        bss_file.close();
        text_file.close();
        std::ifstream data("output.data.tmp");
        std::ifstream bss("output.bss.tmp");
        std::ifstream text("output.text.tmp");
        char c;

        file << data.rdbuf();
        file << bss.rdbuf();
        file << text.rdbuf();

        std::remove("output.data.tmp");
        std::remove("output.bss.tmp");
        std::remove("output.text.tmp");
    }

    std::string CodeGen::nodeCalcString(const nbuFrontend::ASTNode& n) {
        std::string ret;
        std::visit(overloads {
            [&](const nbuFrontend::BlockStmtNode& n) {
                for (const auto& node : n.codes) {
                    ret += nodeCalcString(*node);
                }
            },
            [&](const nbuFrontend::asmNode& n) {
                ret += "\t"+n.rawAsm+"\n";
            },
            [&](const nbuFrontend::ReturnStmtNode& n) {
                ret += nodeCalcString(*n.expression);
                ret += "\tret\n";
            },
            [&](const nbuFrontend::VariableDeclareNode& n) {
                
            }, 
            [&](const auto&) {
                ret += "\tret\n";
            }
        },n);
        return ret;
    }

    int CodeGen::calcStructSize(nbuFrontend::StructTypeInfo& info) {
        int currentSize = 0;
        int maxAlignment = 1; // Track the largest alignment requirements in the struct
        for (const auto& [name, type] : info.fields) {
            int fieldSize = 0;
            int fieldAlignment = 1;

            if (type.kind != nbuFrontend::Type::Kind::STRUCT) {
                fieldSize = nbuFrontend::typeSize[type.kind];
                fieldAlignment = fieldSize;
            } else {
                fieldSize = calcStructSize(structs[type.name]);
                fieldAlignment = 8;
            }

            if (fieldAlignment > maxAlignment) {
                maxAlignment = fieldAlignment;
            }

            if (currentSize % fieldAlignment != 0) {
                currentSize += (fieldAlignment - (currentSize % fieldAlignment));
            }

            // Place the field
            currentSize += fieldSize;
        }
        if (currentSize % maxAlignment != 0) {
            currentSize += (maxAlignment - (currentSize % maxAlignment));
        }
        return currentSize;
    }
}