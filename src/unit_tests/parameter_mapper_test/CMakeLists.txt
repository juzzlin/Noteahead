set(HEADER_FILES
    parameter_mapper_test.hpp
)

set(SOURCE_FILES
    parameter_mapper_test.cpp
    ../../common/parameter_mapper.cpp
)

qt_add_executable(parameter_mapper_test ${HEADER_FILES} ${SOURCE_FILES})

target_link_libraries(parameter_mapper_test PRIVATE Qt6::Test Qt6::Core)

add_test(NAME parameter_mapper_test COMMAND parameter_mapper_test)
