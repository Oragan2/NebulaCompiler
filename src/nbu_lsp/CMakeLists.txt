add_executable(nbu-lsp lsp_main.cpp)

target_link_libraries(nbu-lsp PRIVATE nbu_frontend)