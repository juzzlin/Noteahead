include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${RTAUDIO_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/src/contrib/SimpleLogger/src)
set(NAME bass_synth_test)
set(SRC
    ${NAME}.cpp
    ${NAME}.hpp
    ../../common/constants.cpp
    ../../common/parameter_mapper.cpp
    ../../common/utils.cpp
    ../../domain/parameter.cpp
    ../../domain/parameter_container.cpp
    ../../domain/devices/device.cpp
    ../../domain/devices/effect.cpp
    ../../domain/devices/bass_synth_device.cpp
    ../../domain/dsp/oversampler.cpp
    ../../domain/dsp/adsr_envelope.cpp
    ../../domain/dsp/cascaded_svf.cpp
    ../../domain/dsp/dsp_component.cpp
    ../../domain/dsp/polyblep_oscillator.cpp
)
qt_add_executable(${NAME} ${SRC})
set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${UNIT_TEST_BASE_DIR})
add_test(${NAME} ${UNIT_TEST_BASE_DIR}/${NAME})
target_link_libraries(${NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Gui SimpleLogger ${RTMIDI_LIBRARIES} ${JACK_LIBRARIES} ${SNDFILE_LIBRARIES} ${RTAUDIO_LIBRARIES})
