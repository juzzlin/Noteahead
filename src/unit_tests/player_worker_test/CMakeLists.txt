include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(NAME player_worker_test)
set(SRC
    ${NAME}.cpp
    ${NAME}.hpp
    ../../application/service/player_worker.cpp
    ../../application/service/midi_service.cpp
    ../../application/service/mixer_service.cpp
    ../../application/service/midi_worker_in.cpp
    ../../application/service/midi_worker_out.cpp
    ../../application/service/jack_service.cpp
    ../../application/service/device_service.cpp
    ../../application/service/midi_worker.cpp
    ../../application/instrument_request.cpp
    ../../common/constants.cpp
    ../../common/utils.cpp
    ../../common/waveform_generator.cpp
    ../../domain/devices/device.cpp
    ../../domain/devices/delay_effect.cpp
    ../../domain/devices/high_pass_filter_effect.cpp
    ../../domain/devices/low_pass_filter_effect.cpp
    ../../domain/devices/panning_effect.cpp
    ../../domain/parameter.cpp
    ../../domain/parameter_container.cpp
    ../../domain/devices/sampler_device.cpp
    ../../domain/devices/synth_device.cpp
    ../../domain/devices/synth_presets.cpp
    ../../domain/devices/volume_effect.cpp
    ../../domain/dsp/adsr_envelope.cpp
    ../../domain/dsp/cascaded_svf.cpp
    ../../domain/dsp/lfo.cpp
    ../../domain/dsp/polyblep_oscillator.cpp
    ../../domain/event.cpp
    ../../domain/event_data.cpp
    ../../domain/instrument.cpp
    ../../domain/instrument_settings.cpp
    ../../domain/midi_address.cpp
    ../../domain/midi_cc_data.cpp
    ../../domain/midi_cc_setting.cpp
    ../../domain/midi_note_data.cpp
    ../../domain/note_data.cpp
    ../../domain/pitch_bend_data.cpp
    ../../infra/audio/audio_engine.cpp
    ../../infra/audio/audio_file_recorder.cpp
    ../../infra/audio/audio_file_streamer.cpp
    ../../infra/audio/audio_player.cpp
    ../../infra/audio/audio_recorder.cpp
    ../../infra/audio/backend/sndfile_reader.cpp
    ../../infra/midi/midi_backend.cpp
    ../../infra/midi/midi_backend_in.cpp
    ../../infra/midi/midi_backend_out.cpp
    ../../infra/midi/midi_cc_mapping.cpp
    ../../infra/midi/implementation/librtmidi/midi_in_rt_midi.cpp
    ../../infra/midi/implementation/librtmidi/midi_out_rt_midi.cpp
    ../../infra/midi/midi_port.cpp
    ../../infra/settings.cpp
    ../../application/service/settings_service.cpp
    ../../application/service/random_service.cpp
)

qt_add_executable(${NAME} ${SRC})
set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${UNIT_TEST_BASE_DIR})
add_test(${NAME} ${UNIT_TEST_BASE_DIR}/${NAME})
target_link_libraries(${NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Gui SimpleLogger ${RTMIDI_LIBRARIES} ${RTAUDIO_LIBRARIES} ${JACK_LIBRARIES} ${SNDFILE_LIBRARIES})
if(JACK_FOUND)
  target_compile_definitions(${NAME} PRIVATE HAVE_JACK)
endif()
