set(HEADER_FILES
    parameter_test.hpp
)

set(SOURCE_FILES
parameter_test.cpp)

qt_add_executable(parameter_test ${HEADER_FILES} ${SOURCE_FILES})

target_link_libraries(parameter_test PRIVATE CommonLib DomainLib SimpleLogger_static Qt6::Test Qt6::Core)

add_test(NAME parameter_test COMMAND parameter_test)
