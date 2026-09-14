cmake_minimum_required(VERSION 3.25)
project(nba_live_10 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated/rexglue.cmake)
include(../cmake/LiveRecomp.cmake)

live_recomp_add_game(nba_live_10
    OUTPUT_NAME "NBA LIVE 10"
    SOURCES
        src/main.cpp
    SETTINGS "${CMAKE_CURRENT_SOURCE_DIR}/settings/nba_live_10.toml"
    ICON "${CMAKE_CURRENT_SOURCE_DIR}/metadata/nba_live_10.ico"
    RESOURCE "${CMAKE_CURRENT_SOURCE_DIR}/resources/nba_live_10.rc"
    DEVELOPMENT_GAME_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/assets"
)
