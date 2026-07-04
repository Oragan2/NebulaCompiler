add_executable(nbu main.cpp)

target_link_libraries(nbu PRIVATE nbu_frontend)
target_link_libraries(nbu PRIVATE nbu_ir)