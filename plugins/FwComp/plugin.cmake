# Flatwater Comp. Included from the root CMake file after shared/shared.cmake; its build output lands in
# build/<Target>_artefacts.
set(CO_DIR ${CMAKE_CURRENT_LIST_DIR})

set(CO_SOURCES
    ${CO_DIR}/Source/PluginProcessor.cpp
    ${CO_DIR}/Source/PluginEditor.cpp)
# the snapshot / test / render executables compile the plug-in sources outside the plug-in wrapper
set(CO_APP_DEFS JucePlugin_Name="Flatwater Comp" JucePlugin_IsSynth=0 JucePlugin_WantsMidiInput=0
    JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0 JucePlugin_VersionString="${PROJECT_VERSION}")

# The manufacturer and plug-in codes, the bundle and CLAP ids make the plug-in's identity in hosts: keep them.
fw_add_plugin(FwComp "Flatwater Comp" Fcp1 "com.flatwater.comp"
    SOURCES ${CO_SOURCES})
target_include_directories(FwComp PRIVATE ${CO_DIR}/Source)

# Editor snapshot tool (renders the GUI to a PNG for the visual comparison; docs/screenshots/README.md).
juce_add_console_app(FwCompSnapshot PRODUCT_NAME "FwCompSnapshot")
target_sources(FwCompSnapshot PRIVATE ${CMAKE_SOURCE_DIR}/tools/Snapshot.cpp ${CO_SOURCES})
target_include_directories(FwCompSnapshot PRIVATE ${CO_DIR}/Source)
target_compile_definitions(FwCompSnapshot PRIVATE ${CO_APP_DEFS})
target_link_libraries(FwCompSnapshot PRIVATE FwCommon juce::juce_recommended_config_flags)

# Unit tests (DSP + processor):  cmake --build build --target FwCompTests, then ctest --test-dir build -C Release
juce_add_console_app(FwCompTests PRODUCT_NAME "FwCompTests")
target_sources(FwCompTests PRIVATE ${CO_DIR}/tests/FwCompTests.cpp ${CO_SOURCES})
target_include_directories(FwCompTests PRIVATE ${CO_DIR}/Source)
target_compile_definitions(FwCompTests PRIVATE ${CO_APP_DEFS})
target_link_libraries(FwCompTests PRIVATE FwCommon juce::juce_recommended_config_flags)
add_test(NAME FwCompTests COMMAND FwCompTests)

# Offline renderer for the null tests and the render regression (the DSP alone, no JUCE; tools/CoRender.cpp)
add_executable(CoRender ${CO_DIR}/tools/CoRender.cpp)
target_include_directories(CoRender PRIVATE ${CO_DIR}/Source ${CMAKE_SOURCE_DIR})
fw_tool_output(CoRender)

if(WIN32)
    # renders Flatwater Comp's VU dial alone at a given needle angle, for the comparison with the filmed reference
    # (CoVuRender.cpp in this folder's tools/)
    juce_add_console_app(CoVuRender PRODUCT_NAME "CoVuRender")
    target_sources(CoVuRender PRIVATE ${CO_DIR}/tools/CoVuRender.cpp)
    target_link_libraries(CoVuRender PRIVATE FwCommon juce::juce_recommended_config_flags)
endif()

# What CI builds (the ci_targets aggregate of the root CMake file); the snapshot tool on Windows only.
add_dependencies(ci_targets FwComp_VST3 FwCompTests CoRender)
if(WIN32)
    add_dependencies(ci_targets FwCompSnapshot)
endif()
