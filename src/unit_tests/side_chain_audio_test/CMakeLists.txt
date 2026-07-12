include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/src/contrib/SimpleLogger/src)

set(TEST_NAME side_chain_audio_test)

set(SOURCE_FILES
    ${TEST_NAME}.cpp
    ${TEST_NAME}.hpp
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
    ../../domain/dsp/oversampler.cpp
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
    ../../domain/tracker/mixer_unit.cpp
    ../../domain/tracker/parameter.cpp
    ../../domain/tracker/parameter_container.cpp
    ../../domain/utility/dbtp_meter.cpp
    ../../domain/utility/lufs_meter.cpp
    ../../domain/utility/rta.cpp
    ../../infra/audio/audio_engine.cpp
    ../../infra/audio/real_time_worker_pool.cpp
    ../../infra/xml/nahd_xml_reader.cpp
    ../../infra/xml/nahd_xml_writer.cpp
)

qt_add_executable(${TEST_NAME} ${SOURCE_FILES})
set_target_properties(${TEST_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${UNIT_TEST_BASE_DIR})
add_test(${TEST_NAME} ${UNIT_TEST_BASE_DIR}/${TEST_NAME})

target_link_libraries(${TEST_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Test
    Qt${QT_VERSION_MAJOR}::Xml
    SimpleLogger
)
