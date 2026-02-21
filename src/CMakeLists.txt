add_subdirectory(contrib/SimpleLogger EXCLUDE_FROM_ALL)
add_subdirectory(contrib/Argengine EXCLUDE_FROM_ALL)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# These are needed only for the IDE
set(HEADER_FILES
  application/application.hpp
  application/instrument_request.hpp
  application/command/note_edit_command.hpp
  application/command/undo_stack.hpp
  application/models/audio_settings_model.hpp
  application/models/column_settings_model.hpp
  application/models/event_selection_model.hpp
  application/models/midi_cc_automations_model.hpp
  application/models/midi_cc_selection_model.hpp
  application/models/midi_settings_model.hpp
  application/models/note_column_line_container_helper.hpp
  application/models/note_column_model.hpp
  application/models/note_column_model_handler.hpp
  application/models/pitch_bend_automations_model.hpp
  application/models/recent_files_model.hpp
  application/models/track_settings_model.hpp
  application/note_converter.hpp
  application/position.hpp
  application/service/application_service.hpp
  application/service/audio_service.hpp
  application/service/audio_worker.hpp
  application/service/automation_service.hpp
  application/service/copy_manager.hpp
  application/service/editor_service.hpp
  application/service/keyboard_service.hpp
  application/service/midi_service.hpp
  application/service/midi_worker.hpp
  application/service/midi_worker_in.hpp
  application/service/midi_worker_out.hpp
  application/service/mixer_service.hpp
  application/service/player_service.hpp
  application/service/player_worker.hpp
  application/service/property_service.hpp
  application/service/random_service.hpp
  application/service/recent_files_manager.hpp
  application/service/selection_service.hpp
  application/service/settings_service.hpp
  application/service/side_chain_service.hpp
  application/service/util_service.hpp
  application/state_machine.hpp
  application/ui_logger.hpp
  common/constants.hpp
  common/utils.hpp
  domain/automation.hpp
  domain/automation_location.hpp
  domain/column.hpp
  domain/column_settings.hpp
  domain/event.hpp
  domain/event_data.hpp
  domain/instrument.hpp
  domain/instrument_settings.hpp
  domain/interpolator.hpp
  domain/line.hpp
  domain/line_event.hpp
  domain/midi_address.hpp
  domain/midi_cc_automation.hpp
  domain/midi_cc_data.hpp
  domain/midi_cc_setting.hpp
  domain/midi_note_data.hpp
  domain/mixer_unit.hpp
  domain/note_data.hpp
  domain/note_data_manipulator.hpp
  domain/pattern.hpp
  domain/pitch_bend_automation.hpp
  domain/pitch_bend_data.hpp
  domain/play_order.hpp
  domain/side_chain_settings.hpp
  domain/song.hpp
  domain/track.hpp
  infra/audio/audio_recorder.hpp
  infra/audio/implementation/librtaudio/audio_recorder_rt_audio.hpp
  infra/audio/ring_buffer.hpp
  infra/midi/export/midi_exporter.hpp
  infra/midi/import/midi_importer.hpp
  infra/midi/implementation/librtmidi/midi_in_rt_midi.hpp
  infra/midi/implementation/librtmidi/midi_out_rt_midi.hpp
  infra/midi/midi_backend.hpp
  infra/midi/midi_backend_in.hpp
  infra/midi/midi_backend_out.hpp
  infra/midi/midi_cc_mapping.hpp
  infra/midi/midi_port.hpp
  infra/settings.hpp
  infra/video/animation.hpp
  infra/video/bars_animation.hpp
  infra/video/default_animation.hpp
  infra/video/video_config.hpp
  infra/video/video_generator.hpp
)

