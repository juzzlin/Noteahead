set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

# These are needed only for the IDE
set(HEADER_FILES
  application/application.hpp
  application/config.hpp
  infra/midi_device.hpp
  infra/midi_service.hpp
  infra/midi_service_rt_midi.hpp
  infra/settings.hpp
)

set(SOURCE_FILES
  application/application.cpp
  application/config.cpp
  infra/midi_device.cpp
  infra/midi_service.cpp
  infra/midi_service_rt_midi.cpp
  infra/settings.cpp
  main.cpp
)

qt_add_executable(${BINARY_NAME} ${HEADER_FILES} ${SOURCE_FILES})

set(URI ${BINARY_NAME})
set(QML_ENTRY_POINT view/qml/Main.qml)
qt_add_qml_module(${BINARY_NAME}
    URI ${URI}
    QML_FILES ${QML_ENTRY_POINT}
)

set_target_properties(${BINARY_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR})
target_compile_definitions(${BINARY_NAME} PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
target_compile_definitions(${BINARY_NAME} PRIVATE QML_ENTRY_POINT="qrc:/qt/qml/${URI}/${QML_ENTRY_POINT}")
target_link_libraries(${BINARY_NAME} PRIVATE Qt6::Core Qt6::Quick rtmidi)
