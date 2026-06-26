set(HEADER_FILES
    controllers/bass_synth_controller.hpp
    controllers/device_controller.hpp
    controllers/device_rack_controller.hpp
    controllers/drum_synth_controller.hpp
    controllers/effect_rack_controller.hpp
    controllers/knob_controller.hpp
    controllers/piano_synth_controller.hpp
    controllers/sampler_controller.hpp
    controllers/synth_controller.hpp
    controllers/wavetable_synth_controller.hpp
    qml/Editor/line_number_renderer.hpp
    qml/Editor/note_column_renderer.hpp
)

set(SOURCE_FILES
    controllers/bass_synth_controller.cpp
    controllers/device_controller.cpp
    controllers/device_rack_controller.cpp
    controllers/drum_synth_controller.cpp
    controllers/effect_rack_controller.cpp
    controllers/knob_controller.cpp
    controllers/piano_synth_controller.cpp
    controllers/sampler_controller.cpp
    controllers/synth_controller.cpp
    controllers/wavetable_synth_controller.cpp
    qml/Editor/line_number_renderer.cpp
    qml/Editor/note_column_renderer.cpp
)

add_library(${ViewLibName} STATIC ${HEADER_FILES} ${SOURCE_FILES})

target_include_directories(${ViewLibName} PUBLIC ..)

target_link_libraries(${ViewLibName} PUBLIC ${CommonLibName} ${DomainLibName} ${ApplicationLibName} Qt6::Core Qt6::Quick)
