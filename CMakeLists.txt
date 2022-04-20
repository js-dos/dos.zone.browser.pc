project(emulators)
cmake_minimum_required(VERSION 3.10)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/src")

add_definitions(
    -DV8_COMPRESS_POINTERS
    -DV8_REVERSE_JSARGS
    -DX86_64
)

include("${CMAKE_CURRENT_LIST_DIR}/emulators/native/libzip/libzip.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/emulators/native/dosbox-jsdos/dosbox-jsdos.cmake")

include_directories(
    "${CMAKE_CURRENT_LIST_DIR}/emulators/src/protocol"
)

set(SOURCES_PROTOCOL
        "${CMAKE_CURRENT_LIST_DIR}/native/protocol.cpp"
)
set_source_files_properties(${SOURCES_PROTOCOL} PROPERTIES COMPILE_FLAGS "${OPT_FLAGS} -std=c++17")

add_library(${PROJECT_NAME} SHARED
    ${SOURCES_PROTOCOL}
    ${SOURCES_SERVER_JSDOS}
    ${SOURCES_LIBZIP}
    ${CMAKE_JS_SRC})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_JS_INC})
target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB})

# Include node-addon-api wrappers
execute_process(COMMAND node -p "require('node-addon-api').include"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE NODE_ADDON_API_DIR
        )
string(REPLACE "\n" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${NODE_ADDON_API_DIR})