set(SOURCE_FILES
  application/application.cpp
  application/instrument_request.cpp
  application/command/note_edit_command.cpp
  application/command/undo_stack.cpp
  application/models/audio_settings_model.cpp
  application/models/column_settings_model.cpp
  application/models/event_selection_model.cpp
  application/models/midi_cc_automations_model.cpp
  application/models/midi_cc_selection_model.cpp
  application/models/midi_settings_model.cpp
  application/models/note_column_line_container_helper.cpp
  application/models/note_column_model.cpp
  application/models/note_column_model_handler.cpp
  application/models/pitch_bend_automations_model.cpp
  application/models/recent_files_model.cpp
  application/models/track_settings_model.cpp
  application/note_converter.cpp
  application/service/application_service.cpp
  application/service/audio_service.cpp
  application/service/audio_worker.cpp
  application/service/automation_service.cpp
  application/service/copy_manager.cpp
  application/service/editor_service.cpp
  application/service/keyboard_service.cpp
  application/service/midi_service.cpp
  application/service/midi_worker.cpp
  application/service/midi_worker_in.cpp
  application/service/midi_worker_out.cpp
  application/service/mixer_service.cpp
  application/service/player_service.cpp
  application/service/player_worker.cpp
  application/service/property_service.cpp
  application/service/random_service.cpp
  application/service/recent_files_manager.cpp
  application/service/selection_service.cpp
  application/service/settings_service.cpp
  application/service/side_chain_service.cpp
  application/service/util_service.cpp
  application/state_machine.cpp
  application/ui_logger.cpp
  common/constants.cpp
  common/utils.cpp
  domain/automation.cpp
  domain/automation_location.cpp
  domain/column.cpp
  domain/column_settings.cpp
  domain/event.cpp
  domain/event_data.cpp
  domain/instrument.cpp
  domain/instrument_settings.cpp
  domain/interpolator.cpp
  domain/line.cpp
  domain/line_event.cpp
  domain/midi_address.cpp
  domain/midi_cc_automation.cpp
  domain/midi_cc_data.cpp
  domain/midi_cc_setting.cpp
  domain/midi_note_data.cpp
  domain/mixer_unit.cpp
  domain/note_data.cpp
  domain/note_data_manipulator.cpp
  domain/pattern.cpp
  domain/pitch_bend_automation.cpp
  domain/pitch_bend_data.cpp
  domain/play_order.cpp
  domain/song.cpp
  domain/track.cpp
  infra/audio/audio_recorder.cpp
  infra/audio/implementation/librtaudio/audio_recorder_rt_audio.cpp
  infra/midi/export/midi_exporter.cpp
  infra/midi/import/midi_importer.cpp
  infra/midi/implementation/librtmidi/midi_in_rt_midi.cpp
  infra/midi/implementation/librtmidi/midi_out_rt_midi.cpp
  infra/midi/midi_backend.cpp
  infra/midi/midi_backend_in.cpp
  infra/midi/midi_backend_out.cpp
  infra/midi/midi_cc_mapping.cpp
  infra/midi/midi_port.cpp
  infra/settings.cpp
  infra/video/animation.cpp
  infra/video/bars_animation.cpp
  infra/video/default_animation.cpp
  infra/video/video_generator.cpp
  main.cpp
)

qt_add_executable(${BINARY_NAME} ${HEADER_FILES} ${SOURCE_FILES})

set(URI ${BINARY_NAME})
set(QML_BASE_DIR view/qml)
if(QT_VERSION_MAJOR EQUAL 6 AND QT_VERSION_MINOR GREATER_EQUAL 5)
  set(QML_ROOT_DIR "qrc:/qt/qml/${URI}/${QML_BASE_DIR}")
else()
  set(QML_ROOT_DIR "qrc:/${URI}/${QML_BASE_DIR}")
