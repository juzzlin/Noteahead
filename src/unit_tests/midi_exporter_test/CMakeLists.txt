cmake_minimum_required(VERSION 3.16)

set(TEST_NAME midi_exporter_test)

set(SRC
    ${TEST_NAME}.cpp
    ../../infra/midi/export/midi_exporter.cpp
    ../../application/service/automation_service.cpp
    ../../application/service/mixer_service.cpp
    ../../domain/song.cpp
    ../../domain/event.cpp
    ../../domain/event_data.cpp
    ../../domain/instrument.cpp
    ../../domain/midi_cc_data.cpp
    ../../domain/note_data.cpp
    ../../domain/pitch_bend_data.cpp
    ../../domain/pattern.cpp
    ../../domain/play_order.cpp
    ../../domain/track.cpp
    ../../domain/column.cpp
    ../../domain/column_settings.cpp
    ../../domain/line.cpp
    ../../domain/line_event.cpp
    ../../domain/midi_address.cpp
    ../../domain/midi_cc_automation.cpp
    ../../domain/midi_cc_setting.cpp
    ../../domain/mixer_unit.cpp
    ../../domain/note_data_manipulator.cpp
    ../../domain/instrument_settings.cpp
    ../../domain/interpolator.cpp
    ../../common/constants.cpp
    ../../common/utils.cpp
    ../../domain/automation.cpp
    ../../domain/automation_location.cpp
    ../../domain/pitch_bend_automation.cpp
    ../../application/service/copy_manager.cpp
    ../../application/service/random_service.cpp
)

add_executable(${TEST_NAME} ${SRC})

target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../..)
target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../contrib/SimpleLogger/src)

target_link_libraries(${TEST_NAME}
    PRIVATE
        Qt6::Test
        SimpleLogger
)

add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
