include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(NAME editor_service_test)
set(SRC
    ${NAME}.cpp
    ${NAME}.hpp
    ../../application/instrument_request.cpp
    ../../application/note_converter.cpp
    ../../application/position.hpp
    ../../application/service/automation_service.cpp
    ../../application/service/copy_manager.cpp
    ../../application/service/editor_service.cpp
    ../../application/service/mixer_service.cpp
    ../../application/service/random_service.cpp
    ../../application/service/selection_service.cpp
    ../../common/constants.cpp
    ../../common/utils.cpp
    ../../domain/automation.cpp
    ../../domain/automation_location.cpp
    ../../domain/column.cpp
    ../../domain/event.cpp
    ../../domain/event_data.cpp
    ../../domain/instrument.cpp
    ../../domain/instrument_settings.cpp
    ../../domain/interpolator.cpp
    ../../domain/line.cpp
    ../../domain/line_event.cpp
    ../../domain/midi_address.cpp
    ../../domain/midi_cc_automation.cpp
    ../../domain/midi_cc_data.cpp
    ../../domain/midi_cc_setting.cpp
    ../../domain/mixer_unit.cpp
    ../../domain/note_data.cpp
    ../../domain/note_data_manipulator.cpp
    ../../domain/pattern.cpp
    ../../domain/pitch_bend_automation.cpp
    ../../domain/pitch_bend_data.cpp
    ../../domain/play_order.cpp
    ../../domain/song.cpp
    ../../domain/track.cpp
)
qt_add_executable(${NAME} ${SRC})
set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${UNIT_TEST_BASE_DIR})
add_test(${NAME} ${UNIT_TEST_BASE_DIR}/${NAME})
target_link_libraries(${NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Test SimpleLogger)
