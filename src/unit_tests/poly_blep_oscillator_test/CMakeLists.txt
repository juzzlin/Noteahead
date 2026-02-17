include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/src)
set(NAME poly_blep_oscillator_test)
set(SRC
    ${NAME}.cpp
    ${NAME}.hpp
    ../../common/constants.cpp
    ../../domain/dsp/dsp_component.cpp
    ../../domain/dsp/poly_blep_oscillator.cpp
)
qt_add_executable(${NAME} ${SRC})
set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${UNIT_TEST_BASE_DIR})
add_test(${NAME} ${UNIT_TEST_BASE_DIR}/${NAME})
target_link_libraries(${NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Test)
