include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${RTAUDIO_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/src/contrib/SimpleLogger/src)
set(NAME bass_synth_controller_test)
set(SRC
    ${NAME}.cpp
    ${NAME}.hpp
    ../../view/controllers/device_controller.cpp
    ../../view/controllers/bass_synth_controller.cpp
    ../../common/constants.cpp
    ../../common/parameter_mapper.cpp
    ../../common/utils.cpp
    ../../domain/devices/bass_synth_device.cpp
    ../../domain/devices/delay_effect.cpp
    ../../domain/devices/effect_factory.cpp
    ../../domain/devices/device.cpp
    ../../domain/devices/effect.cpp
    ../../domain/dsp/lfo.cpp
    ../../domain/devices/auto_panner_effect.cpp
    ../../domain/devices/effect_rack.cpp
    ../../domain/devices/high_pass_filter_effect.cpp
    ../../domain/devices/low_pass_filter_effect.cpp
    ../../domain/devices/panning_effect.cpp
    ../../domain/devices/panner_effect.cpp
    ../../domain/devices/volume_effect.cpp
    ../../domain/dsp/adsr_envelope.cpp
    ../../domain/dsp/svf_filter.cpp
    ../../domain/dsp/cascaded_svf.cpp
    ../../domain/dsp/diode_ladder_filter.cpp
    ../../domain/dsp/compressor_effect.cpp
    ../../domain/dsp/clipper_effect.cpp
    ../../domain/dsp/dsp_component.cpp
    ../../domain/dsp/eq_8_band_parametric_effect.cpp
    ../../domain/dsp/oversampler.cpp
    ../../domain/dsp/poly_blep_oscillator.cpp
    ../../domain/dsp/reverb_effect.cpp
    ../../domain/dsp/chorus_effect.cpp
    ../../domain/parameter.cpp
    ../../domain/parameter_container.cpp
)
qt_add_executable(${NAME} ${SRC})
set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${UNIT_TEST_BASE_DIR})
add_test(${NAME} ${UNIT_TEST_BASE_DIR}/${NAME})
target_link_libraries(${NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Gui SimpleLogger ${RTMIDI_LIBRARIES} ${JACK_LDFLAGS} ${SNDFILE_LIBRARIES} ${RTAUDIO_LIBRARIES})
