include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(NAME note_column_renderer_test)
set(SRC
${NAME}.cpp
    ${NAME}.hpp)
qt_add_executable(${NAME} ${SRC})
set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${UNIT_TEST_BASE_DIR})
add_test(${NAME} ${UNIT_TEST_BASE_DIR}/${NAME})
# Paints through QPainter, so it needs a QGuiApplication and a font database. Forced offscreen so it
# runs the same with or without a display, this being the only test in the suite that is not guiless.
set_tests_properties(${NAME} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
target_link_libraries(${NAME} PRIVATE ApplicationLib Argengine_static CommonLib DomainLib InfraLib SimpleLogger_static ViewLib Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Quick SimpleLogger_static)
