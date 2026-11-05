fw_add_plugin(FwBassLift "Flatwater Bass Lift" Fbs1 "com.flatwater.basslift"
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/FwBassLiftProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/FwBassLiftEditor.cpp)
target_include_directories(FwBassLift PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Editor snapshot tool (renders the GUI to a PNG for the visual comparison).
juce_add_console_app(FwBassLiftSnapshot PRODUCT_NAME "FwBassLiftSnapshot")
target_sources(FwBassLiftSnapshot PRIVATE
    ${CMAKE_SOURCE_DIR}/tools/Snapshot.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwBassLiftProcessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwBassLiftEditor.cpp)
target_include_directories(FwBassLiftSnapshot PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(FwBassLiftSnapshot PRIVATE
    JucePlugin_Name="Flatwater Bass Lift" JucePlugin_IsSynth=0 JucePlugin_WantsMidiInput=0
    JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0 JucePlugin_VersionString="0.1.0")
target_link_libraries(FwBassLiftSnapshot PRIVATE FwCommon juce::juce_recommended_config_flags)

# Offline renderer of the Flatwater Bass Lift DSP (null tests and fitting scripts)
add_executable(FwBassLiftRender ${CMAKE_SOURCE_DIR}/tools/FwBassLiftRender.cpp)
target_include_directories(FwBassLiftRender PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Unit tests (DSP + processor):  cmake --build build --target FwBassLiftTests, then ctest --test-dir build -C Release
juce_add_console_app(FwBassLiftTests PRODUCT_NAME "FwBassLiftTests")
target_sources(FwBassLiftTests PRIVATE
    ${CMAKE_SOURCE_DIR}/tests/FwBassLiftTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwBassLiftProcessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwBassLiftEditor.cpp)
target_include_directories(FwBassLiftTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(FwBassLiftTests PRIVATE
    JucePlugin_Name="Flatwater Bass Lift" JucePlugin_IsSynth=0 JucePlugin_WantsMidiInput=0
    JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0 JucePlugin_VersionString="0.1.0")
target_link_libraries(FwBassLiftTests PRIVATE FwCommon juce::juce_recommended_config_flags)
add_test(NAME FwBassLiftTests COMMAND FwBassLiftTests)

# What CI builds (the ci_targets aggregate, root CMakeLists.txt); the snapshot tool on Windows only.
add_dependencies(ci_targets FwBassLift_VST3 FwBassLiftTests FwBassLiftRender)
if(WIN32)
    add_dependencies(ci_targets FwBassLiftSnapshot)
endif()
