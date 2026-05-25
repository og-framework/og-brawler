# SPDX-License-Identifier: BUSL-1.1

file(GLOB_RECURSE OGBRAWLER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

# UE-only file: the IMPLEMENT_MODULE stub doesn't compile outside UBT.
list(REMOVE_ITEM OGBRAWLER_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/OGBrawler.cpp"
)

add_library(og_brawler STATIC ${OGBRAWLER_SOURCES})

target_include_directories(og_brawler PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(og_brawler PUBLIC og_simulation glm::glm-header-only)
