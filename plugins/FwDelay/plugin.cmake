# Flatwater Delay. Included from the root CMake file after shared/shared.cmake; its build output lands in
# build/<Target>_artefacts.
set(DE_DIR ${CMAKE_CURRENT_LIST_DIR})

set(DE_SOURCES
    ${DE_DIR}/Source/PluginProcessor.cpp
    ${DE_DIR}/Source/PluginEditor.cpp
    ${DE_DIR}/Source/Presets.cpp)
# the snapshot / test / render executables compile the plug-in sources outside the plug-in wrapper
set(DE_APP_DEFS JucePlugin_Name="Flatwater Delay" JucePlugin_IsSynth=0 JucePlugin_WantsMidiInput=0
    JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0 JucePlugin_VersionString="${PROJECT_VERSION}")

# The manufacturer and plug-in codes, the bundle and CLAP ids make the plug-in's identity in hosts: keep them.
flatwater_add_plugin(FwDelay "Flatwater Delay" Fdl1 "com.flatwater.delay"
    CATEGORIES Fx Delay
    SOURCES ${DE_SOURCES})
target_include_directories(FwDelay PRIVATE ${DE_DIR}/Source)

# Unit tests (DSP + processor):  cmake --build build --target FwDelayTests, then ctest --test-dir build -C Release
juce_add_console_app(FwDelayTests PRODUCT_NAME "FwDelayTests")
target_sources(FwDelayTests PRIVATE ${DE_DIR}/tests/FwDelayTests.cpp ${DE_SOURCES})
target_include_directories(FwDelayTests PRIVATE ${DE_DIR}/Source)
target_compile_definitions(FwDelayTests PRIVATE ${DE_APP_DEFS})
target_link_libraries(FwDelayTests PRIVATE FlatwaterShared juce::juce_recommended_config_flags)
add_test(NAME FwDelayTests COMMAND FwDelayTests)

flatwater_add_host_test(FwDelay)

# What CI builds (the ci_targets aggregate of the root CMake file): the VST3, the tests, and with the tools
# the renderer and, on Windows, the snapshot tool.
add_dependencies(ci_targets FwDelay_VST3 FwDelayTests)

if(FLATWATER_TOOLS)
    # Editor snapshot tool (renders the GUI to a PNG for the visual comparison; docs/screenshots/README.md).
    juce_add_console_app(FwDelaySnapshot PRODUCT_NAME "FwDelaySnapshot")
    target_sources(FwDelaySnapshot PRIVATE ${CMAKE_SOURCE_DIR}/tools/Snapshot.cpp ${DE_SOURCES})
    target_include_directories(FwDelaySnapshot PRIVATE ${DE_DIR}/Source)
    target_compile_definitions(FwDelaySnapshot PRIVATE ${DE_APP_DEFS})
    target_link_libraries(FwDelaySnapshot PRIVATE FlatwaterShared juce::juce_recommended_config_flags)

    # Offline renderer for the null tests and the render regression (the DSP alone, no JUCE; tools/DeRender.cpp)
    add_executable(DeRender ${DE_DIR}/tools/DeRender.cpp)
    target_include_directories(DeRender PRIVATE ${DE_DIR}/Source ${CMAKE_SOURCE_DIR})
    flatwater_tool_output(DeRender)

    add_dependencies(ci_targets DeRender)
    if(WIN32)
        add_dependencies(ci_targets FwDelaySnapshot)
    endif()
endif()
