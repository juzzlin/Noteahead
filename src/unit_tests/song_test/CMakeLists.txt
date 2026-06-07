include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(NAME song_test)
set(SRC
    ${NAME}.cpp
    ${NAME}.hpp
    ../../application/position.hpp
    ../../application/service/automation_service.cpp
    ../../application/service/property_service.cpp
    ../../application/service/copy_manager.cpp
    ../../application/service/random_service.cpp
    ../../application/service/side_chain_service.cpp
    ../../common/constants.cpp
    ../../common/parameter_mapper.cpp
    ../../common/utils.cpp
    ../../domain/tracker/arpeggiator.cpp
    ../../domain/tracker/automation.cpp
    ../../domain/tracker/automation_location.cpp
    ../../domain/tracker/column.cpp
    ../../domain/tracker/column_settings.cpp
    ../../domain/devices/drum_synth_constants.cpp
    ../../domain/devices/synth_presets.cpp
    ../../domain/tracker/event.cpp
    ../../domain/tracker/event_data.cpp
    ../../domain/tracker/instrument.cpp
    ../../domain/tracker/instrument_settings.cpp
    ../../domain/tracker/interpolator.cpp
    ../../domain/tracker/line.cpp
    ../../domain/tracker/line_event.cpp
    ../../domain/midi/midi_address.cpp
    ../../domain/midi/midi_cc_automation.cpp
    ../../domain/midi/midi_cc_data.cpp
    ../../domain/midi/midi_cc_setting.cpp
    ../../domain/tracker/mixer_unit.cpp
    ../../domain/tracker/note_data.cpp
    ../../domain/tracker/pattern.cpp
    ../../domain/midi/pitch_bend_automation.cpp
    ../../domain/midi/pitch_bend_data.cpp
    ../../domain/tracker/play_order.cpp
    ../../domain/tracker/song.cpp
    ../../domain/tracker/track.cpp
    ../../infra/midi/midi_cc_mapping.cpp
)
qt_add_executable(${NAME} ${SRC})
set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${UNIT_TEST_BASE_DIR})
add_test(${NAME} ${UNIT_TEST_BASE_DIR}/${NAME})
target_link_libraries(${NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Test SimpleLogger)
