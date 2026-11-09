# Flatwater Vocal Comp. Included from the root CMake file after shared/shared.cmake; its build output lands in
# build/<Target>_artefacts.
set(VC_DIR ${CMAKE_CURRENT_LIST_DIR})

set(VC_SOURCES
    ${VC_DIR}/Source/PluginProcessor.cpp
    ${VC_DIR}/Source/PluginEditor.cpp)
# the snapshot / test / render executables compile the plug-in sources outside the plug-in wrapper
set(VC_APP_DEFS JucePlugin_Name="Flatwater Vocal Comp" JucePlugin_IsSynth=0 JucePlugin_WantsMidiInput=0
    JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0 JucePlugin_VersionString="${PROJECT_VERSION}")

# The manufacturer and plug-in codes, the bundle and CLAP ids make the plug-in's identity in hosts: keep them.
fw_add_plugin(FwVocalComp "Flatwater Vocal Comp" Fvc1 "com.flatwater.vocalcomp"
    SOURCES ${VC_SOURCES})
target_include_directories(FwVocalComp PRIVATE ${VC_DIR}/Source)

# Editor snapshot tool (renders the GUI to a PNG for the visual comparison; docs/screenshots/README.md).
juce_add_console_app(FwVocalCompSnapshot PRODUCT_NAME "FwVocalCompSnapshot")
target_sources(FwVocalCompSnapshot PRIVATE ${VC_DIR}/tools/Snapshot.cpp ${VC_SOURCES})
target_include_directories(FwVocalCompSnapshot PRIVATE ${VC_DIR}/Source)
target_compile_definitions(FwVocalCompSnapshot PRIVATE ${VC_APP_DEFS})
target_link_libraries(FwVocalCompSnapshot PRIVATE FwCommon juce::juce_recommended_config_flags)

# Unit tests (DSP + processor):  cmake --build build --target FwVocalCompTests, then ctest --test-dir build -C Release
juce_add_console_app(FwVocalCompTests PRODUCT_NAME "FwVocalCompTests")
target_sources(FwVocalCompTests PRIVATE ${VC_DIR}/tests/FwVocalCompTests.cpp ${VC_SOURCES})
target_include_directories(FwVocalCompTests PRIVATE ${VC_DIR}/Source)
target_compile_definitions(FwVocalCompTests PRIVATE ${VC_APP_DEFS})
target_link_libraries(FwVocalCompTests PRIVATE FwCommon juce::juce_recommended_config_flags)
add_test(NAME FwVocalCompTests COMMAND FwVocalCompTests)

# Offline renderer for the null tests and the render regression (the DSP alone, no JUCE; tools/VcRender.cpp)
add_executable(VcRender ${VC_DIR}/tools/VcRender.cpp)
target_include_directories(VcRender PRIVATE ${VC_DIR}/Source ${CMAKE_SOURCE_DIR})
fw_tool_output(VcRender)

# What CI builds (the ci_targets aggregate of the root CMake file); the snapshot tool on Windows only.
add_dependencies(ci_targets FwVocalComp_VST3 FwVocalCompTests VcRender)
if(WIN32)
    add_dependencies(ci_targets FwVocalCompSnapshot)
endif()
