set(HEADER_FILES
    audio/audio_engine.hpp
    audio/audio_file_recorder.hpp
    audio/audio_file_streamer.hpp
    audio/audio_player.hpp
    audio/audio_recorder.hpp
    audio/backend/audio_file_reader.hpp
    audio/backend/sndfile_reader.hpp
    audio/implementation/jack/audio_player_jack.hpp
    audio/implementation/jack/audio_recorder_jack.hpp
    audio/implementation/librtaudio/audio_player_rt_audio.hpp
    audio/implementation/librtaudio/audio_recorder_rt_audio.hpp
    audio/real_time_worker_pool.hpp
    audio/ring_buffer.hpp
    data_service.hpp
    midi/export/midi_exporter.hpp
    midi/implementation/librtmidi/midi_in_rt_midi.hpp
    midi/implementation/librtmidi/midi_out_rt_midi.hpp
    midi/import/midi_importer.hpp
    midi/midi_backend.hpp
    midi/midi_backend_in.hpp
    midi/midi_backend_out.hpp
    midi/midi_cc_mapping.hpp
    midi/midi_port.hpp
    settings.hpp
    xml/nahd_xml_reader.hpp
    xml/nahd_xml_writer.hpp
)

set(SOURCE_FILES
    audio/audio_engine.cpp
    audio/audio_file_recorder.cpp
    audio/audio_file_streamer.cpp
    audio/audio_player.cpp
    audio/audio_recorder.cpp
    audio/backend/sndfile_reader.cpp
    audio/implementation/jack/audio_player_jack.cpp
    audio/implementation/jack/audio_recorder_jack.cpp
    audio/implementation/librtaudio/audio_player_rt_audio.cpp
    audio/implementation/librtaudio/audio_recorder_rt_audio.cpp
    audio/real_time_worker_pool.cpp
    data_service.cpp
    midi/export/midi_exporter.cpp
    midi/implementation/librtmidi/midi_in_rt_midi.cpp
    midi/implementation/librtmidi/midi_out_rt_midi.cpp
    midi/import/midi_importer.cpp
    midi/midi_backend.cpp
    midi/midi_backend_in.cpp
    midi/midi_backend_out.cpp
    midi/midi_cc_mapping.cpp
    midi/midi_port.cpp
    settings.cpp
    xml/nahd_xml_reader.cpp
    xml/nahd_xml_writer.cpp
)

add_library(${InfraLibName} OBJECT ${HEADER_FILES} ${SOURCE_FILES})

target_include_directories(${InfraLibName} PUBLIC ..)

target_link_libraries(${InfraLibName} PUBLIC ${CommonLibName} ${DomainLibName} Qt6::Core Qt6::Quick PkgConfig::RTMIDI PkgConfig::RTAUDIO PkgConfig::SNDFILE)

if(JACK_FOUND)
    target_compile_definitions(${InfraLibName} PUBLIC HAVE_JACK)
    target_link_libraries(${InfraLibName} PRIVATE PkgConfig::JACK)
endif()

if(ENABLE_MIDI_DEBUG)
  target_compile_definitions(${InfraLibName} PRIVATE ENABLE_MIDI_DEBUG)
endif()
