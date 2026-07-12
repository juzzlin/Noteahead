include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/src/contrib/SimpleLogger/src)
set(NAME panner_test)
set(SRC
    ${NAME}.cpp
    ${NAME}.hpp
    ../../common/constants.cpp
    ../../common/parameter_mapper.cpp
    ../../common/utils.cpp
    ../../domain/devices/device.cpp
    ../../domain/dsp/cascaded_svf.cpp
    ../../domain/dsp/diode_ladder_filter.cpp
    ../../domain/dsp/dsp_component.cpp
    ../../domain/dsp/high_pass_filter.cpp
    ../../domain/dsp/lfo.cpp
    ../../domain/dsp/low_pass_filter.cpp
    ../../domain/dsp/panning.cpp
    ../../domain/dsp/svf_filter.cpp
    ../../domain/dsp/true_stereo_panner.cpp
    ../../domain/dsp/volume.cpp
    ../../domain/effects/all_pass_filter.cpp
    ../../domain/effects/auto_panner.cpp
    ../../domain/effects/chorus.cpp
    ../../domain/effects/clipper.cpp
    ../../domain/effects/compressor.cpp
    ../../domain/effects/delay.cpp
    ../../domain/effects/effect.cpp
    ../../domain/effects/effect_factory.cpp
    ../../domain/effects/effect_rack.cpp
    ../../domain/effects/eq_8_band_parametric.cpp
    ../../domain/effects/panner.cpp
    ../../domain/effects/reverb.cpp
    ../../domain/effects/saturator.cpp
    ../../domain/tracker/parameter.cpp
    ../../domain/tracker/parameter_container.cpp
    ../../domain/utility/dbtp_meter.cpp
    ../../domain/utility/lufs_meter.cpp
    ../../domain/utility/rta.cpp
)
qt_add_executable(${NAME} ${SRC})
set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${UNIT_TEST_BASE_DIR})
add_test(${NAME} ${UNIT_TEST_BASE_DIR}/${NAME})
target_link_libraries(${NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Gui SimpleLogger PkgConfig::RTMIDI PkgConfig::JACK PkgConfig::SNDFILE PkgConfig::RTAUDIO)
