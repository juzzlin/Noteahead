include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(NAME device_rack_controller_test)

set(SRC
    ${NAME}.cpp
    ${NAME}.hpp
    ../../application/command/automation_command.cpp
    ../../application/command/composite_command.cpp
    ../../application/command/note_edit_command.cpp
    ../../application/command/undo_stack.cpp
    ../../application/instrument_request.cpp
    ../../application/models/sampler/sampler_pad_model.cpp
    ../../application/note_converter.cpp
    ../../application/position.cpp
    ../../application/service/automation_service.cpp
    ../../application/service/bass_synth_controller.cpp
    ../../application/service/copy_manager.cpp
    ../../application/service/device_rack_controller.cpp
    ../../application/service/device_service.cpp
    ../../application/service/drum_synth_controller.cpp
    ../../application/service/editor_service.cpp
    ../../application/service/mixer_service.cpp
    ../../application/service/property_service.cpp
    ../../application/service/random_service.cpp
    ../../application/service/sampler_controller.cpp
    ../../application/service/selection_service.cpp
    ../../application/service/settings_service.cpp
    ../../application/service/side_chain_service.cpp
    ../../application/service/synth_controller.cpp
    ../../common/constants.cpp
    ../../common/parameter_mapper.cpp
    ../../common/utils.cpp
    ../../common/waveform_generator.cpp
    ../../domain/arpeggiator.cpp
    ../../domain/automation.cpp
    ../../domain/automation_location.cpp
    ../../domain/column.cpp
    ../../domain/column_settings.cpp
    ../../domain/devices/bass_synth_device.cpp
    ../../domain/devices/delay_effect.cpp
    ../../domain/devices/device.cpp
    ../../domain/devices/drum_synth_constants.cpp
    ../../domain/devices/drum_synth_device.cpp
    ../../domain/devices/effect.cpp
    ../../domain/devices/effect_rack.cpp
    ../../domain/devices/high_pass_filter_effect.cpp
    ../../domain/devices/low_pass_filter_effect.cpp
    ../../domain/devices/panning_effect.cpp
    ../../domain/devices/sampler_device.cpp
    ../../domain/devices/synth_device.cpp
    ../../domain/devices/synth_presets.cpp
    ../../domain/devices/volume_effect.cpp
    ../../domain/dsp/adsr_envelope.cpp
    ../../domain/dsp/svf_filter.cpp
    ../../domain/dsp/cascaded_svf.cpp
    ../../domain/dsp/compressor_effect.cpp
    ../../domain/dsp/drum/clap_engine.cpp
    ../../domain/dsp/drum/crash_engine.cpp
    ../../domain/dsp/drum/hihat_engine.cpp
    ../../domain/dsp/drum/kick_engine.cpp
    ../../domain/dsp/drum/ride_engine.cpp
    ../../domain/dsp/drum/snare_engine.cpp
    ../../domain/dsp/drum/tom_engine.cpp
    ../../domain/dsp/dsp_component.cpp
    ../../domain/dsp/eq_8_band_parametric_effect.cpp
    ../../domain/dsp/lfo.cpp
    ../../domain/dsp/multi_engine.cpp
    ../../domain/dsp/oversampler.cpp
    ../../domain/dsp/polyblep_oscillator.cpp
    ../../domain/dsp/reverb_effect.cpp
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
    ../../domain/midi_note_data.cpp
    ../../domain/mixer_unit.cpp
    ../../domain/note_data.cpp
    ../../domain/note_data_manipulator.cpp
    ../../domain/parameter.cpp
    ../../domain/parameter_container.cpp
    ../../domain/pattern.cpp
    ../../domain/pitch_bend_automation.cpp
    ../../domain/pitch_bend_data.cpp
    ../../domain/play_order.cpp
    ../../domain/song.cpp
    ../../domain/track.cpp
    ../../infra/audio/audio_engine.cpp
    ../../infra/audio/backend/sndfile_reader.cpp
    ../../infra/audio/real_time_worker_pool.cpp
    ../../infra/midi/midi_cc_mapping.cpp
    ../../infra/settings.cpp
)

qt_add_executable(${NAME} ${SRC})

set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${UNIT_TEST_BASE_DIR})

add_test(${NAME} ${UNIT_TEST_BASE_DIR}/${NAME})

target_link_libraries(${NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Test
    SimpleLogger
    ${SNDFILE_LIBRARIES}
)
