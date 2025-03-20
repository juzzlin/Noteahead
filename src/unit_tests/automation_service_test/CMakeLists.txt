include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(NAME automation_service_test)
set(SRC
    ${NAME}.cpp
    ${NAME}.hpp
    ../../application/service/automation_service.cpp
    ../../common/constants.cpp
    ../../common/utils.cpp
    ../../domain/event.cpp
    ../../domain/event_data.cpp
    ../../domain/interpolator.cpp
    ../../domain/midi_cc_automation.cpp
    ../../domain/midi_cc_data.cpp
    ../../domain/note_data.cpp
)
add_executable(${NAME} ${SRC} ${MOC_SRC})
set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${UNIT_TEST_BASE_DIR})
add_test(${NAME} ${UNIT_TEST_BASE_DIR}/${NAME})
target_link_libraries(${NAME} Qt${QT_VERSION_MAJOR}::Test SimpleLogger)
