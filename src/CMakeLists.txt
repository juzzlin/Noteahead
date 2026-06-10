add_subdirectory(contrib/SimpleLogger EXCLUDE_FROM_ALL)
add_subdirectory(contrib/Argengine EXCLUDE_FROM_ALL)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# These are needed only for the IDE
set(HEADER_FILES)

set(SOURCE_FILES
  main.cpp
)

qt_add_executable(${BINARY_NAME} ${HEADER_FILES} ${SOURCE_FILES})

set(URI ${BINARY_NAME})
set(QML_BASE_DIR view/qml)
if(QT_VERSION_MAJOR EQUAL 6 AND QT_VERSION_MINOR GREATER_EQUAL 5)
  set(QML_ROOT_DIR "qrc:/qt/qml/${URI}/${QML_BASE_DIR}")
else()
  set(QML_ROOT_DIR "qrc:/${URI}/${QML_BASE_DIR}")
endif()
set(MAIN Main.qml)
set(QML_SOURCE_FILES
    ${QML_BASE_DIR}/${MAIN}
    ${QML_BASE_DIR}/BottomBar.qml
    ${QML_BASE_DIR}/Components/FilterKnob.qml
    ${QML_BASE_DIR}/Components/Knob.qml
    ${QML_BASE_DIR}/Components/LayoutSeparator.qml
    ${QML_BASE_DIR}/Components/MenuItemDelegate.qml
    ${QML_BASE_DIR}/Components/MidiCcComboBox.qml
    ${QML_BASE_DIR}/Components/SyncSlider.qml
    ${QML_BASE_DIR}/Components/VirtualKeyboard.qml
    ${QML_BASE_DIR}/Components/WaveformView.qml
    ${QML_BASE_DIR}/Constants.qml
    ${QML_BASE_DIR}/Dialogs/AboutDialog.qml
    ${QML_BASE_DIR}/Dialogs/AddMidiCcAutomationDialog.qml
    ${QML_BASE_DIR}/Dialogs/AddMidiCcSettingDialog.qml
    ${QML_BASE_DIR}/Dialogs/AddPitchBendAutomationDialog.qml
    ${QML_BASE_DIR}/Dialogs/AudioRenderDialog.qml
    ${QML_BASE_DIR}/Dialogs/AutoPannerDialog.qml
    ${QML_BASE_DIR}/Dialogs/BassSynthDialog.qml
    ${QML_BASE_DIR}/Dialogs/BassSynthDialog_Distortion.qml
    ${QML_BASE_DIR}/Dialogs/BassSynthDialog_Filter.qml
    ${QML_BASE_DIR}/Dialogs/BassSynthDialog_Global.qml
    ${QML_BASE_DIR}/Dialogs/BassSynthDialog_Oscillator.qml
    ${QML_BASE_DIR}/Dialogs/ChorusDialog.qml
    ${QML_BASE_DIR}/Dialogs/ClipperDialog.qml
    ${QML_BASE_DIR}/Dialogs/ColumnSettingsDialog.qml
    ${QML_BASE_DIR}/Dialogs/ColumnSettingsDialog_InstrumentSettings.qml
    ${QML_BASE_DIR}/Dialogs/ColumnSettingsDialog_MidiEffects.qml
    ${QML_BASE_DIR}/Dialogs/ColumnSettingsDialog_TimingSettings.qml
    ${QML_BASE_DIR}/Dialogs/CompressorDialog.qml
    ${QML_BASE_DIR}/Dialogs/DelayDialog.qml
    ${QML_BASE_DIR}/Dialogs/DelayCalculatorDialog.qml
    ${QML_BASE_DIR}/Dialogs/DeleteUnusedPatternsDialog.qml
    ${QML_BASE_DIR}/Dialogs/DeviceGalleryDialog.qml
    ${QML_BASE_DIR}/Dialogs/DeviceInsertEffectsDialog.qml
    ${QML_BASE_DIR}/Dialogs/DeviceRackDialog.qml
    ${QML_BASE_DIR}/Dialogs/DrumSynthDialog.qml
    ${QML_BASE_DIR}/Dialogs/EditMidiCcAutomationsDelegate.qml
    ${QML_BASE_DIR}/Dialogs/EditMidiCcAutomationsDialog.qml
    ${QML_BASE_DIR}/Dialogs/EditPitchBendAutomationsDelegate.qml
    ${QML_BASE_DIR}/Dialogs/EditPitchBendAutomationsDialog.qml
    ${QML_BASE_DIR}/Dialogs/EffectSendsDialog.qml
    ${QML_BASE_DIR}/Dialogs/EffectsGalleryDialog.qml
    ${QML_BASE_DIR}/Dialogs/Eq8BandParametricDialog.qml
    ${QML_BASE_DIR}/Dialogs/EventSelectionDialog.qml
    ${QML_BASE_DIR}/Dialogs/GainConverterDialog.qml
    ${QML_BASE_DIR}/Dialogs/IntegerInputDialog.qml
    ${QML_BASE_DIR}/Dialogs/InterpolationDialog.qml
    ${QML_BASE_DIR}/Dialogs/ManualDialog.qml
    ${QML_BASE_DIR}/Dialogs/MasterEffectsDialog.qml
    ${QML_BASE_DIR}/Dialogs/MidiCcAutomationModel.qml
    ${QML_BASE_DIR}/Dialogs/MidiCcSelector.qml
    ${QML_BASE_DIR}/Dialogs/MidiExportDialog.qml
    ${QML_BASE_DIR}/Dialogs/MidiImportDialog.qml
    ${QML_BASE_DIR}/Dialogs/ModulationGroupBox.qml
    ${QML_BASE_DIR}/Dialogs/NoteFrequencyDialog.qml
    ${QML_BASE_DIR}/Dialogs/PannerDialog.qml
    ${QML_BASE_DIR}/Dialogs/PitchBendAutomationModel.qml
    ${QML_BASE_DIR}/Dialogs/RecentFilesDialog.qml
    ${QML_BASE_DIR}/Dialogs/ReverbDialog.qml
    ${QML_BASE_DIR}/Dialogs/SamplerDialog.qml
    ${QML_BASE_DIR}/Dialogs/SamplerDialog_Global.qml
    ${QML_BASE_DIR}/Dialogs/SamplerDialog_Header.qml
    ${QML_BASE_DIR}/Dialogs/SamplerDialog_Offset.qml
    ${QML_BASE_DIR}/Dialogs/SamplerDialog_Pads.qml
    ${QML_BASE_DIR}/Dialogs/SamplerDialog_WaveformView.qml
    ${QML_BASE_DIR}/Dialogs/SettingsDialog.qml
    ${QML_BASE_DIR}/Dialogs/SettingsDialog_AudioSettings.qml
    ${QML_BASE_DIR}/Dialogs/SettingsDialog_GeneralSettings.qml
    ${QML_BASE_DIR}/Dialogs/SettingsDialog_MidiSettings.qml
    ${QML_BASE_DIR}/Dialogs/SettingsDialog_ThemeSettings.qml
    ${QML_BASE_DIR}/Dialogs/ShortcutsDialog.qml
    ${QML_BASE_DIR}/Dialogs/SideChainTargetDelegate.qml
    ${QML_BASE_DIR}/Dialogs/StringInputDialog.qml
    ${QML_BASE_DIR}/Dialogs/SynthDialog.qml
    ${QML_BASE_DIR}/Dialogs/SynthDialog_AmpEg.qml
    ${QML_BASE_DIR}/Dialogs/SynthDialog_Delay.qml
    ${QML_BASE_DIR}/Dialogs/SynthDialog_Filter.qml
    ${QML_BASE_DIR}/Dialogs/SynthDialog_Global.qml
    ${QML_BASE_DIR}/Dialogs/SynthDialog_Lfo.qml
    ${QML_BASE_DIR}/Dialogs/SynthDialog_ModEg.qml
    ${QML_BASE_DIR}/Dialogs/SynthDialog_Multi.qml
    ${QML_BASE_DIR}/Dialogs/SynthDialog_Presets.qml
    ${QML_BASE_DIR}/Dialogs/SynthDialog_Tab1.qml
    ${QML_BASE_DIR}/Dialogs/SynthDialog_Tab2.qml
    ${QML_BASE_DIR}/Dialogs/SynthDialog_Tab3.qml
    ${QML_BASE_DIR}/Dialogs/SynthDialog_Vco1.qml
    ${QML_BASE_DIR}/Dialogs/SynthDialog_Vco2.qml
    ${QML_BASE_DIR}/Dialogs/SynthDialog_Vco3.qml
    ${QML_BASE_DIR}/Dialogs/TrackSettingsDialog.qml
    ${QML_BASE_DIR}/Dialogs/TrackSettingsDialog_InstrumentSettings.qml
    ${QML_BASE_DIR}/Dialogs/TrackSettingsDialog_MidiCcSettings.qml
    ${QML_BASE_DIR}/Dialogs/TrackSettingsDialog_MidiEffects.qml
    ${QML_BASE_DIR}/Dialogs/TrackSettingsDialog_TimingSettings.qml
    ${QML_BASE_DIR}/Dialogs/UnsavedChangesDialog.qml
    ${QML_BASE_DIR}/Dialogs/WavetableSynthDialog.qml
    ${QML_BASE_DIR}/Dialogs/WavetableSynthDialog_AmpEg.qml
    ${QML_BASE_DIR}/Dialogs/WavetableSynthDialog_Filter.qml
    ${QML_BASE_DIR}/Dialogs/WavetableSynthDialog_Global.qml
    ${QML_BASE_DIR}/Dialogs/WavetableSynthDialog_Lfo1.qml
    ${QML_BASE_DIR}/Dialogs/WavetableSynthDialog_Lfo2.qml
    ${QML_BASE_DIR}/Dialogs/WavetableSynthDialog_ModEg.qml
    ${QML_BASE_DIR}/Dialogs/WavetableSynthDialog_Noise.qml
    ${QML_BASE_DIR}/Dialogs/WavetableSynthDialog_Osc1.qml
    ${QML_BASE_DIR}/Dialogs/WavetableSynthDialog_Osc2.qml
    ${QML_BASE_DIR}/Dialogs/WavetableSynthDialog_Tab1.qml
    ${QML_BASE_DIR}/Dialogs/WavetableSynthDialog_Tab2.qml
    ${QML_BASE_DIR}/Dialogs/WavetableSynthDialog_Tab3.qml
    ${QML_BASE_DIR}/Editor/AudioWaveView.qml
    ${QML_BASE_DIR}/Editor/Cursor.qml
    ${QML_BASE_DIR}/Editor/EditorView.qml
    ${QML_BASE_DIR}/Editor/IndexHighlight.qml
    ${QML_BASE_DIR}/Editor/KeyboardHandler.qml
    ${QML_BASE_DIR}/Editor/LineNumberColumn.qml
    ${QML_BASE_DIR}/Editor/LineNumberDelegate.qml
    ${QML_BASE_DIR}/Editor/MainContextMenu.qml
    ${QML_BASE_DIR}/Editor/MainContextMenu_Column.qml
    ${QML_BASE_DIR}/Editor/MainContextMenu_Line.qml
    ${QML_BASE_DIR}/Editor/MainContextMenu_Pattern.qml
    ${QML_BASE_DIR}/Editor/MainContextMenu_Selection.qml
    ${QML_BASE_DIR}/Editor/MainContextMenu_Song.qml
    ${QML_BASE_DIR}/Editor/MainContextMenu_Track.qml
    ${QML_BASE_DIR}/Editor/MouseHandler.qml
    ${QML_BASE_DIR}/Editor/MuteSoloButtons.qml
    ${QML_BASE_DIR}/Editor/NoteColumn.qml
    ${QML_BASE_DIR}/Editor/NoteColumnHeader.qml
    ${QML_BASE_DIR}/Editor/NoteColumn_LineContainer.qml
    ${QML_BASE_DIR}/Editor/NoteColumn_LineDelegate.qml
    ${QML_BASE_DIR}/Editor/Pattern.qml
    ${QML_BASE_DIR}/Editor/PositionBar.qml
    ${QML_BASE_DIR}/Editor/SongView.qml
    ${QML_BASE_DIR}/Editor/ThemedProgressBar.qml
    ${QML_BASE_DIR}/Editor/Track.qml
    ${QML_BASE_DIR}/Editor/TrackHeader.qml
    ${QML_BASE_DIR}/Editor/TrackHeader_ColumnButtons.qml
    ${QML_BASE_DIR}/Editor/VelocityScale.qml
    ${QML_BASE_DIR}/Editor/VolumeMeter.qml
    ${QML_BASE_DIR}/MainMenu/MainMenu.qml
    ${QML_BASE_DIR}/MainMenu/MainMenu_Devices.qml
    ${QML_BASE_DIR}/MainMenu/MainMenu_Edit.qml
    ${QML_BASE_DIR}/MainMenu/MainMenu_Effects.qml
    ${QML_BASE_DIR}/MainMenu/MainMenu_File.qml
    ${QML_BASE_DIR}/MainMenu/MainMenu_Help.qml
    ${QML_BASE_DIR}/MainMenu/MainMenu_Tools.qml
    ${QML_BASE_DIR}/NoteVisualizer.qml
    ${QML_BASE_DIR}/ToolBar/EditorControls.qml
    ${QML_BASE_DIR}/ToolBar/MainToolBar.qml
    ${QML_BASE_DIR}/ToolBar/PlayerButtons/LoopButton.qml
    ${QML_BASE_DIR}/ToolBar/PlayerButtons/PlayButton.qml
    ${QML_BASE_DIR}/ToolBar/PlayerButtons/PrevButton.qml
    ${QML_BASE_DIR}/ToolBar/PlayerButtons/StopButton.qml
    ${QML_BASE_DIR}/ToolBar/PlayerControls.qml
    ${QML_BASE_DIR}/ToolBar/Separator.qml
    ${QML_BASE_DIR}/ToolBar/ToolBarButtonBase.qml
    ${QML_BASE_DIR}/UiService.qml
)

