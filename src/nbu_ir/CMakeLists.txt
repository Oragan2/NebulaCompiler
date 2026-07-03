add_library(nbu_ir STATIC
    ir/ir.cpp
    irTranslator/irTranslator.cpp
)

link_libraries(nbu_ir PRIVATE nbu_frontend)

target_include_directories(nbu_ir PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/ir
    ${CMAKE_CURRENT_SOURCE_DIR}/irTranslator
)