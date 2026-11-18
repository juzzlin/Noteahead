set(HEADER_FILES
    constants.hpp
    denormal_protection.hpp
    parameter_mapper.hpp
    semantic_version.hpp
    theme_preset.hpp
    utils.hpp
    waveform_generator.hpp
    xml/project_reader.hpp
    xml/project_writer.hpp
)

set(SOURCE_FILES
    constants.cpp
    parameter_mapper.cpp
    semantic_version.cpp
    utils.cpp
    waveform_generator.cpp
)

add_library(${CommonLibName} STATIC ${HEADER_FILES} ${SOURCE_FILES})

target_include_directories(${CommonLibName} PUBLIC ..)

target_link_libraries(${CommonLibName} PUBLIC Qt6::Core Qt6::Quick)
