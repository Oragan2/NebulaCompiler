add_library(nbu_frontend STATIC
    lexer/lexer.cpp
    parser/parser.cpp
    semanticAnalyser/semantic.cpp
)

# Tell anyone linking to this library where to find the header files automatically
target_include_directories(nbu_frontend PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/lexer
    ${CMAKE_CURRENT_SOURCE_DIR}/parser
    ${CMAKE_CURRENT_SOURCE_DIR}/semanticAnalyser
)