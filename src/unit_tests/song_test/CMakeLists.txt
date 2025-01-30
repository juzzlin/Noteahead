include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(NAME song_test)
set(SRC
    ${NAME}.cpp
    ${NAME}.hpp
    ../../application/copy_manager.cpp
    ../../application/position.hpp
    ../../domain/column.cpp
    ../../common/constants.cpp
    ../../domain/event.cpp
    ../../domain/instrument.cpp
    ../../domain/line.cpp
    ../../domain/note_data.cpp
    ../../domain/pattern.cpp
    ../../domain/play_order.cpp
    ../../domain/song.cpp
    ../../domain/track.cpp
)
add_executable(${NAME} ${SRC} ${MOC_SRC})
set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${UNIT_TEST_BASE_DIR})
add_test(${NAME} ${UNIT_TEST_BASE_DIR}/${NAME})
target_link_libraries(${NAME} Qt${QT_VERSION_MAJOR}::Test SimpleLogger)