endif()
set(MAIN Main.qml)
set(QML_SOURCE_FILES
    ${QML_BASE_DIR}/${MAIN}
    ${QML_BASE_DIR}/BottomBar.qml
    ${QML_BASE_DIR}/Components/LayoutSeparator.qml
    ${QML_BASE_DIR}/Components/MenuItemDelegate.qml
    ${QML_BASE_DIR}/Components/MidiCcComboBox.qml
    ${QML_BASE_DIR}/Components/VirtualKeyboard.qml
    ${QML_BASE_DIR}/Constants.qml
    ${QML_BASE_DIR}/Dialogs/AboutDialog.qml
    ${QML_BASE_DIR}/Dialogs/AddMidiCcAutomationDialog.qml
    ${QML_BASE_DIR}/Dialogs/AddMidiCcSettingDialog.qml
    ${QML_BASE_DIR}/Dialogs/AddPitchBendAutomationDialog.qml
    ${QML_BASE_DIR}/Dialogs/ColumnSettingsDialog.qml
    ${QML_BASE_DIR}/Dialogs/ColumnSettingsDialog_MidiEffects.qml
    ${QML_BASE_DIR}/Dialogs/DelayCalculatorDialog.qml
    ${QML_BASE_DIR}/Dialogs/EditMidiCcAutomationsDelegate.qml
    ${QML_BASE_DIR}/Dialogs/EditMidiCcAutomationsDialog.qml
    ${QML_BASE_DIR}/Dialogs/EditPitchBendAutomationsDelegate.qml
    ${QML_BASE_DIR}/Dialogs/EditPitchBendAutomationsDialog.qml
    ${QML_BASE_DIR}/Dialogs/EventSelectionDialog.qml
    ${QML_BASE_DIR}/Dialogs/GainConverterDialog.qml
    ${QML_BASE_DIR}/Dialogs/IntegerInputDialog.qml
    ${QML_BASE_DIR}/Dialogs/InterpolationDialog.qml
    ${QML_BASE_DIR}/Dialogs/ManualDialog.qml
    ${QML_BASE_DIR}/Dialogs/MidiCcAutomationModel.qml
    ${QML_BASE_DIR}/Dialogs/MidiCcSelector.qml
    ${QML_BASE_DIR}/Dialogs/MidiExportDialog.qml
    ${QML_BASE_DIR}/Dialogs/MidiImportDialog.qml
    ${QML_BASE_DIR}/Dialogs/NoteFrequencyDialog.qml
    ${QML_BASE_DIR}/Dialogs/PitchBendAutomationModel.qml
    ${QML_BASE_DIR}/Dialogs/RecentFilesDialog.qml
    ${QML_BASE_DIR}/Dialogs/SettingsDialog.qml
    ${QML_BASE_DIR}/Dialogs/SettingsDialog_AudioSettings.qml
    ${QML_BASE_DIR}/Dialogs/SettingsDialog_GeneralSettings.qml
    ${QML_BASE_DIR}/Dialogs/SettingsDialog_MidiSettings.qml
    ${QML_BASE_DIR}/Dialogs/ShortcutsDialog.qml
    ${QML_BASE_DIR}/Dialogs/SideChainTargetDelegate.qml
    ${QML_BASE_DIR}/Dialogs/TrackSettingsDialog.qml
    ${QML_BASE_DIR}/Dialogs/TrackSettingsDialog_InstrumentSettings.qml
    ${QML_BASE_DIR}/Dialogs/TrackSettingsDialog_MidiCcSettings.qml
    ${QML_BASE_DIR}/Dialogs/TrackSettingsDialog_MidiEffects.qml
    ${QML_BASE_DIR}/Dialogs/TrackSettingsDialog_TimingSettings.qml
    ${QML_BASE_DIR}/Dialogs/UnsavedChangesDialog.qml
    ${QML_BASE_DIR}/Editor/AudioWaveView.qml
    ${QML_BASE_DIR}/Editor/Cursor.qml
    ${QML_BASE_DIR}/Editor/EditorView.qml
    ${QML_BASE_DIR}/Editor/IndexHighlight.qml
    ${QML_BASE_DIR}/Editor/KeyboardHandler.qml
    ${QML_BASE_DIR}/Editor/LineNumberColumn.qml
    ${QML_BASE_DIR}/Editor/LineNumberDelegate.qml
    ${QML_BASE_DIR}/Editor/MainContextMenu.qml
    ${QML_BASE_DIR}/Editor/MainContextMenu_Column.qml
    ${QML_BASE_DIR}/Editor/MainContextMenu_Line.qml
    ${QML_BASE_DIR}/Editor/MainContextMenu_Pattern.qml
    ${QML_BASE_DIR}/Editor/MainContextMenu_Selection.qml
    ${QML_BASE_DIR}/Editor/MainContextMenu_Track.qml
    ${QML_BASE_DIR}/Editor/MouseHandler.qml
    ${QML_BASE_DIR}/Editor/MuteSoloButtons.qml
    ${QML_BASE_DIR}/Editor/NoteColumn.qml
    ${QML_BASE_DIR}/Editor/NoteColumnHeader.qml
    ${QML_BASE_DIR}/Editor/NoteColumn_LineContainer.qml
    ${QML_BASE_DIR}/Editor/NoteColumn_LineDelegate.qml
    ${QML_BASE_DIR}/Editor/Pattern.qml
    ${QML_BASE_DIR}/Editor/PositionBar.qml
    ${QML_BASE_DIR}/Editor/SongView.qml
    ${QML_BASE_DIR}/Editor/ThemedProgressBar.qml
    ${QML_BASE_DIR}/Editor/Track.qml
    ${QML_BASE_DIR}/Editor/TrackHeader.qml
    ${QML_BASE_DIR}/Editor/TrackHeader_ColumnButtons.qml
    ${QML_BASE_DIR}/Editor/VelocityScale.qml
    ${QML_BASE_DIR}/Editor/VolumeMeter.qml
    ${QML_BASE_DIR}/MainMenu/MainMenu.qml
    ${QML_BASE_DIR}/MainMenu/MainMenu_Edit.qml
    ${QML_BASE_DIR}/MainMenu/MainMenu_File.qml
    ${QML_BASE_DIR}/MainMenu/MainMenu_Help.qml
    ${QML_BASE_DIR}/MainMenu/MainMenu_Tools.qml
    ${QML_BASE_DIR}/NoteVisualizer.qml
    ${QML_BASE_DIR}/ToolBar/EditorControls.qml
    ${QML_BASE_DIR}/ToolBar/MainToolBar.qml
    ${QML_BASE_DIR}/ToolBar/PlayerButtons/LoopButton.qml
    ${QML_BASE_DIR}/ToolBar/PlayerButtons/PlayButton.qml
    ${QML_BASE_DIR}/ToolBar/PlayerButtons/PrevButton.qml
    ${QML_BASE_DIR}/ToolBar/PlayerButtons/StopButton.qml
    ${QML_BASE_DIR}/ToolBar/PlayerControls.qml
    ${QML_BASE_DIR}/ToolBar/Separator.qml
    ${QML_BASE_DIR}/ToolBar/ToolBarButtonBase.qml
    ${QML_BASE_DIR}/UiService.qml
)

