include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/src/contrib/SimpleLogger/src)

set(TEST_NAME sub_mixer_test)

set(SOURCE_FILES
${TEST_NAME}.cpp
    ${TEST_NAME}.hpp)

qt_add_executable(${TEST_NAME} ${SOURCE_FILES})
set_target_properties(${TEST_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${UNIT_TEST_BASE_DIR})
add_test(${TEST_NAME} ${UNIT_TEST_BASE_DIR}/${TEST_NAME})

target_link_libraries(${TEST_NAME} PRIVATE ApplicationLib Argengine_static CommonLib DomainLib InfraLib SimpleLogger_static ViewLib Qt${QT_VERSION_MAJOR}::Test
    Qt${QT_VERSION_MAJOR}::Xml
    SimpleLogger_static)
