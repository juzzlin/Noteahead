add_subdirectory(contrib/SimpleLogger EXCLUDE_FROM_ALL)
include_directories(contrib/SimpleLogger/src)

add_subdirectory(contrib/Argengine EXCLUDE_FROM_ALL)
include_directories(contrib/Argengine/src)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# These are needed only for the IDE
set(HEADER_FILES
  application/application.hpp
  application/application_service.hpp
  application/config.hpp
  application/copy_manager.hpp
  application/editor_service.hpp
  application/instrument_request.hpp
  application/midi_service.hpp
  application/midi_worker.hpp
  application/mixer_service.hpp
  application/models/event_selection_model.hpp
  application/models/recent_files_model.hpp
  application/models/track_settings_model.hpp
  application/note_converter.hpp
  application/player_service.hpp
  application/player_worker.hpp
  application/position.hpp
  application/recent_files_manager.hpp
  application/state_machine.hpp
  application/ui_logger.hpp
  common/constants.hpp
  common/utils.hpp
  domain/column.hpp
  domain/event.hpp
  domain/instrument.hpp
  domain/instrument_settings.hpp
  domain/line.hpp
  domain/line_event.hpp
  domain/note_data.hpp
  domain/pattern.hpp
  domain/play_order.hpp
  domain/song.hpp
  domain/track.hpp
  infra/midi_backend.hpp
  infra/midi_backend_rt_midi.hpp
  infra/midi_cc.hpp
  infra/midi_device.hpp
  infra/settings.hpp
)

set(SOURCE_FILES
  application/application.cpp
  application/application_service.cpp
  application/config.cpp
  application/copy_manager.cpp
  application/editor_service.cpp
  application/instrument_request.cpp
  application/midi_service.cpp
  application/midi_worker.cpp
  application/mixer_service.cpp
  application/models/event_selection_model.cpp
  application/models/recent_files_model.cpp
  application/models/track_settings_model.cpp
  application/note_converter.cpp
  application/player_service.cpp
  application/player_worker.cpp
  application/recent_files_manager.cpp
  application/state_machine.cpp
  application/ui_logger.cpp
  common/constants.cpp
  common/utils.cpp
  domain/column.cpp
  domain/event.cpp
  domain/instrument.cpp
  domain/instrument_settings.cpp
  domain/line.cpp
  domain/line_event.cpp
  domain/note_data.cpp
  domain/pattern.cpp
  domain/play_order.cpp
  domain/song.cpp
  domain/track.cpp
  infra/midi_backend.cpp
  infra/midi_backend_rt_midi.cpp
  infra/midi_device.cpp
  infra/settings.cpp
  main.cpp
)

qt_add_executable(${BINARY_NAME} ${HEADER_FILES} ${SOURCE_FILES})

set(URI ${BINARY_NAME})
set(QML_BASE_DIR view/qml)
set(QML_ROOT_DIR "qrc:/qt/qml/${URI}/${QML_BASE_DIR}")
set(MAIN Main.qml)
set(QML_SOURCE_FILES
    ${QML_BASE_DIR}/${MAIN}
    ${QML_BASE_DIR}/BottomBar.qml
    ${QML_BASE_DIR}/Constants.qml
    ${QML_BASE_DIR}/Dialogs/AboutDialog.qml
    ${QML_BASE_DIR}/Dialogs/EventSelectionDialog.qml
    ${QML_BASE_DIR}/Dialogs/RecentFilesDialog.qml
    ${QML_BASE_DIR}/Dialogs/SettingsDialog.qml
    ${QML_BASE_DIR}/Dialogs/TrackSettingsDialog.qml
    ${QML_BASE_DIR}/Dialogs/UnsavedChangesDialog.qml
    ${QML_BASE_DIR}/Editor/Cursor.qml
    ${QML_BASE_DIR}/Editor/EditorView.qml
    ${QML_BASE_DIR}/Editor/IndexHighlight.qml
    ${QML_BASE_DIR}/Editor/KeyboardHandler.qml
    ${QML_BASE_DIR}/Editor/LineNumberColumn.qml
    ${QML_BASE_DIR}/Editor/MainContextMenu.qml
    ${QML_BASE_DIR}/Editor/NoteColumn.qml
    ${QML_BASE_DIR}/Editor/NoteColumnLine.qml
    ${QML_BASE_DIR}/Editor/Pattern.qml
    ${QML_BASE_DIR}/Editor/PositionBar.qml
    ${QML_BASE_DIR}/Editor/Track.qml
    ${QML_BASE_DIR}/Editor/TrackHeader.qml
    ${QML_BASE_DIR}/Editor/TrackHeaderColumnButtons.qml
    ${QML_BASE_DIR}/Editor/TrackHeaderMuteSoloButtons.qml
    ${QML_BASE_DIR}/Editor/VolumeMeter.qml
    ${QML_BASE_DIR}/MainMenu.qml
    ${QML_BASE_DIR}/MainMenuItemDelegate.qml
    ${QML_BASE_DIR}/ToolBar/EditorControls.qml
    ${QML_BASE_DIR}/ToolBar/MainToolBar.qml
    ${QML_BASE_DIR}/ToolBar/PlayerButtons/PlayButton.qml
    ${QML_BASE_DIR}/ToolBar/PlayerButtons/PrevButton.qml
    ${QML_BASE_DIR}/ToolBar/PlayerButtons/StopButton.qml
    ${QML_BASE_DIR}/ToolBar/PlayerControls.qml
    ${QML_BASE_DIR}/ToolBar/Separator.qml
    ${QML_BASE_DIR}/ToolBar/ToolBarButtonBase.qml
    ${QML_BASE_DIR}/UiService.qml
)
set(SVG_SOURCE_FILES
    ${QML_BASE_DIR}/Graphics/add_box.svg
    ${QML_BASE_DIR}/Graphics/del_box.svg
    ${QML_BASE_DIR}/Graphics/mute.svg
    ${QML_BASE_DIR}/Graphics/play.svg
    ${QML_BASE_DIR}/Graphics/prev.svg
    ${QML_BASE_DIR}/Graphics/settings.svg
    ${QML_BASE_DIR}/Graphics/solo.svg
    ${QML_BASE_DIR}/Graphics/stop.svg
)
qt_add_qml_module(${BINARY_NAME}
    URI ${URI}
    QML_FILES ${QML_SOURCE_FILES}
    RESOURCES ${SVG_SOURCE_FILES}
)

target_compile_definitions(${BINARY_NAME} PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
target_compile_definitions(${BINARY_NAME} PRIVATE QML_ROOT_DIR="${QML_ROOT_DIR}")
target_compile_definitions(${BINARY_NAME} PRIVATE QML_ENTRY_POINT="${MAIN}")
target_link_libraries(${BINARY_NAME} PRIVATE Qt6::Core Qt6::Quick rtmidi SimpleLogger Argengine)
