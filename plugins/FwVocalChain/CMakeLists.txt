# the reverb's measured responses per rate (nulltest/pack_vocalchain_reverb.py)
juce_add_binary_data(FwVocalChainIR HEADER_NAME FwVocalChainIR.h NAMESPACE FwVocalChainIR SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/ir/reverb_44100.bin ${CMAKE_CURRENT_SOURCE_DIR}/ir/reverb_48000.bin
    ${CMAKE_CURRENT_SOURCE_DIR}/ir/reverb_88200.bin ${CMAKE_CURRENT_SOURCE_DIR}/ir/reverb_96000.bin)
set_target_properties(FwVocalChainIR PROPERTIES POSITION_INDEPENDENT_CODE ON)

fw_add_plugin(FwVocalChain "Flatwater Vocal Chain" Fch1 "com.flatwater.vocalchain"
    CATEGORIES Fx
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/FwVocalChainProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/FwVocalChainDsp.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/FwVocalChainEditor.cpp)
target_include_directories(FwVocalChain PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/plugins/FwDelay ${CMAKE_SOURCE_DIR}/plugins/FwComp)
target_link_libraries(FwVocalChain PRIVATE FwVocalChainIR)

# Editor snapshot tool (renders the GUI to a PNG for the visual comparison).
juce_add_console_app(FwVocalChainSnapshot PRODUCT_NAME "FwVocalChainSnapshot")
target_sources(FwVocalChainSnapshot PRIVATE
    ${CMAKE_SOURCE_DIR}/tools/Snapshot.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwVocalChainProcessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwVocalChainDsp.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwVocalChainEditor.cpp)
target_include_directories(FwVocalChainSnapshot PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/plugins/FwDelay ${CMAKE_SOURCE_DIR}/plugins/FwComp)
target_compile_definitions(FwVocalChainSnapshot PRIVATE
    JucePlugin_Name="Flatwater Vocal Chain" JucePlugin_IsSynth=0 JucePlugin_WantsMidiInput=0
    JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0 JucePlugin_VersionString="0.1.0")
target_link_libraries(FwVocalChainSnapshot PRIVATE FwCommon FwVocalChainIR juce::juce_recommended_config_flags)

# Unit tests (parameters, processor, editor):  cmake --build build --target FwVocalChainTests, then ctest --test-dir build -C Release
juce_add_console_app(FwVocalChainTests PRODUCT_NAME "FwVocalChainTests")
target_sources(FwVocalChainTests PRIVATE
    ${CMAKE_SOURCE_DIR}/tests/FwVocalChainTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwVocalChainProcessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwVocalChainDsp.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwVocalChainEditor.cpp)
target_include_directories(FwVocalChainTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/plugins/FwDelay ${CMAKE_SOURCE_DIR}/plugins/FwComp)
target_compile_definitions(FwVocalChainTests PRIVATE
    JucePlugin_Name="Flatwater Vocal Chain" JucePlugin_IsSynth=0 JucePlugin_WantsMidiInput=0
    JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0 JucePlugin_VersionString="0.1.0")
target_link_libraries(FwVocalChainTests PRIVATE FwCommon FwVocalChainIR juce::juce_recommended_config_flags)
add_test(NAME FwVocalChainTests COMMAND FwVocalChainTests)


# Offline renderer for the null tests: the plug-in's processor on raw float files (tools/FwVocalChainRender.cpp)
juce_add_console_app(FwVocalChainRender PRODUCT_NAME "FwVocalChainRender")
target_sources(FwVocalChainRender PRIVATE
    ${CMAKE_SOURCE_DIR}/tools/FwVocalChainRender.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwVocalChainProcessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwVocalChainDsp.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FwVocalChainEditor.cpp)
target_include_directories(FwVocalChainRender PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/plugins/FwDelay ${CMAKE_SOURCE_DIR}/plugins/FwComp)
target_compile_definitions(FwVocalChainRender PRIVATE
    JucePlugin_Name="Flatwater Vocal Chain" JucePlugin_IsSynth=0 JucePlugin_WantsMidiInput=0
    JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0 JucePlugin_VersionString="0.1.0")
target_link_libraries(FwVocalChainRender PRIVATE FwCommon FwVocalChainIR juce::juce_recommended_config_flags)

# What CI builds (the ci_targets aggregate, root CMakeLists.txt); the snapshot tool on Windows only.
add_dependencies(ci_targets FwVocalChain_VST3 FwVocalChainTests FwVocalChainRender)
if(WIN32)
    add_dependencies(ci_targets FwVocalChainSnapshot)
endif()
