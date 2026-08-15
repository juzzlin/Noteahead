set(DSP_SOURCES
    adsr_envelope.cpp
    base_rate_source.cpp
    cascaded_svf.cpp
    compressor_core.cpp
    dc_blocker.cpp
    diode_ladder_filter.cpp
    dsp_component.cpp
    divide_down_generator.cpp
    ensemble_chorus.cpp
    fft.cpp
    lfo.cpp
    linkwitz_riley_crossover.cpp
    multi_engine.cpp
    one_pole_filter.cpp
    ensemble_phaser.cpp
    poly_blep_oscillator.cpp
    svf_filter.cpp
    wavetable.cpp
    wavetable_oscillator.cpp
    drum/clap_engine.cpp
    drum/crash_engine.cpp
    drum/hihat_engine.cpp
    drum/kick_808_engine.cpp
    drum/kick_engine.cpp
    drum/ride_engine.cpp
    drum/snare_engine.cpp
    drum/tom_engine.cpp
)

add_library(Dsp STATIC ${DSP_SOURCES})
target_include_directories(Dsp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)
target_link_libraries(Dsp PRIVATE Qt6::Core)
