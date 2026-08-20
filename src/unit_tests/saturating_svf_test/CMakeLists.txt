include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/src/contrib/SimpleLogger/src)
set(NAME saturating_svf_test)
set(SRC
${NAME}.cpp
    ${NAME}.hpp)
qt_add_executable(${NAME} ${SRC})
set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${UNIT_TEST_BASE_DIR})
add_test(${NAME} ${UNIT_TEST_BASE_DIR}/${NAME})
target_link_libraries(${NAME} PRIVATE CommonLib DomainLib SimpleLogger_static Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Gui)
