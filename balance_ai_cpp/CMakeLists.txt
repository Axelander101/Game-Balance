cmake_minimum_required(VERSION 3.20)
project(GameBalanceAI VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

add_library(gba_core
    src/core/Json.cpp
    src/core/Model.cpp
    src/core/GameModule.cpp
    src/core/Report.cpp
    src/core/Evolution.cpp
    src/genres/TowerDefenseModule.cpp
    src/genres/ArenaSurvivalModule.cpp
    src/genres/DeckbuilderModule.cpp
    src/genres/RoguelikeModule.cpp
    src/genres/TurnStrategyModule.cpp
    src/genres/AutoBattlerModule.cpp
)

target_include_directories(gba_core PUBLIC include)
target_link_libraries(gba_core PUBLIC Threads::Threads)
if(MSVC)
    target_compile_options(gba_core PRIVATE /W4)
else()
    target_compile_options(gba_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(game_balance_ai src/main.cpp)
target_link_libraries(game_balance_ai PRIVATE gba_core)

include(CTest)
if(BUILD_TESTING)
    add_executable(gba_selftest tests/selftest.cpp)
    target_link_libraries(gba_selftest PRIVATE gba_core)
    target_compile_definitions(gba_selftest PRIVATE GBA_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
    add_test(NAME gba_selftest COMMAND gba_selftest)
endif()
