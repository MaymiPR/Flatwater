# Flatwater Bass Lift. Included from the root CMake file after shared/shared.cmake; its build output lands in
# build/<Target>_artefacts.
set(BF_DIR ${CMAKE_CURRENT_LIST_DIR})

set(BF_SOURCES
    ${BF_DIR}/Source/PluginProcessor.cpp
    ${BF_DIR}/Source/PluginEditor.cpp)
# the snapshot / test / render executables compile the plug-in sources outside the plug-in wrapper
set(BF_APP_DEFS JucePlugin_Name="Flatwater Bass Lift" JucePlugin_IsSynth=0 JucePlugin_WantsMidiInput=0
    JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0 JucePlugin_VersionString="${PROJECT_VERSION}")

# The manufacturer and plug-in codes, the bundle and CLAP ids make the plug-in's identity in hosts: keep them.
fw_add_plugin(FwBassLift "Flatwater Bass Lift" Fbs1 "com.flatwater.basslift"
    SOURCES ${BF_SOURCES})
target_include_directories(FwBassLift PRIVATE ${BF_DIR}/Source)

# Editor snapshot tool (renders the GUI to a PNG for the visual comparison; docs/screenshots/README.md).
juce_add_console_app(FwBassLiftSnapshot PRODUCT_NAME "FwBassLiftSnapshot")
target_sources(FwBassLiftSnapshot PRIVATE ${CMAKE_SOURCE_DIR}/tools/Snapshot.cpp ${BF_SOURCES})
target_include_directories(FwBassLiftSnapshot PRIVATE ${BF_DIR}/Source)
target_compile_definitions(FwBassLiftSnapshot PRIVATE ${BF_APP_DEFS})
target_link_libraries(FwBassLiftSnapshot PRIVATE FwCommon juce::juce_recommended_config_flags)

# Unit tests (DSP + processor):  cmake --build build --target FwBassLiftTests, then ctest --test-dir build -C Release
juce_add_console_app(FwBassLiftTests PRODUCT_NAME "FwBassLiftTests")
target_sources(FwBassLiftTests PRIVATE ${BF_DIR}/tests/FwBassLiftTests.cpp ${BF_SOURCES})
target_include_directories(FwBassLiftTests PRIVATE ${BF_DIR}/Source)
target_compile_definitions(FwBassLiftTests PRIVATE ${BF_APP_DEFS})
target_link_libraries(FwBassLiftTests PRIVATE FwCommon juce::juce_recommended_config_flags)
add_test(NAME FwBassLiftTests COMMAND FwBassLiftTests)

# Offline renderer for the null tests and the render regression (the DSP alone, no JUCE; tools/BfRender.cpp)
add_executable(BfRender ${BF_DIR}/tools/BfRender.cpp)
target_include_directories(BfRender PRIVATE ${BF_DIR}/Source ${CMAKE_SOURCE_DIR})
fw_tool_output(BfRender)

# What CI builds (the ci_targets aggregate of the root CMake file); the snapshot tool on Windows only.
add_dependencies(ci_targets FwBassLift_VST3 FwBassLiftTests BfRender)
if(WIN32)
    add_dependencies(ci_targets FwBassLiftSnapshot)
endif()
