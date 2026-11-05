fw_add_plugin(FwComp "Flatwater Comp" Fcp1 "com.flatwater.comp"
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/FwCompProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/FwCompEditor.cpp)
target_include_directories(FwComp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Editor snapshot tool (renders the GUI to a PNG for the visual comparison).
juce_add_console_app(FwCompSnapshot PRODUCT_NAME "FwCompSnapshot")
target_sources(FwCompSnapshot PRIVATE
    ${CMAKE_SOURCE_DIR}/tools/Snapshot.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwCompProcessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwCompEditor.cpp)
target_include_directories(FwCompSnapshot PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(FwCompSnapshot PRIVATE
    JucePlugin_Name="Flatwater Comp" JucePlugin_IsSynth=0 JucePlugin_WantsMidiInput=0
    JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0 JucePlugin_VersionString="0.1.0")
target_link_libraries(FwCompSnapshot PRIVATE FwCommon juce::juce_recommended_config_flags)

# Offline renderer of the DSP for the null tests (nulltest/nulltest_fwcomp.py).
add_executable(FwCompRender ${CMAKE_SOURCE_DIR}/tools/FwCompRender.cpp)
target_include_directories(FwCompRender PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/Source/common)

# Unit tests (DSP + processor):  cmake --build build --target FwCompTests, then ctest --test-dir build -C Release
juce_add_console_app(FwCompTests PRODUCT_NAME "FwCompTests")
target_sources(FwCompTests PRIVATE
    ${CMAKE_SOURCE_DIR}/tests/FwCompTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwCompProcessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwCompEditor.cpp)
target_include_directories(FwCompTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(FwCompTests PRIVATE
    JucePlugin_Name="Flatwater Comp" JucePlugin_IsSynth=0 JucePlugin_WantsMidiInput=0
    JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0 JucePlugin_VersionString="0.1.0")
target_link_libraries(FwCompTests PRIVATE FwCommon juce::juce_recommended_config_flags)
add_test(NAME FwCompTests COMMAND FwCompTests)

# What CI builds (the ci_targets aggregate, root CMakeLists.txt); the snapshot tool on Windows only.
add_dependencies(ci_targets FwComp_VST3 FwCompTests FwCompRender)
if(WIN32)
    add_dependencies(ci_targets FwCompSnapshot)
endif()
