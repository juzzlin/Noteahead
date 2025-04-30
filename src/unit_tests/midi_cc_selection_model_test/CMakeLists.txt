include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(NAME midi_cc_selection_model_test)
set(SRC
    ${NAME}.cpp
    ${NAME}.hpp
    ../../application/models/midi_cc_selection_model.cpp
    ../../common/constants.cpp
    ../../common/utils.cpp
    ../../domain/midi_cc_setting.cpp
    ../../infra/midi/midi_cc_mapping.cpp
)
add_executable(${NAME} ${SRC} ${MOC_SRC})
set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${UNIT_TEST_BASE_DIR})
add_test(${NAME} ${UNIT_TEST_BASE_DIR}/${NAME})
target_link_libraries(${NAME} Qt${QT_VERSION_MAJOR}::Test SimpleLogger)
