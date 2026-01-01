include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(NAME property_service_test)
set(SRC
    ${NAME}.cpp
    ${NAME}.hpp
    ../../application/service/property_service.cpp
    ../../infra/midi/midi_cc_mapping.cpp
)
qt_add_executable(${NAME} ${SRC})
set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${UNIT_TEST_BASE_DIR})
add_test(${NAME} ${UNIT_TEST_BASE_DIR}/${NAME})
target_link_libraries(${NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Test SimpleLogger)