set(HTML_SOURCE_FILES ${QML_BASE_DIR}/Manual.html)

set(PNG_SOURCE_FILES
    ${QML_BASE_DIR}/Graphics/add_box.png
    ${QML_BASE_DIR}/Graphics/del_box.png
    ${QML_BASE_DIR}/Graphics/delete.png
    ${QML_BASE_DIR}/Graphics/icon.png
    ${QML_BASE_DIR}/Graphics/mute.png
    ${QML_BASE_DIR}/Graphics/play.png
    ${QML_BASE_DIR}/Graphics/prev.png
    ${QML_BASE_DIR}/Graphics/replay.png
    ${QML_BASE_DIR}/Graphics/record.png
    ${QML_BASE_DIR}/Graphics/settings.png
    ${QML_BASE_DIR}/Graphics/solo.png
    ${QML_BASE_DIR}/Graphics/stop.png
)

qt_add_qml_module(${BINARY_NAME}
    VERSION 1.0
    URI ${URI}
    QML_FILES ${QML_SOURCE_FILES}
    RESOURCES ${PNG_SOURCE_FILES} ${HTML_SOURCE_FILES}
)

target_include_directories(${BINARY_NAME} PRIVATE contrib/SimpleLogger/src)
target_include_directories(${BINARY_NAME} PRIVATE contrib/Argengine/src)
target_include_directories(${BINARY_NAME} PRIVATE ${RTAUDIO_INCLUDE_DIRS})
target_include_directories(${BINARY_NAME} PRIVATE ${SNDFILE_INCLUDE_DIRS})

target_compile_definitions(${BINARY_NAME} PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
target_compile_definitions(${BINARY_NAME} PRIVATE QML_ROOT_DIR="${QML_ROOT_DIR}")
target_compile_definitions(${BINARY_NAME} PRIVATE QML_ENTRY_POINT="${MAIN}")

if(ENABLE_MIDI_DEBUG)
  target_compile_definitions(${BINARY_NAME} PRIVATE ENABLE_MIDI_DEBUG)
endif()

target_link_libraries(${BINARY_NAME} PRIVATE Qt6::Core Qt6::Quick ${RTMIDI_LIBRARIES} ${RTAUDIO_LIBRARIES} ${SNDFILE_LIBRARIES} SimpleLogger_static Argengine_static)

include(${CMAKE_SOURCE_DIR}/cmake/DebianPackagingDefaultQt.cmake)

include(CPack)
