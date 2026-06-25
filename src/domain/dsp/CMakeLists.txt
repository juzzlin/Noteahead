set(DSP_SOURCES
    adsr_envelope.cpp
    cascaded_svf.cpp
    dc_blocker.cpp
    diode_ladder_filter.cpp
    dsp_component.cpp
    lfo.cpp
    multi_engine.cpp
    oversampler.cpp
    poly_blep_oscillator.cpp
    svf_filter.cpp
    wavetable.cpp
    wavetable_oscillator.cpp
    drum/clap_engine.cpp
    drum/crash_engine.cpp
    drum/hihat_engine.cpp
    drum/kick_engine.cpp
    drum/ride_engine.cpp
    drum/snare_engine.cpp
    drum/tom_engine.cpp
)

add_library(Dsp STATIC ${DSP_SOURCES})
target_include_directories(Dsp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)
target_link_libraries(Dsp PRIVATE Qt6::Core)
