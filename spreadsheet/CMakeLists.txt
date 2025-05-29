# Указываем минимальную версию CMake
cmake_minimum_required(VERSION 3.10)

# Форсируем использование Windows 7 SDK
if(WIN32 AND CMAKE_SYSTEM_VERSION VERSION_LESS 10.0)
    set(CMAKE_SYSTEM_VERSION 7.1)
    set(gtest_force_shared_crt OFF)  # Статическая линковка CRT
endif()

# Отключаем предупреждения о совместимости
cmake_policy(SET CMP0135 NEW)







project(spreadsheet)

set(CMAKE_CXX_STANDARD 17)
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(
        CMAKE_CXX_FLAGS_DEBUG
        "${CMAKE_CXX_FLAGS_DEBUG} /JMC"
    )
else()
    set(
        CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Wno-unused-parameter -Wno-implicit-fallthrough"
    )
endif()

set(ANTLR_EXECUTABLE ${CMAKE_CURRENT_SOURCE_DIR}/antlr-4.13.2-complete.jar)
include(${CMAKE_CURRENT_SOURCE_DIR}/FindANTLR.cmake)

add_definitions(
    -DANTLR4CPP_STATIC
    -D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
)


# Отключаем автоматическую загрузку старого googletest (если есть)
set(FETCHCONTENT_QUIET OFF)

# Загружаем актуальную версию googletest
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1  # Последняя стабильная версия, совместимая с Win7
)
FetchContent_MakeAvailable(googletest)

set(WITH_STATIC_CRT OFF CACHE BOOL "Visual C++ static CRT for ANTLR" FORCE)
add_subdirectory(antlr4_runtime)

antlr_target(FormulaParser Formula.g4 LEXER PARSER LISTENER)

include_directories(
    ${ANTLR4_INCLUDE_DIRS}
    ${ANTLR_FormulaParser_OUTPUT_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/antlr4_runtime/runtime/src
)

file(GLOB sources
    *.cpp
    *.h
)

add_executable(
    spreadsheet
    ${ANTLR_FormulaParser_CXX_OUTPUTS}
    ${sources}
)

target_link_libraries(spreadsheet antlr4_static)
if(MSVC)
    target_compile_options(antlr4_static PRIVATE /W0)
endif()

install(
    TARGETS spreadsheet
    DESTINATION bin
    EXPORT spreadsheet
)

set_directory_properties(PROPERTIES VS_STARTUP_PROJECT spreadsheet)