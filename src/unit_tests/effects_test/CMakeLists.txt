include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/src)
set(NAME effects_test)
set(SRC
    ${NAME}.cpp
    ${NAME}.hpp
    ../../common/constants.cpp
    ../../common/parameter_mapper.cpp
    ../../common/utils.cpp
    ../../domain/effects/delay.cpp
    ../../domain/effects/effect.cpp
    ../../domain/effects/high_pass_filter.cpp
    ../../domain/effects/low_pass_filter.cpp
    ../../domain/effects/panning.cpp
    ../../domain/dsp/true_stereo_panner.cpp
    ../../domain/effects/volume.cpp
    ../../domain/dsp/svf_filter.cpp
    ../../domain/dsp/cascaded_svf.cpp
    ../../domain/dsp/diode_ladder_filter.cpp
    ../../domain/dsp/compressor.cpp
    ../../domain/dsp/clipper.cpp
    ../../domain/dsp/saturator.cpp
    ../../domain/dsp/dsp_component.cpp
    ../../domain/dsp/eq_8_band_parametric.cpp
    ../../domain/dsp/lfo.cpp
    ../../domain/dsp/reverb.cpp
    ../../domain/dsp/chorus.cpp
    ../../domain/tracker/parameter.cpp
    ../../domain/tracker/parameter_container.cpp
)
qt_add_executable(${NAME} ${SRC})
set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${UNIT_TEST_BASE_DIR})
add_test(${NAME} ${UNIT_TEST_BASE_DIR}/${NAME})
target_link_libraries(${NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Test SimpleLogger)
