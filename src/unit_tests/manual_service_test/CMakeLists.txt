include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(NAME manual_service_test)
set(SRC
${NAME}.cpp
    ${NAME}.hpp)
qt_add_executable(${NAME} ${SRC})
set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${UNIT_TEST_BASE_DIR})
target_compile_definitions(${NAME} PRIVATE MANUAL_HTML_PATH="${CMAKE_SOURCE_DIR}/src/view/qml/Manual.html")
add_test(${NAME} ${UNIT_TEST_BASE_DIR}/${NAME})
target_link_libraries(${NAME} PRIVATE ApplicationLib Argengine_static CommonLib DomainLib InfraLib SimpleLogger_static ViewLib Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Gui SimpleLogger_static)
