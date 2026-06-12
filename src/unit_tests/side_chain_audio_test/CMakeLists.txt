include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/src/contrib/SimpleLogger/src)

set(TEST_NAME side_chain_audio_test)

set(SOURCE_FILES
    ${TEST_NAME}.cpp
    ${TEST_NAME}.hpp
    ../../common/constants.cpp
    ../../common/parameter_mapper.cpp
    ../../common/utils.cpp
    ../../domain/tracker/parameter.cpp
    ../../domain/tracker/parameter_container.cpp
    ../../domain/tracker/mixer_unit.cpp
    ../../domain/devices/device.cpp
    ../../domain/effects/effect.cpp
    ../../domain/effects/effect_rack.cpp
    ../../domain/effects/effect_factory.cpp
    ../../domain/dsp/dsp_component.cpp
    ../../domain/dsp/compressor_effect.cpp
    ../../domain/dsp/svf_filter.cpp
    ../../domain/dsp/cascaded_svf.cpp
    ../../domain/dsp/diode_ladder_filter.cpp
    ../../domain/dsp/reverb_effect.cpp
    ../../domain/dsp/chorus_effect.cpp
    ../../domain/dsp/clipper_effect.cpp
    ../../domain/dsp/eq_8_band_parametric_effect.cpp
    ../../domain/dsp/lfo.cpp
    ../../domain/dsp/oversampler.cpp
    ../../domain/effects/auto_panner_effect.cpp
    ../../domain/effects/delay_effect.cpp
    ../../domain/effects/high_pass_filter_effect.cpp
    ../../domain/effects/low_pass_filter_effect.cpp
    ../../domain/effects/panner_effect.cpp
    ../../domain/effects/panning_effect.cpp
    ../../domain/effects/volume_effect.cpp
    ../../infra/audio/audio_engine.cpp
    ../../infra/audio/real_time_worker_pool.cpp
)

qt_add_executable(${TEST_NAME} ${SOURCE_FILES})
set_target_properties(${TEST_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${UNIT_TEST_BASE_DIR})
add_test(${TEST_NAME} ${UNIT_TEST_BASE_DIR}/${TEST_NAME})

target_link_libraries(${TEST_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Test
    Qt${QT_VERSION_MAJOR}::Xml
    SimpleLogger
)
