add_library(nbu_backend STATIC
    codeGen/codegen.cpp
)

link_libraries(nbu_backend PRIVATE nbu_frontend)

target_include_directories(nbu_backend PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/codeGen
)