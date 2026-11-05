fw_add_plugin(FwVocalComp "Flatwater Vocal Comp" Fvc1 "com.flatwater.vocalcomp"
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/FwVocalCompProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/FwVocalCompEditor.cpp)
target_include_directories(FwVocalComp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Editor snapshot tool (renders the GUI to a PNG for the visual comparison).
juce_add_console_app(FwVocalCompSnapshot PRODUCT_NAME "FwVocalCompSnapshot")
target_sources(FwVocalCompSnapshot PRIVATE
    ${CMAKE_SOURCE_DIR}/tools/FwVocalCompSnapshot.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwVocalCompProcessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwVocalCompEditor.cpp)
target_include_directories(FwVocalCompSnapshot PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(FwVocalCompSnapshot PRIVATE
    JucePlugin_Name="Flatwater Vocal Comp" JucePlugin_IsSynth=0 JucePlugin_WantsMidiInput=0
    JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0 JucePlugin_VersionString="0.1.0")
target_link_libraries(FwVocalCompSnapshot PRIVATE FwCommon juce::juce_recommended_config_flags)

# What CI builds (the ci_targets aggregate, root CMakeLists.txt) (its tests and renderer join in the root CMakeLists.txt); the snapshot tool on Windows only.
add_dependencies(ci_targets FwVocalComp_VST3)
if(WIN32)
    add_dependencies(ci_targets FwVocalCompSnapshot)
endif()
