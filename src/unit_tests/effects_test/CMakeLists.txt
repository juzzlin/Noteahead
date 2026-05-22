include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/src)
set(NAME effects_test)
set(SRC
    ${NAME}.cpp
    ${NAME}.hpp
    ../../common/constants.cpp
    ../../common/parameter_mapper.cpp
    ../../common/utils.cpp
    ../../domain/devices/delay_effect.cpp
    ../../domain/devices/effect.cpp
    ../../domain/devices/high_pass_filter_effect.cpp
    ../../domain/devices/low_pass_filter_effect.cpp
    ../../domain/devices/panning_effect.cpp
    ../../domain/devices/volume_effect.cpp
    ../../domain/dsp/biquad_filter.cpp
    ../../domain/dsp/cascaded_svf.cpp
    ../../domain/dsp/compressor_effect.cpp
    ../../domain/dsp/dsp_component.cpp
    ../../domain/dsp/eq_8_band_parametric_effect.cpp
    ../../domain/dsp/reverb_effect.cpp
    ../../domain/parameter.cpp
    ../../domain/parameter_container.cpp
)
qt_add_executable(${NAME} ${SRC})
set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${UNIT_TEST_BASE_DIR})
add_test(${NAME} ${UNIT_TEST_BASE_DIR}/${NAME})
target_link_libraries(${NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Test SimpleLogger)
