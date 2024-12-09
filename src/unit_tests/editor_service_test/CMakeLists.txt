include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(NAME editor_service_test)
set(SRC
    ${NAME}.cpp
    ${NAME}.hpp
    ../../application/editor_service.cpp
    ../../application/note_converter.cpp
    ../../application/position.hpp
    ../../domain/column.cpp
    ../../domain/line.cpp
    ../../domain/pattern.cpp
    ../../domain/song.cpp
    ../../domain/track.cpp
)
add_executable(${NAME} ${SRC} ${MOC_SRC})
set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${UNIT_TEST_BASE_DIR})
add_test(${NAME} ${UNIT_TEST_BASE_DIR}/${NAME})
target_link_libraries(${NAME} Qt${QT_VERSION_MAJOR}::Test SimpleLogger)
