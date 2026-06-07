set(EFFECTS_SOURCES
    effect.cpp
    effect_factory.cpp
    effect_rack.cpp
    auto_panner_effect.cpp
    delay_effect.cpp
    high_pass_filter_effect.cpp
    low_pass_filter_effect.cpp
    panner_effect.cpp
    panning_effect.cpp
    volume_effect.cpp
    ../dsp/chorus_effect.cpp
    ../dsp/clipper_effect.cpp
    ../dsp/compressor_effect.cpp
    ../dsp/eq_8_band_parametric_effect.cpp
    ../dsp/reverb_effect.cpp
)

add_library(Effects STATIC ${EFFECTS_SOURCES})
target_include_directories(Effects PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)
target_link_libraries(Effects PRIVATE Qt6::Core Dsp)
