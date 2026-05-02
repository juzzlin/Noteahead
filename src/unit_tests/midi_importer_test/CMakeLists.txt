cmake_minimum_required(VERSION 3.16)

set(NAME midi_importer_test)

set(SRC
    ${NAME}.cpp
    ../../application/position.hpp
    ../../application/service/automation_service.cpp
    ../../application/service/device_service.cpp
    ../../application/service/property_service.cpp
    ../../application/service/copy_manager.cpp
    ../../application/service/mixer_service.cpp
    ../../application/service/random_service.cpp
    ../../application/service/side_chain_service.cpp
    ../../application/service/midi_service.cpp
    ../../application/service/midi_worker_in.cpp
    ../../application/service/midi_worker_out.cpp
    ../../application/service/jack_service.cpp
    ../../application/service/midi_worker.cpp
    ../../application/service/settings_service.cpp
    ../../application/instrument_request.cpp
    ../../domain/midi_note_data.cpp
    ../../common/constants.cpp
    ../../common/utils.cpp
    ../../common/waveform_generator.cpp
    ../../domain/arpeggiator.cpp
    ../../domain/automation.cpp
    ../../domain/automation_location.cpp
    ../../domain/column.cpp
    ../../domain/column_settings.cpp
    ../../domain/devices/device.cpp
    ../../domain/devices/high_pass_filter_effect.cpp
    ../../domain/devices/low_pass_filter_effect.cpp
    ../../domain/devices/panning_effect.cpp
    ../../domain/parameter.cpp
    ../../domain/parameter_container.cpp
    ../../domain/devices/sampler_device.cpp
    ../../domain/devices/volume_effect.cpp
    ../../domain/event.cpp
    ../../domain/event_data.cpp
    ../../domain/instrument.cpp
    ../../domain/instrument_settings.cpp
    ../../domain/interpolator.cpp
    ../../domain/line.cpp
    ../../domain/line_event.cpp
    ../../domain/midi_address.cpp
    ../../domain/midi_cc_automation.cpp
    ../../domain/midi_cc_data.cpp
    ../../domain/midi_cc_setting.cpp
    ../../domain/mixer_unit.cpp
    ../../domain/note_data.cpp
    ../../domain/note_data_manipulator.cpp
    ../../domain/pattern.cpp
    ../../domain/pitch_bend_automation.cpp
    ../../domain/pitch_bend_data.cpp
    ../../domain/play_order.cpp
    ../../domain/song.cpp
    ../../domain/track.cpp
    ../../infra/audio/audio_engine.cpp
    ../../infra/audio/audio_file_recorder.cpp
    ../../infra/audio/audio_file_streamer.cpp
    ../../infra/audio/audio_player.cpp
    ../../infra/audio/audio_recorder.cpp
    ../../infra/audio/backend/sndfile_reader.cpp
    ../../infra/midi/midi_backend.cpp
    ../../infra/midi/midi_backend_in.cpp
    ../../infra/midi/midi_backend_out.cpp
    ../../infra/midi/implementation/librtmidi/midi_in_rt_midi.cpp
    ../../infra/midi/implementation/librtmidi/midi_out_rt_midi.cpp
    ../../infra/midi/midi_port.cpp
    ../../infra/settings.cpp
    ../../infra/midi/export/midi_exporter.cpp
    ../../infra/midi/import/midi_importer.cpp
    ../../infra/midi/midi_cc_mapping.cpp
)

qt_add_executable(${NAME} ${SRC})
set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${UNIT_TEST_BASE_DIR})
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../..)
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../contrib/SimpleLogger/src)
target_include_directories(${NAME} PRIVATE ${RTMIDI_INCLUDE_DIRS} ${RTAUDIO_INCLUDE_DIRS} ${JACK_INCLUDE_DIRS} ${SNDFILE_INCLUDE_DIRS})
target_link_libraries(${NAME} PRIVATE Qt6::Test Qt6::Gui SimpleLogger ${RTMIDI_LIBRARIES} ${RTAUDIO_LIBRARIES} ${JACK_LIBRARIES} ${SNDFILE_LIBRARIES})
if(JACK_FOUND)
  target_compile_definitions(${NAME} PRIVATE HAVE_JACK)
endif()
add_test(NAME ${NAME} COMMAND ${NAME})
