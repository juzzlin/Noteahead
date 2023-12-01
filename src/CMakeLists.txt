set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

qt_add_executable(${BINARY_NAME}
  application.cpp
  config.cpp
  constants.hpp
  main.cpp
  settings.cpp
)

set(URI ${BINARY_NAME})
set(MAIN Ui/Main.qml)
qt_add_qml_module(${BINARY_NAME}
    URI ${URI}
    QML_FILES
        ${MAIN}
)

set_target_properties(${BINARY_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR})
target_compile_definitions(${BINARY_NAME} PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
target_compile_definitions(${BINARY_NAME} PRIVATE QML_ENTRY_POINT="qrc:/qt/qml/${URI}/${MAIN}")
target_link_libraries(${BINARY_NAME} PRIVATE Qt6::Core Qt6::Quick)
