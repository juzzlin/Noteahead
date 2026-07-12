include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/src/contrib/SimpleLogger/src)
set(NAME string_voice_test)
set(SRC
    ${NAME}.cpp
    ${NAME}.hpp
    ../../common/constants.cpp
    ../../common/parameter_mapper.cpp
    ../../common/utils.cpp
    ../../domain/devices/device.cpp
    ../../domain/devices/string_voice_device.cpp
    ../../domain/dsp/adsr_envelope.cpp
    ../../domain/effects/all_pass_filter.cpp
    ../../domain/dsp/cascaded_svf.cpp
    ../../domain/effects/chorus.cpp
    ../../domain/effects/clipper.cpp
    ../../domain/effects/compressor.cpp
    ../../domain/utility/dbtp_meter.cpp
    ../../domain/dsp/dsp_component.cpp
    ../../domain/dsp/ensemble_chorus.cpp
    ../../domain/effects/eq_8_band_parametric.cpp
    ../../domain/dsp/lfo.cpp
    ../../domain/utility/lufs_meter.cpp
    ../../domain/dsp/oversampler.cpp
    ../../domain/dsp/poly_blep_oscillator.cpp
    ../../domain/effects/reverb.cpp
    ../../domain/utility/rta.cpp
    ../../domain/effects/saturator.cpp
    ../../domain/dsp/svf_filter.cpp
    ../../domain/dsp/true_stereo_panner.cpp
    ../../domain/effects/auto_panner.cpp
    ../../domain/effects/delay.cpp
    ../../domain/effects/effect.cpp
    ../../domain/effects/effect_factory.cpp
    ../../domain/effects/effect_rack.cpp
    ../../domain/dsp/high_pass_filter.cpp
    ../../domain/dsp/low_pass_filter.cpp
    ../../domain/effects/panner.cpp
    ../../domain/dsp/panning.cpp
    ../../domain/dsp/volume.cpp
    ../../domain/tracker/parameter.cpp
    ../../domain/tracker/parameter_container.cpp
    ../../infra/xml/nahd_xml_reader.cpp
    ../../infra/xml/nahd_xml_writer.cpp
)
qt_add_executable(${NAME} ${SRC})
set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${UNIT_TEST_BASE_DIR})
add_test(${NAME} ${UNIT_TEST_BASE_DIR}/${NAME})
target_link_libraries(${NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Gui SimpleLogger PkgConfig::RTMIDI PkgConfig::JACK PkgConfig::SNDFILE PkgConfig::RTAUDIO)
