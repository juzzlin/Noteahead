include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(NAME editor_service_test)
set(SRC
${NAME}.cpp
    ${NAME}.hpp)
qt_add_executable(${NAME} ${SRC})
set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${UNIT_TEST_BASE_DIR})
add_test(${NAME} ${UNIT_TEST_BASE_DIR}/${NAME})
target_link_libraries(${NAME} PRIVATE ApplicationLib Argengine_static CommonLib DomainLib InfraLib SimpleLogger_static ViewLib Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Gui SimpleLogger_static PkgConfig::SNDFILE PkgConfig::JACK PkgConfig::RTMIDI PkgConfig::RTAUDIO)

if(JACK_FOUND)
  target_compile_definitions(${NAME} PRIVATE HAVE_JACK)
endif()

set(DELAY_TEST_NAME editor_service_delay_test)
set(DELAY_TEST_SRC
${DELAY_TEST_NAME}.cpp
    ${DELAY_TEST_NAME}.hpp)
qt_add_executable(${DELAY_TEST_NAME} ${DELAY_TEST_SRC})
set_target_properties(${DELAY_TEST_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${UNIT_TEST_BASE_DIR})
add_test(${DELAY_TEST_NAME} ${UNIT_TEST_BASE_DIR}/${DELAY_TEST_NAME})
target_link_libraries(${DELAY_TEST_NAME} PRIVATE ApplicationLib Argengine_static CommonLib DomainLib InfraLib SimpleLogger_static ViewLib Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Gui SimpleLogger_static PkgConfig::SNDFILE PkgConfig::JACK PkgConfig::RTMIDI PkgConfig::RTAUDIO)

if(JACK_FOUND)
  target_compile_definitions(${DELAY_TEST_NAME} PRIVATE HAVE_JACK)
endif()
