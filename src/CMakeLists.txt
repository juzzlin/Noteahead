set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

# These are needed only for the IDE
set(HEADER_FILES
  application/application.hpp
  application/application_service.hpp
  application/config.hpp
  application/editor_service.hpp
  application/player.hpp
  application/player_worker.hpp
  domain/column.hpp
  domain/event.hpp
  domain/instrument.hpp
  domain/line.hpp
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
  application/player.cpp
  application/player_worker.cpp
  domain/column.cpp
  domain/event.cpp
  domain/instrument.cpp
  domain/line.cpp
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

set(QML_BASE_DIR view/qml)
set(QML_ENTRY_POINT ${QML_BASE_DIR}/Main.qml)
set(QML_SOURCE_FILES
    ${QML_ENTRY_POINT}
    ${QML_BASE_DIR}/EditorView.qml
    ${QML_BASE_DIR}/Track.qml
)

set(URI ${BINARY_NAME})
qt_add_qml_module(${BINARY_NAME}
    URI ${URI}
    QML_FILES ${QML_SOURCE_FILES}

)

set_target_properties(${BINARY_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR})
target_compile_definitions(${BINARY_NAME} PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
target_compile_definitions(${BINARY_NAME} PRIVATE QML_ENTRY_POINT="qrc:/qt/qml/${URI}/${QML_ENTRY_POINT}")
target_link_libraries(${BINARY_NAME} PRIVATE Qt6::Core Qt6::Quick rtmidi)
