fw_add_plugin(FwPitchBend "Flatwater Pitch Bend" Fpb1 "com.flatwater.pitchbend"
    CATEGORIES Fx Pitch Shift
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/FwPitchBendProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/FwPitchBendEditor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/FwPitchBendWidgets.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/FwPitchBendShifter.cpp)
target_include_directories(FwPitchBend PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Editor snapshot tool (renders the GUI to a PNG for the visual comparison).
juce_add_console_app(FwPitchBendSnapshot PRODUCT_NAME "FwPitchBendSnapshot")
target_sources(FwPitchBendSnapshot PRIVATE
    ${CMAKE_SOURCE_DIR}/tools/Snapshot.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwPitchBendProcessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwPitchBendEditor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwPitchBendWidgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwPitchBendShifter.cpp)
target_include_directories(FwPitchBendSnapshot PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(FwPitchBendSnapshot PRIVATE
    JucePlugin_Name="Flatwater Pitch Bend" JucePlugin_IsSynth=0 JucePlugin_WantsMidiInput=0
    JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0 JucePlugin_VersionString="0.1.0")
target_link_libraries(FwPitchBendSnapshot PRIVATE FwCommon juce::juce_recommended_config_flags)

# Unit tests (parameters, processor, editor):  cmake --build build --target FwPitchBendTests, then ctest --test-dir build -C Release
juce_add_console_app(FwPitchBendTests PRODUCT_NAME "FwPitchBendTests")
target_sources(FwPitchBendTests PRIVATE
    ${CMAKE_SOURCE_DIR}/tests/FwPitchBendTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwPitchBendProcessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwPitchBendEditor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwPitchBendWidgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwPitchBendShifter.cpp)
target_include_directories(FwPitchBendTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(FwPitchBendTests PRIVATE
    JucePlugin_Name="Flatwater Pitch Bend" JucePlugin_IsSynth=0 JucePlugin_WantsMidiInput=0
    JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0 JucePlugin_VersionString="0.1.0")
target_link_libraries(FwPitchBendTests PRIVATE FwCommon juce::juce_recommended_config_flags)
add_test(NAME FwPitchBendTests COMMAND FwPitchBendTests)

# Offline renderer for the null tests: the plug-in's processor on raw float files (tools/FwPitchBendRender.cpp)
juce_add_console_app(FwPitchBendRender PRODUCT_NAME "FwPitchBendRender")
target_sources(FwPitchBendRender PRIVATE
    ${CMAKE_SOURCE_DIR}/tools/FwPitchBendRender.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwPitchBendProcessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwPitchBendEditor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwPitchBendWidgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwPitchBendShifter.cpp)
target_include_directories(FwPitchBendRender PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(FwPitchBendRender PRIVATE
    JucePlugin_Name="Flatwater Pitch Bend" JucePlugin_IsSynth=0 JucePlugin_WantsMidiInput=0
    JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0 JucePlugin_VersionString="0.1.0")
target_link_libraries(FwPitchBendRender PRIVATE FwCommon juce::juce_recommended_config_flags)

# The shifter alone, offline (tools/FwPitchBendShifterRender.cpp; the shifter's scorecard runs on it). No JUCE needed.
add_executable(FwPitchBendShifterRender ${CMAKE_SOURCE_DIR}/tools/FwPitchBendShifterRender.cpp ${CMAKE_CURRENT_SOURCE_DIR}/FwPitchBendShifter.cpp)
target_include_directories(FwPitchBendShifterRender PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/Source/common)

# What CI builds (the ci_targets aggregate, root CMakeLists.txt); the snapshot tool on Windows only.
add_dependencies(ci_targets FwPitchBend_VST3 FwPitchBendTests FwPitchBendRender)
if(WIN32)
    add_dependencies(ci_targets FwPitchBendSnapshot)
endif()
