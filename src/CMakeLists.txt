add_subdirectory(contrib/SimpleLogger EXCLUDE_FROM_ALL)
include_directories(contrib/SimpleLogger/src)

add_subdirectory(contrib/Argengine EXCLUDE_FROM_ALL)
include_directories(contrib/Argengine/src)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

# These are needed only for the IDE
set(HEADER_FILES
  application/application.hpp
  application/application_service.hpp
  application/config.hpp
  application/editor_service.hpp
  application/note_converter.hpp
  application/player_service.hpp
  application/player_worker.hpp
  application/position.hpp
  application/state_machine.hpp
  application/ui_logger.hpp
  common/constants.hpp
  domain/column.hpp
  domain/event.hpp
  domain/instrument.hpp
  domain/line.hpp
  domain/note_data.hpp
  domain/pattern.hpp
  domain/song.hpp
  domain/track.hpp
  infra/midi_device.hpp
  infra/midi_service.hpp
  infra/midi_service_rt_midi.hpp
  infra/settings.hpp
)

set(SOURCE_FILES
  application/application.cpp
  application/application_service.cpp
  application/config.cpp
  application/editor_service.cpp
  application/note_converter.cpp
  application/player_service.cpp
  application/player_worker.cpp
  application/state_machine.cpp
  application/ui_logger.cpp
  common/constants.cpp
  domain/column.cpp
  domain/event.cpp
  domain/instrument.cpp
  domain/line.cpp
  domain/note_data.cpp
  domain/pattern.cpp
  domain/song.cpp
  domain/track.cpp
  infra/midi_device.cpp
  infra/midi_service.cpp
  infra/midi_service_rt_midi.cpp
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
    ${QML_BASE_DIR}/Common/TrackSettings.qml
    ${QML_BASE_DIR}/Constants.qml
    ${QML_BASE_DIR}/Dialogs/AboutDialog.qml
    ${QML_BASE_DIR}/Dialogs/TrackSettingsDialog.qml
    ${QML_BASE_DIR}/Dialogs/UnsavedChangesDialog.qml
    ${QML_BASE_DIR}/Editor/Cursor.qml
    ${QML_BASE_DIR}/Editor/EditorView.qml
    ${QML_BASE_DIR}/Editor/IndexHighlight.qml
    ${QML_BASE_DIR}/Editor/KeyboardHandler.qml
    ${QML_BASE_DIR}/Editor/LineNumberColumn.qml
    ${QML_BASE_DIR}/Editor/NoteColumn.qml
    ${QML_BASE_DIR}/Editor/NoteColumnLine.qml
    ${QML_BASE_DIR}/Editor/Pattern.qml
    ${QML_BASE_DIR}/Editor/PositionBar.qml
    ${QML_BASE_DIR}/Editor/Track.qml
    ${QML_BASE_DIR}/Editor/TrackHeader.qml
    ${QML_BASE_DIR}/Editor/VelocityCell.qml
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
    ${QML_BASE_DIR}/Graphics/play.svg
    ${QML_BASE_DIR}/Graphics/prev.svg
    ${QML_BASE_DIR}/Graphics/settings.svg
    ${QML_BASE_DIR}/Graphics/stop.svg
)
qt_add_qml_module(${BINARY_NAME}
    URI ${URI}
    QML_FILES ${QML_SOURCE_FILES}
    RESOURCES ${SVG_SOURCE_FILES}
)

set_target_properties(${BINARY_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR})
target_compile_definitions(${BINARY_NAME} PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
target_compile_definitions(${BINARY_NAME} PRIVATE QML_ROOT_DIR="${QML_ROOT_DIR}")
target_compile_definitions(${BINARY_NAME} PRIVATE QML_ENTRY_POINT="${MAIN}")
target_link_libraries(${BINARY_NAME} PRIVATE Qt6::Core Qt6::Quick rtmidi SimpleLogger Argengine)
