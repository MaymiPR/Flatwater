# Flatwater Vocal Leveler. Included from the root CMake file after shared/shared.cmake; its build output lands in
# build/<Target>_artefacts.
set(VL_DIR ${CMAKE_CURRENT_LIST_DIR})

set(VL_SOURCES
    ${VL_DIR}/Source/PluginProcessor.cpp
    ${VL_DIR}/Source/PluginEditor.cpp
    ${VL_DIR}/Source/Presets.cpp)
# the snapshot / test / render executables compile the plug-in sources outside the plug-in wrapper
set(VL_APP_DEFS JucePlugin_Name="Flatwater Vocal Leveler" JucePlugin_IsSynth=0 JucePlugin_WantsMidiInput=0
    JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0 JucePlugin_VersionString="${PROJECT_VERSION}")

# The manufacturer and plug-in codes, the bundle and CLAP ids make the plug-in's identity in hosts: keep them.
flatwater_add_plugin(FwVocalLeveler "Flatwater Vocal Leveler" Fve1 "com.flatwater.vocalleveler"
    CATEGORIES Fx Dynamics
    SOURCES ${VL_SOURCES})
target_include_directories(FwVocalLeveler PRIVATE ${VL_DIR}/Source)

# Unit tests (DSP + processor):  cmake --build build --target FwVocalLevelerTests, then ctest --test-dir build -C Release
juce_add_console_app(FwVocalLevelerTests PRODUCT_NAME "FwVocalLevelerTests")
target_sources(FwVocalLevelerTests PRIVATE ${VL_DIR}/tests/FwVocalLevelerTests.cpp ${VL_SOURCES})
target_include_directories(FwVocalLevelerTests PRIVATE ${VL_DIR}/Source)
target_compile_definitions(FwVocalLevelerTests PRIVATE ${VL_APP_DEFS})
target_link_libraries(FwVocalLevelerTests PRIVATE FlatwaterShared juce::juce_recommended_config_flags)
add_test(NAME FwVocalLevelerTests COMMAND FwVocalLevelerTests)

flatwater_add_host_test(FwVocalLeveler)

# What CI builds (the ci_targets aggregate of the root CMake file): the VST3, the tests, and with the tools
# the renderer and, on Windows, the snapshot tool.
add_dependencies(ci_targets FwVocalLeveler_VST3 FwVocalLevelerTests)

if(FLATWATER_TOOLS)
    # Editor snapshot tool (renders the GUI to a PNG for the visual comparison; docs/screenshots/README.md).
    juce_add_console_app(FwVocalLevelerSnapshot PRODUCT_NAME "FwVocalLevelerSnapshot")
    target_sources(FwVocalLevelerSnapshot PRIVATE ${CMAKE_SOURCE_DIR}/tools/Snapshot.cpp ${VL_SOURCES})
    target_include_directories(FwVocalLevelerSnapshot PRIVATE ${VL_DIR}/Source)
    target_compile_definitions(FwVocalLevelerSnapshot PRIVATE ${VL_APP_DEFS})
    target_link_libraries(FwVocalLevelerSnapshot PRIVATE FlatwaterShared juce::juce_recommended_config_flags)

    # Offline renderer for the null tests and the render regression: the plug-in's processor on raw float files
    # (tools/VlRender.cpp)
    juce_add_console_app(VlRender PRODUCT_NAME "VlRender")
    target_sources(VlRender PRIVATE ${VL_DIR}/tools/VlRender.cpp ${VL_SOURCES})
    target_include_directories(VlRender PRIVATE ${VL_DIR}/Source)
    target_compile_definitions(VlRender PRIVATE ${VL_APP_DEFS})
    target_link_libraries(VlRender PRIVATE FlatwaterShared juce::juce_recommended_config_flags)

    add_dependencies(ci_targets VlRender)
    if(WIN32)
        add_dependencies(ci_targets FwVocalLevelerSnapshot)
    endif()
endif()
