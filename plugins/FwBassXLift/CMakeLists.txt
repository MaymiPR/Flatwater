fw_add_plugin(FwBassXLift "Flatwater BassX Lift" Fbx1 "com.flatwater.bassxlift"
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/FwBassXLiftProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/FwBassXLiftEditor.cpp)
target_include_directories(FwBassXLift PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Editor snapshot tool (renders the GUI to a PNG for the visual comparison).
juce_add_console_app(FwBassXLiftSnapshot PRODUCT_NAME "FwBassXLiftSnapshot")
target_sources(FwBassXLiftSnapshot PRIVATE
    ${CMAKE_SOURCE_DIR}/tools/Snapshot.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwBassXLiftProcessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwBassXLiftEditor.cpp)
target_include_directories(FwBassXLiftSnapshot PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(FwBassXLiftSnapshot PRIVATE
    JucePlugin_Name="Flatwater BassX Lift" JucePlugin_IsSynth=0 JucePlugin_WantsMidiInput=0
    JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0 JucePlugin_VersionString="0.1.0")
target_link_libraries(FwBassXLiftSnapshot PRIVATE FwCommon juce::juce_recommended_config_flags)

# Offline renderer of the DSP for the null test (nulltest/nulltest_fwbassxlift.py).
add_executable(FwBassXLiftRender ${CMAKE_SOURCE_DIR}/tools/FwBassXLiftRender.cpp)
target_include_directories(FwBassXLiftRender PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Unit tests (DSP + processor):  cmake --build build --target FwBassXLiftTests, then ctest --test-dir build -C Release
juce_add_console_app(FwBassXLiftTests PRODUCT_NAME "FwBassXLiftTests")
target_sources(FwBassXLiftTests PRIVATE
    ${CMAKE_SOURCE_DIR}/tests/FwBassXLiftTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwBassXLiftProcessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwBassXLiftEditor.cpp)
target_include_directories(FwBassXLiftTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(FwBassXLiftTests PRIVATE
    JucePlugin_Name="Flatwater BassX Lift" JucePlugin_IsSynth=0 JucePlugin_WantsMidiInput=0
    JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0 JucePlugin_VersionString="0.1.0")
target_link_libraries(FwBassXLiftTests PRIVATE FwCommon juce::juce_recommended_config_flags)
add_test(NAME FwBassXLiftTests COMMAND FwBassXLiftTests)

# What CI builds (the ci_targets aggregate, root CMakeLists.txt); the snapshot tool on Windows only.
add_dependencies(ci_targets FwBassXLift_VST3 FwBassXLiftTests FwBassXLiftRender)
if(WIN32)
    add_dependencies(ci_targets FwBassXLiftSnapshot)
endif()
