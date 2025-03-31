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
  application/models/midi_cc_selection_model.hpp
  application/models/recent_files_model.hpp
  application/models/track_settings_model.hpp
  application/note_converter.hpp
  application/player_service.hpp
  application/player_worker.hpp
  application/position.hpp
  application/recent_files_manager.hpp
  application/selection_service.hpp
  application/state_machine.hpp
  application/ui_logger.hpp
  common/constants.hpp
  common/utils.hpp
  domain/column.hpp
  domain/event.hpp
  domain/instrument.hpp
  domain/instrument_settings.hpp
  domain/interpolator.hpp
  domain/line.hpp
  domain/line_event.hpp
  domain/midi_cc_setting.hpp
  domain/mixer_unit.hpp
  domain/note_data.hpp
  domain/note_data_manipulator.hpp
  domain/pattern.hpp
  domain/play_order.hpp
  domain/song.hpp
  domain/track.hpp
  infra/midi/midi_backend.hpp
  infra/midi/midi_backend_rt_midi.hpp
  infra/midi/midi_cc.hpp
  infra/midi/midi_device.hpp
  infra/settings.hpp
  infra/video/animation.hpp
  infra/video/default_particle_animation.hpp
  infra/video/video_config.hpp
  infra/video/video_generator.hpp
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
  application/models/midi_cc_selection_model.cpp
  application/models/recent_files_model.cpp
  application/models/track_settings_model.cpp
  application/note_converter.cpp
  application/player_service.cpp
  application/player_worker.cpp
  application/recent_files_manager.cpp
  application/selection_service.cpp
  application/state_machine.cpp
  application/ui_logger.cpp
  common/constants.cpp
  common/utils.cpp
  domain/column.cpp
  domain/event.cpp
  domain/instrument.cpp
  domain/instrument_settings.cpp
  domain/interpolator.cpp
  domain/line.cpp
  domain/line_event.cpp
  domain/midi_cc_setting.cpp
  domain/mixer_unit.cpp
  domain/note_data.cpp
  domain/note_data_manipulator.cpp
  domain/pattern.cpp
  domain/play_order.cpp
  domain/song.cpp
  domain/track.cpp
  infra/midi/midi_backend.cpp
  infra/midi/midi_cc.cpp
  infra/midi/midi_backend_rt_midi.cpp
  infra/midi/midi_device.cpp
  infra/settings.cpp
  infra/video/animation.cpp
  infra/video/default_particle_animation.cpp
  infra/video/video_generator.cpp
  main.cpp
)

# See https://doc.qt.io/qt-6/qt-cmake-policy-qtp0001.html
qt_policy(SET QTP0001 NEW)

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
    ${QML_BASE_DIR}/Dialogs/IntegerInputDialog.qml
    ${QML_BASE_DIR}/Dialogs/InterpolationDialog.qml
    ${QML_BASE_DIR}/Dialogs/MidiCcSelector.qml
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
    ${QML_BASE_DIR}/Editor/MuteSoloButtons.qml
    ${QML_BASE_DIR}/Editor/NoteColumn.qml
    ${QML_BASE_DIR}/Editor/NoteColumnHeader.qml
    ${QML_BASE_DIR}/Editor/NoteColumnLine.qml
    ${QML_BASE_DIR}/Editor/Pattern.qml
    ${QML_BASE_DIR}/Editor/PositionBar.qml
    ${QML_BASE_DIR}/Editor/Track.qml
    ${QML_BASE_DIR}/Editor/TrackHeader.qml
    ${QML_BASE_DIR}/Editor/TrackHeaderColumnButtons.qml
    ${QML_BASE_DIR}/Editor/VelocityScale.qml
    ${QML_BASE_DIR}/Editor/VolumeMeter.qml
    ${QML_BASE_DIR}/MainMenu.qml
    ${QML_BASE_DIR}/MainMenuItemDelegate.qml
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
set(SVG_SOURCE_FILES
    ${QML_BASE_DIR}/Graphics/add_box.svg
    ${QML_BASE_DIR}/Graphics/del_box.svg
    ${QML_BASE_DIR}/Graphics/icon.svg
    ${QML_BASE_DIR}/Graphics/mute.svg
    ${QML_BASE_DIR}/Graphics/play.svg
    ${QML_BASE_DIR}/Graphics/prev.svg
    ${QML_BASE_DIR}/Graphics/replay.svg
    ${QML_BASE_DIR}/Graphics/settings.svg
    ${QML_BASE_DIR}/Graphics/solo.svg
    ${QML_BASE_DIR}/Graphics/stop.svg
)
qt_add_qml_module(${BINARY_NAME}
    VERSION 1.0
    URI ${URI}
    QML_FILES ${QML_SOURCE_FILES}
    RESOURCES ${SVG_SOURCE_FILES}
)

target_compile_definitions(${BINARY_NAME} PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
target_compile_definitions(${BINARY_NAME} PRIVATE QML_ROOT_DIR="${QML_ROOT_DIR}")
target_compile_definitions(${BINARY_NAME} PRIVATE QML_ENTRY_POINT="${MAIN}")
target_link_libraries(${BINARY_NAME} PRIVATE Qt6::Core Qt6::Quick rtmidi SimpleLogger_static Argengine_static)

install(TARGETS ${BINARY_NAME} RUNTIME DESTINATION bin)
install(FILES ${CMAKE_SOURCE_DIR}/data/linux/noteahead.desktop DESTINATION share/applications)
install(FILES ${CMAKE_SOURCE_DIR}/data/linux/noteahead.appdata.xml DESTINATION share/metainfo)
install(FILES ${CMAKE_SOURCE_DIR}/data/linux/noteahead.png DESTINATION share/pixmaps)
install(FILES ${CMAKE_SOURCE_DIR}/data/linux/noteahead.png DESTINATION share/icons/hicolor/256x256/apps)

# Set up CPack for Debian packaging
set(CPACK_PACKAGE_NAME "noteahead")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Noteahead MIDI Tracker")
set(CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${VERSION_PATCH})
set(CPACK_PACKAGE_VENDOR Juzzlin)
set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CPACK_PACKAGE_NAME})
set(CPACK_VERBATIM_VARIABLES ON)

set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Jussi Lind <jussi.lind@iki.fi>")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS YES)
set(QML_RUNTIME_PACKAGES qml6-module-qtqml qml6-module-qtcore qml6-module-qtquick-dialogs qml6-module-qtquick-templates qml6-module-qtquick-window)
set(CPACK_DEBIAN_PACKAGE_DEPENDS ${QML_RUNTIME_PACKAGES})
set(CPACK_DEBIAN_PACKAGE_VERSION ${APPLICATION_VERSION})

if(DISTRO_VERSION)
    message(STATUS "Distro version: ${DISTRO_VERSION}")
    set(CPACK_PACKAGE_FILE_NAME ${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${DISTRO_VERSION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE})
else()
    set(CPACK_PACKAGE_FILE_NAME ${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE})
endif()

include(CPack)
