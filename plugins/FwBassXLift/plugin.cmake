# Flatwater BassX Lift. Included from the root CMake file after shared/shared.cmake; its build output lands in
# build/<Target>_artefacts.
set(BX_DIR ${CMAKE_CURRENT_LIST_DIR})

set(BX_SOURCES
    ${BX_DIR}/Source/PluginProcessor.cpp
    ${BX_DIR}/Source/PluginEditor.cpp)
# the snapshot / test / render executables compile the plug-in sources outside the plug-in wrapper
set(BX_APP_DEFS JucePlugin_Name="Flatwater BassX Lift" JucePlugin_IsSynth=0 JucePlugin_WantsMidiInput=0
    JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0 JucePlugin_VersionString="${PROJECT_VERSION}")

# The manufacturer and plug-in codes, the bundle and CLAP ids make the plug-in's identity in hosts: keep them.
fw_add_plugin(FwBassXLift "Flatwater BassX Lift" Fbx1 "com.flatwater.bassxlift"
    SOURCES ${BX_SOURCES})
target_include_directories(FwBassXLift PRIVATE ${BX_DIR}/Source)

# Editor snapshot tool (renders the GUI to a PNG for the visual comparison; docs/screenshots/README.md).
juce_add_console_app(FwBassXLiftSnapshot PRODUCT_NAME "FwBassXLiftSnapshot")
target_sources(FwBassXLiftSnapshot PRIVATE ${CMAKE_SOURCE_DIR}/tools/Snapshot.cpp ${BX_SOURCES})
target_include_directories(FwBassXLiftSnapshot PRIVATE ${BX_DIR}/Source)
target_compile_definitions(FwBassXLiftSnapshot PRIVATE ${BX_APP_DEFS})
target_link_libraries(FwBassXLiftSnapshot PRIVATE FwCommon juce::juce_recommended_config_flags)

# Unit tests (DSP + processor):  cmake --build build --target FwBassXLiftTests, then ctest --test-dir build -C Release
juce_add_console_app(FwBassXLiftTests PRODUCT_NAME "FwBassXLiftTests")
target_sources(FwBassXLiftTests PRIVATE ${BX_DIR}/tests/FwBassXLiftTests.cpp ${BX_SOURCES})
target_include_directories(FwBassXLiftTests PRIVATE ${BX_DIR}/Source)
target_compile_definitions(FwBassXLiftTests PRIVATE ${BX_APP_DEFS})
target_link_libraries(FwBassXLiftTests PRIVATE FwCommon juce::juce_recommended_config_flags)
add_test(NAME FwBassXLiftTests COMMAND FwBassXLiftTests)

# Offline renderer for the null tests and the render regression (the DSP alone, no JUCE; tools/BxRender.cpp)
add_executable(BxRender ${BX_DIR}/tools/BxRender.cpp)
target_include_directories(BxRender PRIVATE ${BX_DIR}/Source ${CMAKE_SOURCE_DIR})
fw_tool_output(BxRender)

# What CI builds (the ci_targets aggregate of the root CMake file); the snapshot tool on Windows only.
add_dependencies(ci_targets FwBassXLift_VST3 FwBassXLiftTests BxRender)
if(WIN32)
    add_dependencies(ci_targets FwBassXLiftSnapshot)
endif()
