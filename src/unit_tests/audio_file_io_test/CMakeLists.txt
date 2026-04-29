include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(NAME audio_file_io_test)
set(SRC
    ${NAME}.cpp
    ${NAME}.hpp
    ../../infra/audio/audio_file_recorder.cpp
    ../../infra/audio/audio_file_streamer.cpp
    ../../infra/audio/backend/sndfile_reader.cpp
)
qt_add_executable(${NAME} ${SRC})
set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${UNIT_TEST_BASE_DIR})
add_test(${NAME} ${UNIT_TEST_BASE_DIR}/${NAME})
target_link_libraries(${NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Test SimpleLogger ${SNDFILE_LIBRARIES})
