include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(NAME player_worker_test)
set(SRC
    ${NAME}.cpp
    ${NAME}.hpp
    ../../application/service/player_worker.cpp
    ../../application/service/midi_service.cpp
    ../../application/service/mixer_service.cpp
    ../../application/service/midi_worker_in.cpp
    ../../application/service/midi_worker_out.cpp
    ../../application/service/midi_worker.cpp
    ../../application/instrument_request.cpp
    ../../common/constants.cpp
    ../../common/utils.cpp
    ../../domain/event.cpp
    ../../domain/event_data.cpp
    ../../domain/instrument.cpp
    ../../domain/instrument_settings.cpp
    ../../domain/midi_address.cpp
    ../../domain/midi_cc_data.cpp
    ../../domain/midi_cc_setting.cpp
    ../../domain/midi_note_data.cpp
    ../../domain/note_data.cpp
    ../../domain/pitch_bend_data.cpp
    ../../infra/midi/midi_backend.cpp
    ../../infra/midi/midi_backend_in.cpp
    ../../infra/midi/midi_backend_out.cpp
    ../../infra/midi/midi_cc_mapping.cpp
    ../../infra/midi/implementation/librtmidi/midi_in_rt_midi.cpp
    ../../infra/midi/implementation/librtmidi/midi_out_rt_midi.cpp
    ../../infra/midi/midi_port.cpp
    ../../infra/settings.cpp
    ../../application/service/random_service.cpp
)

qt_add_executable(${NAME} ${SRC})
set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${UNIT_TEST_BASE_DIR})
add_test(${NAME} ${UNIT_TEST_BASE_DIR}/${NAME})
target_link_libraries(${NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Test SimpleLogger ${RTMIDI_LIBRARIES} ${RTAUDIO_LIBRARIES})
