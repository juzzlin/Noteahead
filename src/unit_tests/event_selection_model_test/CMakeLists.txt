include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(NAME event_selection_model_test)
set(SRC
    ${NAME}.cpp
    ${NAME}.hpp
    ../../application/models/event_selection_model.cpp
    ../../common/constants.cpp
    ../../common/utils.cpp
    ../../domain/event_data.cpp
    ../../domain/instrument_settings.cpp
    ../../domain/midi_cc_setting.cpp
)
qt_add_executable(${NAME} ${SRC})
set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${UNIT_TEST_BASE_DIR})
add_test(${NAME} ${UNIT_TEST_BASE_DIR}/${NAME})
target_link_libraries(${NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Test SimpleLogger)
