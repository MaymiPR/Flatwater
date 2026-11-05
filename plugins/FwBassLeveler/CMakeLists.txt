fw_add_plugin(FwBassLeveler "Flatwater Bass Leveler" Fbe1 "com.flatwater.bassleveler"
    CATEGORIES Fx Dynamics
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/FwBassLevelerProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/FwBassLevelerEditor.cpp)
target_include_directories(FwBassLeveler PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Editor snapshot tool (renders the GUI to a PNG for the visual comparison).
juce_add_console_app(FwBassLevelerSnapshot PRODUCT_NAME "FwBassLevelerSnapshot")
target_sources(FwBassLevelerSnapshot PRIVATE
    ${CMAKE_SOURCE_DIR}/tools/Snapshot.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwBassLevelerProcessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwBassLevelerEditor.cpp)
target_include_directories(FwBassLevelerSnapshot PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(FwBassLevelerSnapshot PRIVATE
    JucePlugin_Name="Flatwater Bass Leveler" JucePlugin_IsSynth=0 JucePlugin_WantsMidiInput=0
    JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0 JucePlugin_VersionString="0.1.0")
target_link_libraries(FwBassLevelerSnapshot PRIVATE FwCommon juce::juce_recommended_config_flags)

# Unit tests (parameters, processor, editor):  cmake --build build --target FwBassLevelerTests, then ctest --test-dir build -C Release
juce_add_console_app(FwBassLevelerTests PRODUCT_NAME "FwBassLevelerTests")
target_sources(FwBassLevelerTests PRIVATE
    ${CMAKE_SOURCE_DIR}/tests/FwBassLevelerTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwBassLevelerProcessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwBassLevelerEditor.cpp)
target_include_directories(FwBassLevelerTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(FwBassLevelerTests PRIVATE
    JucePlugin_Name="Flatwater Bass Leveler" JucePlugin_IsSynth=0 JucePlugin_WantsMidiInput=0
    JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0 JucePlugin_VersionString="0.1.0")
target_link_libraries(FwBassLevelerTests PRIVATE FwCommon juce::juce_recommended_config_flags)
add_test(NAME FwBassLevelerTests COMMAND FwBassLevelerTests)

# Offline renderer for the null tests: the plug-in's processor on raw float files (tools/FwBassLevelerRender.cpp)
juce_add_console_app(FwBassLevelerRender PRODUCT_NAME "FwBassLevelerRender")
target_sources(FwBassLevelerRender PRIVATE
    ${CMAKE_SOURCE_DIR}/tools/FwBassLevelerRender.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwBassLevelerProcessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwBassLevelerEditor.cpp)
target_include_directories(FwBassLevelerRender PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(FwBassLevelerRender PRIVATE
    JucePlugin_Name="Flatwater Bass Leveler" JucePlugin_IsSynth=0 JucePlugin_WantsMidiInput=0
    JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0 JucePlugin_VersionString="0.1.0")
target_link_libraries(FwBassLevelerRender PRIVATE FwCommon juce::juce_recommended_config_flags)

# What CI builds (the ci_targets aggregate, root CMakeLists.txt); the snapshot tool on Windows only.
add_dependencies(ci_targets FwBassLeveler_VST3 FwBassLevelerTests FwBassLevelerRender)
if(WIN32)
    add_dependencies(ci_targets FwBassLevelerSnapshot)
endif()
