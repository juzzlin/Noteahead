set(HEADER_FILES
    knob_controller_test.hpp
)

set(SOURCE_FILES
    knob_controller_test.cpp
    ../../application/service/knob_controller.cpp
    ../../common/constants.cpp
)

qt_add_executable(knob_controller_test ${HEADER_FILES} ${SOURCE_FILES})

target_link_libraries(knob_controller_test PRIVATE Qt6::Test Qt6::Core)
target_include_directories(knob_controller_test PRIVATE ../../contrib/SimpleLogger/src)
target_include_directories(knob_controller_test PRIVATE ../../contrib/Argengine/src)

add_test(NAME knob_controller_test COMMAND knob_controller_test)
