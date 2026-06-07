include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(NAME automation_service_test)
set(SRC
    ${NAME}.cpp
    ${NAME}.hpp
    ../../application/position.hpp
    ../../application/service/automation_service.cpp
    ../../application/service/property_service.cpp
    ../../application/service/random_service.cpp
    ../../common/constants.cpp
    ../../common/parameter_mapper.cpp
    ../../common/utils.cpp
    ../../domain/tracker/automation.cpp
    ../../domain/tracker/automation_location.cpp
    ../../domain/devices/drum_synth_constants.cpp
    ../../domain/tracker/event.cpp
    ../../domain/tracker/event_data.cpp
    ../../domain/tracker/interpolator.cpp
    ../../domain/midi/midi_cc_automation.cpp
    ../../domain/midi/midi_cc_data.cpp
    ../../domain/tracker/note_data.cpp
    ../../domain/midi/pitch_bend_automation.cpp
    ../../domain/midi/pitch_bend_data.cpp
    ../../infra/midi/midi_cc_mapping.cpp
)
qt_add_executable(${NAME} ${SRC})
set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${UNIT_TEST_BASE_DIR})
add_test(${NAME} ${UNIT_TEST_BASE_DIR}/${NAME})
target_link_libraries(${NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Test SimpleLogger)
