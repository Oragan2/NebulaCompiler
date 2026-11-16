#include "../../nbu_frontend/parser/parser.h"
#include "../../nbu_frontend/lexer/lexer.h"
#include "../../nbu_frontend/semanticAnalyser/semantic.h"
#include "ir.h"
#include "type.h"
#include "irTranslator.h"
#include <cstdlib>
#include <fstream>
#include <vector>
#include <variant>
#include <iostream>

int main(int argc, char** argv)
{
    if (argc == 1)
    {
        throw std::runtime_error("Bad usage, no input file");
    }

    std::ifstream inputFile(argv[1]);

    std::vector<nbuFrontend::Token> tokens = nbuFrontend::lexer(inputFile);

    nbuFrontend::Parser parser(tokens);

    std::vector<nbuFrontend::ASTNode> nodes = parser.parse();

    nbuFrontend::Semantic semantic(nodes);

    auto [errors, warnings] = semantic.semanticAnalyses();

    std::cout << "The analyses ended with " << errors << " errors and " << warnings << " warnings" << std::endl;

    if (errors != 0)
    {
        std::cerr << "Fix errors before recompilling" << std::endl;
        return EXIT_FAILURE;
    }

    //print_tree(semantic.getNodes());

    nbuIR::IRTranslator irTranslator{semantic.getNodes(), semantic.getEnums(), semantic.getGlobals()};

    auto IR = irTranslator.Translate();

    std::cout << nbuIR::irProgramToStr(IR);

    //std::ofstream outputFile("a.out");

    return EXIT_SUCCESS;
}
