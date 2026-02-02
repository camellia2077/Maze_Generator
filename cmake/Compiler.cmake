# --- Compiler configuration ---
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set(CMAKE_CXX_COMPILER_LAUNCHER ccache)
    message(STATUS "ccache found, will be used for compilation.")
else()
    message(STATUS "ccache not found, proceeding without it.")
endif()
