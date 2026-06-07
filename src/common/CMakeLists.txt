set(HEADER_FILES
    constants.hpp
    denormal_protection.hpp
    parameter_mapper.hpp
    utils.hpp
    waveform_generator.hpp
)

set(SOURCE_FILES
    constants.cpp
    parameter_mapper.cpp
    utils.cpp
    waveform_generator.cpp
)

add_library(${CommonLibName} STATIC ${HEADER_FILES} ${SOURCE_FILES})

target_include_directories(${CommonLibName} PUBLIC ..)

target_link_libraries(${CommonLibName} PUBLIC Qt6::Core Qt6::Quick)
