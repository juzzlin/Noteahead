include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(NAME instrument_layers_model_test)
set(SRC
    ${NAME}.cpp
    ${NAME}.hpp
    ../../application/models/instrument_layers_model.cpp
    ../../common/constants.cpp
    ../../common/utils.cpp
    ../../domain/automation.cpp
    ../../domain/automation_location.cpp
    ../../domain/instrument_layer.cpp
)
qt_add_executable(${NAME} ${SRC})
set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${UNIT_TEST_BASE_DIR})
add_test(${NAME} ${UNIT_TEST_BASE_DIR}/${NAME})
target_link_libraries(${NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Test SimpleLogger)