set(HTML_SOURCE_FILES ${QML_BASE_DIR}/Manual.html)

set(PNG_SOURCE_FILES
    ${QML_BASE_DIR}/Graphics/add_box.png
    ${QML_BASE_DIR}/Graphics/alert.png
    ${QML_BASE_DIR}/Graphics/del_box.png
    ${QML_BASE_DIR}/Graphics/delete.png
    ${QML_BASE_DIR}/Graphics/icon.png
    ${QML_BASE_DIR}/Graphics/mute.png
    ${QML_BASE_DIR}/Graphics/piano.png
    ${QML_BASE_DIR}/Graphics/play.png
    ${QML_BASE_DIR}/Graphics/prev.png
    ${QML_BASE_DIR}/Graphics/record.png
    ${QML_BASE_DIR}/Graphics/replay.png
    ${QML_BASE_DIR}/Graphics/settings.png
    ${QML_BASE_DIR}/Graphics/solo.png
    ${QML_BASE_DIR}/Graphics/stop.png
)

qt_add_qml_module(${BINARY_NAME}
    VERSION 1.0
    URI ${URI}
    QML_FILES ${QML_SOURCE_FILES}
    RESOURCES ${PNG_SOURCE_FILES} ${HTML_SOURCE_FILES}
)

set(ApplicationLibName "ApplicationLib")
set(CommonLibName "CommonLib")
set(DomainLibName "DomainLib")
set(InfraLibName "InfraLib")
set(ViewLibName "ViewLib")

add_subdirectory(application)
add_subdirectory(common)
add_subdirectory(domain)
add_subdirectory(infra)
add_subdirectory(view)

target_include_directories(${BINARY_NAME} PRIVATE contrib/SimpleLogger/src)
target_include_directories(${BINARY_NAME} PRIVATE contrib/Argengine/src)

target_compile_definitions(${BINARY_NAME} PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)

if(ENABLE_MIDI_DEBUG)
  target_compile_definitions(${BINARY_NAME} PRIVATE ENABLE_MIDI_DEBUG)
endif()

target_link_libraries(${BINARY_NAME} PRIVATE Qt6::Core Qt6::Quick SimpleLogger_static Argengine_static ${ApplicationLibName} ${CommonLibName} ${DomainLibName} ${ViewLibName} ${InfraLibName})


include(${CMAKE_SOURCE_DIR}/cmake/DebianPackagingDefaultQt.cmake)

include(CPack)

include(Style.cmake)
