find_program(CLANG_FORMAT_EXE NAMES clang-format)

if(CLANG_FORMAT_EXE)
    file(GLOB_RECURSE ALL_SOURCE_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    )

    set(STYLE_FILES ${ALL_SOURCE_FILES})
    list(FILTER STYLE_FILES EXCLUDE REGEX "contrib/.*")

    add_custom_target(style
        COMMAND ${CLANG_FORMAT_EXE} -i ${STYLE_FILES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Formatting project files with clang-format"
    )
else()
    message(WARNING "clang-format not found, 'style' target will not be available")
endif()
