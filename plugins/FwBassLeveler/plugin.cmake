# Flatwater Bass Leveler. Included from the root CMake file after shared/shared.cmake; its build output lands in
# build/<Target>_artefacts.
set(BL_DIR ${CMAKE_CURRENT_LIST_DIR})

set(BL_SOURCES
    ${BL_DIR}/Source/PluginProcessor.cpp
    ${BL_DIR}/Source/PluginEditor.cpp
    ${BL_DIR}/Source/Presets.cpp)
# the snapshot / test / render executables compile the plug-in sources outside the plug-in wrapper
set(BL_APP_DEFS JucePlugin_Name="Flatwater Bass Leveler" JucePlugin_IsSynth=0 JucePlugin_WantsMidiInput=0
    JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0 JucePlugin_VersionString="${PROJECT_VERSION}")

# The manufacturer and plug-in codes, the bundle and CLAP ids make the plug-in's identity in hosts: keep them.
flatwater_add_plugin(FwBassLeveler "Flatwater Bass Leveler" Fbe1 "com.flatwater.bassleveler"
    CATEGORIES Fx Dynamics
    SOURCES ${BL_SOURCES})
target_include_directories(FwBassLeveler PRIVATE ${BL_DIR}/Source)

# Unit tests (DSP + processor):  cmake --build build --target FwBassLevelerTests, then ctest --test-dir build -C Release
juce_add_console_app(FwBassLevelerTests PRODUCT_NAME "FwBassLevelerTests")
target_sources(FwBassLevelerTests PRIVATE ${BL_DIR}/tests/FwBassLevelerTests.cpp ${BL_SOURCES})
target_include_directories(FwBassLevelerTests PRIVATE ${BL_DIR}/Source)
target_compile_definitions(FwBassLevelerTests PRIVATE ${BL_APP_DEFS})
target_link_libraries(FwBassLevelerTests PRIVATE FlatwaterShared juce::juce_recommended_config_flags)
add_test(NAME FwBassLevelerTests COMMAND FwBassLevelerTests)

flatwater_add_host_test(FwBassLeveler)

# What CI builds (the ci_targets aggregate of the root CMake file): the VST3, the tests, and with the tools
# the renderer and, on Windows, the snapshot tool.
add_dependencies(ci_targets FwBassLeveler_VST3 FwBassLevelerTests)

if(FLATWATER_TOOLS)
    # Editor snapshot tool (renders the GUI to a PNG for the visual comparison; docs/screenshots/README.md).
    juce_add_console_app(FwBassLevelerSnapshot PRODUCT_NAME "FwBassLevelerSnapshot")
    target_sources(FwBassLevelerSnapshot PRIVATE ${CMAKE_SOURCE_DIR}/tools/Snapshot.cpp ${BL_SOURCES})
    target_include_directories(FwBassLevelerSnapshot PRIVATE ${BL_DIR}/Source)
    target_compile_definitions(FwBassLevelerSnapshot PRIVATE ${BL_APP_DEFS})
    target_link_libraries(FwBassLevelerSnapshot PRIVATE FlatwaterShared juce::juce_recommended_config_flags)

    # Offline renderer for the null tests and the render regression: the plug-in's processor on raw float files
    # (tools/BlRender.cpp)
    juce_add_console_app(BlRender PRODUCT_NAME "BlRender")
    target_sources(BlRender PRIVATE ${BL_DIR}/tools/BlRender.cpp ${BL_SOURCES})
    target_include_directories(BlRender PRIVATE ${BL_DIR}/Source)
    target_compile_definitions(BlRender PRIVATE ${BL_APP_DEFS})
    target_link_libraries(BlRender PRIVATE FlatwaterShared juce::juce_recommended_config_flags)

    add_dependencies(ci_targets BlRender)
    if(WIN32)
        add_dependencies(ci_targets FwBassLevelerSnapshot)
    endif()
endif()
