include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/src/contrib/SimpleLogger/src)
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
    ../../domain/effects/delay_effect.cpp
    ../../domain/effects/effect_factory.cpp
    ../../domain/devices/device.cpp
    ../../domain/effects/effect.cpp
    ../../domain/dsp/lfo.cpp
    ../../domain/effects/auto_panner_effect.cpp
    ../../domain/effects/effect_rack.cpp
    ../../domain/effects/high_pass_filter_effect.cpp
    ../../domain/effects/low_pass_filter_effect.cpp
    ../../domain/effects/panning_effect.cpp
    ../../domain/effects/panner_effect.cpp
    ../../domain/effects/volume_effect.cpp
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
    ../../domain/dsp/all_pass_filter.cpp
    ../../domain/dsp/chorus_effect.cpp
    ../../domain/tracker/parameter.cpp
    ../../domain/tracker/parameter_container.cpp
    ../../infra/xml/nahd_xml_reader.cpp
    ../../infra/xml/nahd_xml_writer.cpp
)
qt_add_executable(${NAME} ${SRC})
set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${UNIT_TEST_BASE_DIR})
add_test(${NAME} ${UNIT_TEST_BASE_DIR}/${NAME})
target_link_libraries(${NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Gui SimpleLogger PkgConfig::RTMIDI PkgConfig::JACK PkgConfig::SNDFILE PkgConfig::RTAUDIO)
