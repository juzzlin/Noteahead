include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(NAME midi_settings_model_test)
set(SRC
    ${NAME}.cpp
    ${NAME}.hpp
    ../../application/models/midi_settings_model.cpp
    ../../application/service/settings_service.cpp
    ../../infra/settings.cpp
)
qt_add_executable(${NAME} ${SRC})
set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${UNIT_TEST_BASE_DIR})
add_test(${NAME} ${UNIT_TEST_BASE_DIR}/${NAME})
target_link_libraries(${NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Gui SimpleLogger)
