cmake_minimum_required(VERSION 3.19)
project(9cc C)

set(CMAKE_C_STANDARD 11)

add_executable(9cc cc/main.c cc/codegen.c cc/parse.c cc/tokenize.c)
add_executable(cpu cpu/main.c cpu/instruction.c cpu/emulator_function.c cpu/modrm.c cpu/io.c)
