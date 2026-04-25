include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(NAME sampler_test)
set(SRC
    ${NAME}.cpp
    ${NAME}.hpp
    ../../application/command/automation_command.cpp
    ../../application/command/composite_command.cpp
    ../../application/command/note_edit_command.cpp
    ../../application/command/undo_stack.cpp
    ../../application/instrument_request.cpp
    ../../application/note_converter.cpp
    ../../application/service/application_service.cpp
    ../../application/service/copy_manager.cpp
    ../../application/service/device_service.cpp
    ../../application/service/editor_service.cpp
    ../../application/service/mixer_service.cpp
    ../../application/service/side_chain_service.cpp
    ../../application/state_machine.cpp
    ../../common/constants.cpp
    ../../common/utils.cpp
    ../../common/waveform_generator.cpp
    ../../domain/column.cpp
    ../../domain/devices/device.cpp
    ../../domain/devices/sampler_device.cpp
    ../../domain/instrument.cpp
    ../../domain/instrument_settings.cpp
    ../../domain/interpolator.cpp
    ../../domain/line.cpp
    ../../domain/line_event.cpp
    ../../domain/midi_address.cpp
    ../../domain/midi_cc_setting.cpp
    ../../domain/midi_note_data.cpp
    ../../domain/mixer_unit.cpp
    ../../domain/note_data.cpp
    ../../domain/note_data_manipulator.cpp
    ../../domain/pattern.cpp
    ../../domain/play_order.cpp
    ../../domain/song.cpp
    ../../domain/track.cpp
    ../../infra/audio/audio_engine.cpp
    ../../infra/audio/backend/sndfile_reader.cpp
    ../../infra/midi/midi_cc_mapping.cpp
    ../../infra/settings.cpp
)
qt_add_executable(${NAME} ${SRC})
set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${UNIT_TEST_BASE_DIR})
add_test(${NAME} ${UNIT_TEST_BASE_DIR}/${NAME})
target_link_libraries(${NAME} PRIVATE ${SNDFILE_LIBRARIES} Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Test SimpleLogger noteahead_audio_backend)
