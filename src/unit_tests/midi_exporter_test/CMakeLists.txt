cmake_minimum_required(VERSION 3.16)

set(NAME midi_exporter_test)

set(SRC
${NAME}.cpp)

qt_add_executable(${NAME} ${SRC})
set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${UNIT_TEST_BASE_DIR})
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../..)
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../contrib/SimpleLogger/src)
target_link_libraries(${NAME} PRIVATE ApplicationLib Argengine_static CommonLib DomainLib InfraLib SimpleLogger_static ViewLib Qt6::Test SimpleLogger_static)
add_test(NAME ${NAME} COMMAND ${NAME})
