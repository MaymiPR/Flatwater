fw_add_plugin(FwVocalLeveler "Flatwater Vocal Leveler" Fve1 "com.flatwater.vocalleveler"
    CATEGORIES Fx Dynamics
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/FwVocalLevelerProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/FwVocalLevelerEditor.cpp)
target_include_directories(FwVocalLeveler PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Editor snapshot tool (renders the GUI to a PNG for the visual comparison).
juce_add_console_app(FwVocalLevelerSnapshot PRODUCT_NAME "FwVocalLevelerSnapshot")
target_sources(FwVocalLevelerSnapshot PRIVATE
    ${CMAKE_SOURCE_DIR}/tools/Snapshot.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwVocalLevelerProcessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwVocalLevelerEditor.cpp)
target_include_directories(FwVocalLevelerSnapshot PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(FwVocalLevelerSnapshot PRIVATE
    JucePlugin_Name="Flatwater Vocal Leveler" JucePlugin_IsSynth=0 JucePlugin_WantsMidiInput=0
    JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0 JucePlugin_VersionString="0.1.0")
target_link_libraries(FwVocalLevelerSnapshot PRIVATE FwCommon juce::juce_recommended_config_flags)

# Unit tests (parameters, processor, editor):  cmake --build build --target FwVocalLevelerTests, then ctest --test-dir build -C Release
juce_add_console_app(FwVocalLevelerTests PRODUCT_NAME "FwVocalLevelerTests")
target_sources(FwVocalLevelerTests PRIVATE
    ${CMAKE_SOURCE_DIR}/tests/FwVocalLevelerTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwVocalLevelerProcessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwVocalLevelerEditor.cpp)
target_include_directories(FwVocalLevelerTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(FwVocalLevelerTests PRIVATE
    JucePlugin_Name="Flatwater Vocal Leveler" JucePlugin_IsSynth=0 JucePlugin_WantsMidiInput=0
    JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0 JucePlugin_VersionString="0.1.0")
target_link_libraries(FwVocalLevelerTests PRIVATE FwCommon juce::juce_recommended_config_flags)
add_test(NAME FwVocalLevelerTests COMMAND FwVocalLevelerTests)

# Offline renderer for the null tests: the plug-in's processor on raw float files (tools/FwVocalLevelerRender.cpp)
juce_add_console_app(FwVocalLevelerRender PRODUCT_NAME "FwVocalLevelerRender")
target_sources(FwVocalLevelerRender PRIVATE
    ${CMAKE_SOURCE_DIR}/tools/FwVocalLevelerRender.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwVocalLevelerProcessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwVocalLevelerEditor.cpp)
target_include_directories(FwVocalLevelerRender PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(FwVocalLevelerRender PRIVATE
    JucePlugin_Name="Flatwater Vocal Leveler" JucePlugin_IsSynth=0 JucePlugin_WantsMidiInput=0
    JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0 JucePlugin_VersionString="0.1.0")
target_link_libraries(FwVocalLevelerRender PRIVATE FwCommon juce::juce_recommended_config_flags)

# What CI builds (the ci_targets aggregate, root CMakeLists.txt); the snapshot tool on Windows only.
add_dependencies(ci_targets FwVocalLeveler_VST3 FwVocalLevelerTests FwVocalLevelerRender)
if(WIN32)
    add_dependencies(ci_targets FwVocalLevelerSnapshot)
endif()
