# --- Build options ---
option(MAZE_ENABLE_OPTIMIZATIONS "Enable release optimizations" ON)

if(MAZE_ENABLE_OPTIMIZATIONS)
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -flto")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-s")
else()
    message(STATUS "Release optimizations disabled.")
endif()
