set(HEADER_FILES
    controllers/audio_analysis_controller.hpp
    controllers/automation_refresh_throttle.hpp
    controllers/device_dialog_model.hpp
    controllers/device_rack_controller.hpp
    controllers/drum_pad_dialog_model.hpp
    controllers/effect_rack_controller.hpp
    controllers/fm_drum_operator_controller.hpp
    controllers/fm_operator_controller.hpp
    controllers/knob_controller.hpp
    controllers/nahd_xml_keys.hpp
    controllers/performance_panel_controller.hpp
    controllers/song_overview_controller.hpp
    plugins/effect_summary.hpp
    plugins/plugin_catalog.hpp
    qml/Components/level_meter_renderer.hpp
    qml/Components/oscilloscope_renderer.hpp
    qml/Components/wave_shape_renderer.hpp
    qml/Components/waveform_renderer.hpp
    qml/Dialogs/automation_curve_renderer.hpp
    qml/Dialogs/eq_curve_renderer.hpp
    qml/Dialogs/rta_renderer.hpp
    qml/Dialogs/song_overview_renderer.hpp
    qml/Dialogs/spectrum_compare_renderer.hpp
    qml/Dialogs/stereo_field_renderer.hpp
    qml/Editor/line_number_renderer.hpp
    qml/Editor/note_column_renderer.hpp
)

set(SOURCE_FILES
    controllers/audio_analysis_controller.cpp
    controllers/automation_refresh_throttle.cpp
    controllers/device_dialog_model.cpp
    controllers/device_rack_controller.cpp
    controllers/drum_pad_dialog_model.cpp
    controllers/effect_rack_controller.cpp
    controllers/fm_drum_operator_controller.cpp
    controllers/fm_operator_controller.cpp
    controllers/knob_controller.cpp
    controllers/nahd_xml_keys.cpp
    controllers/performance_panel_controller.cpp
    controllers/song_overview_controller.cpp
    plugins/effect_summary.cpp
    plugins/plugin_catalog.cpp
    qml/Components/level_meter_renderer.cpp
    qml/Components/oscilloscope_renderer.cpp
    qml/Components/wave_shape_renderer.cpp
    qml/Components/waveform_renderer.cpp
    qml/Dialogs/automation_curve_renderer.cpp
    qml/Dialogs/eq_curve_renderer.cpp
    qml/Dialogs/rta_renderer.cpp
    qml/Dialogs/song_overview_renderer.cpp
    qml/Dialogs/spectrum_compare_renderer.cpp
    qml/Dialogs/stereo_field_renderer.cpp
    qml/Editor/line_number_renderer.cpp
    qml/Editor/note_column_renderer.cpp
)

add_library(${ViewLibName} STATIC ${HEADER_FILES} ${SOURCE_FILES})

target_include_directories(${ViewLibName} PUBLIC ..)

target_link_libraries(${ViewLibName} PUBLIC ${CommonLibName} ${DomainLibName} ${ApplicationLibName} Qt6::Core Qt6::Quick)
