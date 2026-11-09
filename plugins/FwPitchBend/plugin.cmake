# Flatwater Pitch Bend. Included from the root CMake file after shared/shared.cmake; its build output lands in
# build/<Target>_artefacts.
set(PB_DIR ${CMAKE_CURRENT_LIST_DIR})

set(PB_SOURCES
    ${PB_DIR}/Source/PluginProcessor.cpp
    ${PB_DIR}/Source/PluginEditor.cpp
    ${PB_DIR}/Source/ui/Widgets.cpp
    ${PB_DIR}/Source/dsp/Shifter.cpp)
# the snapshot / test / render executables compile the plug-in sources outside the plug-in wrapper
set(PB_APP_DEFS JucePlugin_Name="Flatwater Pitch Bend" JucePlugin_IsSynth=0 JucePlugin_WantsMidiInput=0
    JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0 JucePlugin_VersionString="${PROJECT_VERSION}")

# The manufacturer and plug-in codes, the bundle and CLAP ids make the plug-in's identity in hosts: keep them.
fw_add_plugin(FwPitchBend "Flatwater Pitch Bend" Fpb1 "com.flatwater.pitchbend"
    CATEGORIES Fx Pitch Shift
    SOURCES ${PB_SOURCES})
target_include_directories(FwPitchBend PRIVATE ${PB_DIR}/Source)

# Editor snapshot tool (renders the GUI to a PNG for the visual comparison; docs/screenshots/README.md).
juce_add_console_app(FwPitchBendSnapshot PRODUCT_NAME "FwPitchBendSnapshot")
target_sources(FwPitchBendSnapshot PRIVATE ${CMAKE_SOURCE_DIR}/tools/Snapshot.cpp ${PB_SOURCES})
target_include_directories(FwPitchBendSnapshot PRIVATE ${PB_DIR}/Source)
target_compile_definitions(FwPitchBendSnapshot PRIVATE ${PB_APP_DEFS})
target_link_libraries(FwPitchBendSnapshot PRIVATE FwCommon juce::juce_recommended_config_flags)

# Unit tests (DSP + processor):  cmake --build build --target FwPitchBendTests, then ctest --test-dir build -C Release
juce_add_console_app(FwPitchBendTests PRODUCT_NAME "FwPitchBendTests")
target_sources(FwPitchBendTests PRIVATE ${PB_DIR}/tests/FwPitchBendTests.cpp ${PB_SOURCES})
target_include_directories(FwPitchBendTests PRIVATE ${PB_DIR}/Source)
target_compile_definitions(FwPitchBendTests PRIVATE ${PB_APP_DEFS})
target_link_libraries(FwPitchBendTests PRIVATE FwCommon juce::juce_recommended_config_flags)
add_test(NAME FwPitchBendTests COMMAND FwPitchBendTests)

# Offline renderer for the null tests and the render regression: the plug-in's processor on raw float files
# (tools/PbRender.cpp)
juce_add_console_app(PbRender PRODUCT_NAME "PbRender")
target_sources(PbRender PRIVATE ${PB_DIR}/tools/PbRender.cpp ${PB_SOURCES})
target_include_directories(PbRender PRIVATE ${PB_DIR}/Source)
target_compile_definitions(PbRender PRIVATE ${PB_APP_DEFS})
target_link_libraries(PbRender PRIVATE FwCommon juce::juce_recommended_config_flags)

# The shifter alone, offline (tools/PbShifterRender.cpp; the shifter's scorecard runs on it). No JUCE needed.
add_executable(PbShifterRender ${PB_DIR}/tools/PbShifterRender.cpp ${PB_DIR}/Source/dsp/Shifter.cpp)
target_include_directories(PbShifterRender PRIVATE ${PB_DIR}/Source ${CMAKE_SOURCE_DIR})
fw_tool_output(PbShifterRender)

# What CI builds (the ci_targets aggregate of the root CMake file); the snapshot tool on Windows only.
add_dependencies(ci_targets FwPitchBend_VST3 FwPitchBendTests PbRender)
if(WIN32)
    add_dependencies(ci_targets FwPitchBendSnapshot)
endif()
