include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${RTAUDIO_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/src/contrib/SimpleLogger/src)
set(NAME auto_panner_test)
set(SRC
    ${NAME}.cpp
    ${NAME}.hpp
    ../../common/constants.cpp
    ../../common/parameter_mapper.cpp
    ../../common/utils.cpp
    ../../domain/effects/auto_panner_effect.cpp
    ../../domain/effects/effect.cpp
    ../../domain/dsp/lfo.cpp
    ../../domain/dsp/dsp_component.cpp
    ../../domain/parameter.cpp
    ../../domain/parameter_container.cpp
)
qt_add_executable(${NAME} ${SRC})
set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${UNIT_TEST_BASE_DIR})
add_test(${NAME} ${UNIT_TEST_BASE_DIR}/${NAME})
target_link_libraries(${NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Gui SimpleLogger ${RTMIDI_LIBRARIES} ${JACK_LIBRARIES} ${SNDFILE_LIBRARIES} ${RTAUDIO_LIBRARIES})
