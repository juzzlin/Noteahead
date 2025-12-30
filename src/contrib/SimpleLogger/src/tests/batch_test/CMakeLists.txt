set(SIMPLE_LOGGER_DIR ${CMAKE_SOURCE_DIR}/src)
include_directories(${SIMPLE_LOGGER_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

set(NAME batch_test)
set(SRC ${NAME}.cpp)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/tests)
add_executable(${NAME} ${SRC})
add_test(${NAME} ${CMAKE_BINARY_DIR}/tests/${NAME})
target_link_libraries(${NAME} ${LIBRARY_NAME})

set(COLLAPSE_NAME collapse_test)
add_executable(${COLLAPSE_NAME} collapse_test.cpp)
add_test(${COLLAPSE_NAME} ${CMAKE_BINARY_DIR}/tests/${COLLAPSE_NAME})
target_link_libraries(${COLLAPSE_NAME} ${LIBRARY_NAME})

set(COLLAPSE_TS_NAME collapse_timestamp_test)
add_executable(${COLLAPSE_TS_NAME} collapse_timestamp_test.cpp)
add_test(${COLLAPSE_TS_NAME} ${CMAKE_BINARY_DIR}/tests/${COLLAPSE_TS_NAME})
target_link_libraries(${COLLAPSE_TS_NAME} ${LIBRARY_NAME})