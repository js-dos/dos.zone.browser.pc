project(emulators)
cmake_minimum_required(VERSION 3.10)

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

if (MINGW)
    set(CMAKE_BUILD_TYPE Release)

    add_definitions(
        -DWIN32
    )

    add_library(jsdos SHARED
        "${CMAKE_CURRENT_LIST_DIR}/native/protocol_dyn.cpp"
        ${SOURCES_SERVER_JSDOS}
        ${SOURCES_LIBZIP}
    )
    
    target_link_libraries(jsdos z ws2_32 winmm)
    target_link_options(jsdos PRIVATE -static-libgcc -static-libstdc++ "-Wl,--output-def,${CMAKE_CURRENT_LIST_DIR}/src/libjsdos.def")

    set_target_properties(jsdos PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
    add_custom_command(TARGET jsdos POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:jsdos> "${CMAKE_CURRENT_LIST_DIR}/src/libjsdos.dll"
        COMMAND cp /mingw64/bin/zlib1.dll "${CMAKE_CURRENT_LIST_DIR}/src"
        COMMAND cp /mingw64/bin/libwinpthread-1.dll "${CMAKE_CURRENT_LIST_DIR}/src"
    )
else()
    if (MSVC)
        set_source_files_properties(${SOURCES_PROTOCOL} PROPERTIES COMPILE_FLAGS "${OPT_FLAGS} /std:c++20")

        add_library(${PROJECT_NAME} SHARED
            ${SOURCES_PROTOCOL}
            ${CMAKE_JS_SRC}
        )
        target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB} "${CMAKE_CURRENT_LIST_DIR}/src/libjsdos.lib")

        add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
            COMMAND lib /MACHINE:X64 "/def:${CMAKE_CURRENT_LIST_DIR}/src/libjsdos.def" "/out:${CMAKE_CURRENT_LIST_DIR}/src/libjsdos.lib")

    else()
        set_source_files_properties(${SOURCES_PROTOCOL} PROPERTIES COMPILE_FLAGS "${OPT_FLAGS} -std=c++17")
        
        add_library(${PROJECT_NAME} SHARED
            ${SOURCES_PROTOCOL}
            ${CMAKE_JS_SRC}
            ${SOURCES_SERVER_JSDOS}
            ${SOURCES_LIBZIP}
        )
        target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB})
    endif()

    set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
    target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_JS_INC})

    # Include node-addon-api wrappers
    execute_process(COMMAND node -p "require('node-addon-api').include"
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE NODE_ADDON_API_DIR
            )
    string(REPLACE "\n" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
    string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
    target_include_directories(${PROJECT_NAME} PRIVATE ${NODE_ADDON_API_DIR})

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> "${CMAKE_CURRENT_LIST_DIR}/src/${PROJECT_NAME}.node")
endif()