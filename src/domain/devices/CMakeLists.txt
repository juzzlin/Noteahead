set(EFFECTS_SOURCES
    effect.cpp
    effect_factory.cpp
    effect_rack.cpp
    auto_panner.cpp
    delay.cpp
    high_pass_filter.cpp
    low_pass_filter.cpp
    panner.cpp
    panning.cpp
    volume.cpp
    ../dsp/chorus.cpp
    ../dsp/clipper.cpp
    ../dsp/compressor.cpp
    ../dsp/eq_8_band_parametric.cpp
    ../dsp/reverb.cpp
)

add_library(Effects STATIC ${EFFECTS_SOURCES})
target_include_directories(Effects PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)
target_link_libraries(Effects PRIVATE Qt6::Core Dsp)
