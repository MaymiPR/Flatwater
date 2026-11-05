fw_add_plugin(FwDelay "Flatwater Delay" Fdl1 "com.flatwater.delay"
    CATEGORIES Fx Delay
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/FwDelayProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/FwDelayEditor.cpp)
target_include_directories(FwDelay PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/plugins/FwComp)

# Offline renderer for the null tests (the DSP only)
add_executable(FwDelayRender ${CMAKE_SOURCE_DIR}/tools/FwDelayRender.cpp)
target_include_directories(FwDelayRender PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/plugins/FwComp)

# Editor snapshot tool (renders the GUI to a PNG for the visual comparison).
juce_add_console_app(FwDelaySnapshot PRODUCT_NAME "FwDelaySnapshot")
target_sources(FwDelaySnapshot PRIVATE
    ${CMAKE_SOURCE_DIR}/tools/Snapshot.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwDelayProcessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwDelayEditor.cpp)
target_include_directories(FwDelaySnapshot PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/plugins/FwComp)
target_compile_definitions(FwDelaySnapshot PRIVATE
    JucePlugin_Name="Flatwater Delay" JucePlugin_IsSynth=0 JucePlugin_WantsMidiInput=0
    JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0 JucePlugin_VersionString="0.1.0")
target_link_libraries(FwDelaySnapshot PRIVATE FwCommon juce::juce_recommended_config_flags)

# Unit tests (parameters, processor, DSP):  cmake --build build --target FwDelayTests, then ctest --test-dir build -C Release
juce_add_console_app(FwDelayTests PRODUCT_NAME "FwDelayTests")
target_sources(FwDelayTests PRIVATE
    ${CMAKE_SOURCE_DIR}/tests/FwDelayTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwDelayProcessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwDelayEditor.cpp)
target_include_directories(FwDelayTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/plugins/FwComp)
target_compile_definitions(FwDelayTests PRIVATE
    JucePlugin_Name="Flatwater Delay" JucePlugin_IsSynth=0 JucePlugin_WantsMidiInput=0
    JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0 JucePlugin_VersionString="0.1.0")
target_link_libraries(FwDelayTests PRIVATE FwCommon juce::juce_recommended_config_flags)
add_test(NAME FwDelayTests COMMAND FwDelayTests)

# What CI builds (the ci_targets aggregate, root CMakeLists.txt); the snapshot tool on Windows only.
add_dependencies(ci_targets FwDelay_VST3 FwDelayTests FwDelayRender)
if(WIN32)
    add_dependencies(ci_targets FwDelaySnapshot)
endif()
