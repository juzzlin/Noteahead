import os
import re

# Core files that almost every test needs now
core_files = [
    "../../application/position.cpp",
    "../../common/constants.cpp",
    "../../common/utils.cpp",
    "../../domain/automation_location.cpp",
    "../../domain/midi_address.cpp",
    "../../domain/midi_note_data.cpp",
]

# Standard domain files
domain_files = [
    "../../domain/arpeggiator.cpp",
    "../../domain/automation.cpp",
    "../../domain/column.cpp",
    "../../domain/column_settings.cpp",
    "../../domain/devices/device.cpp",
    "../../domain/devices/effect.cpp",
    "../../domain/devices/gain.cpp",
    "../../domain/devices/panner.cpp",
    "../../domain/devices/sampler_device.cpp",
    "../../domain/devices/svf_filter.cpp",
    "../../domain/event.cpp",
    "../../domain/event_data.cpp",
    "../../domain/instrument.cpp",
    "../../domain/instrument_settings.cpp",
    "../../domain/interpolator.cpp",
    "../../domain/line.cpp",
    "../../domain/line_event.cpp",
    "../../domain/midi_cc_automation.cpp",
    "../../domain/midi_cc_data.cpp",
    "../../domain/midi_cc_setting.cpp",
    "../../domain/mixer_unit.cpp",
    "../../domain/note_data.cpp",
    "../../domain/note_data_manipulator.cpp",
    "../../domain/pattern.cpp",
    "../../domain/pitch_bend_automation.cpp",
    "../../domain/pitch_bend_data.cpp",
    "../../domain/play_order.cpp",
    "../../domain/song.cpp",
    "../../domain/track.cpp",
]

# Common services
service_files = [
    "../../application/service/automation_service.cpp",
    "../../application/service/copy_manager.cpp",
    "../../application/service/device_service.cpp",
    "../../application/service/jack_service.cpp",
    "../../application/service/midi_service.cpp",
    "../../application/service/midi_worker.cpp",
    "../../application/service/midi_worker_in.cpp",
    "../../application/service/midi_worker_out.cpp",
    "../../application/service/mixer_service.cpp",
    "../../application/service/property_service.cpp",
    "../../application/service/random_service.cpp",
    "../../application/service/settings_service.cpp",
    "../../application/service/side_chain_service.cpp",
]

# Common infra
infra_files = [
    "../../infra/audio/async_audio_file_reader.cpp",
    "../../infra/audio/async_audio_file_writer.cpp",
    "../../infra/audio/audio_engine.cpp",
    "../../infra/audio/audio_player.cpp",
    "../../infra/audio/audio_recorder.cpp",
    "../../infra/audio/backend/sndfile_reader.cpp",
    "../../infra/audio/implementation/alsa/audio_player_alsa.cpp",
    "../../infra/audio/implementation/alsa/audio_recorder_alsa.cpp",
    "../../infra/audio/implementation/jack/audio_player_jack.cpp",
    "../../infra/audio/implementation/jack/audio_recorder_jack.cpp",
    "../../infra/midi/implementation/librtmidi/midi_in_rt_midi.cpp",
    "../../infra/midi/implementation/librtmidi/midi_out_rt_midi.cpp",
    "../../infra/midi/midi_backend.cpp",
    "../../infra/midi/midi_backend_in.cpp",
    "../../infra/midi/midi_backend_out.cpp",
    "../../infra/midi/midi_cc_mapping.cpp",
    "../../infra/midi/midi_port.cpp",
    "../../infra/settings.cpp",
]

def update_cmake(file_path):
    with open(file_path, 'r') as f:
        content = f.read()

    match = re.search(r'set\(SRC(.*?)\)', content, re.DOTALL)
    if not match:
        print(f"Could not find SRC in {file_path}")
        return

    src_content = match.group(1)
    files = re.findall(r'\S+', src_content)
    
    # Heuristic: if it uses any of our core areas, give it the full set
    is_complex_test = any(("../../domain/" in f or "../../application/service/" in f or "../../infra/" in f) for f in files)
    
    new_files = set(files)
    if is_complex_test:
        new_files.update(core_files)
        new_files.update(domain_files)
        new_files.update(service_files)
        new_files.update(infra_files)
        
    if "midi_exporter_test" in file_path or "midi_importer_test" in file_path or is_complex_test:
        new_files.add("../../infra/midi/export/midi_exporter.cpp")
        new_files.add("../../infra/midi/import/midi_importer.cpp")

    # Add instrument_request.cpp which is also common
    if is_complex_test:
        new_files.add("../../application/instrument_request.cpp")

    # Sort: variables first, then paths
    sorted_files = sorted(list(new_files), key=lambda x: (not x.startswith('$'), x.lower()))
    
    new_src_content = "\n    " + "\n    ".join(sorted_files) + "\n"
    new_content = content.replace(src_content, new_src_content)
    
    if new_content != content:
        with open(file_path, 'w') as f:
            f.write(new_content)
        print(f"Updated {file_path}")
    else:
        print(f"No changes for {file_path}")

test_dir = "/home/juzzlin/Home/Git/Noteahead/src/unit_tests"
for root, dirs, files in os.walk(test_dir):
    if "CMakeLists.txt" in files and root != test_dir:
        update_cmake(os.path.join(root, "CMakeLists.txt"))
