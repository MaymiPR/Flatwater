# Flatwater Vocal Chain. Included from the root CMake file after shared/shared.cmake; its build output lands in
# build/<Target>_artefacts.
set(CH_DIR ${CMAKE_CURRENT_LIST_DIR})

set(CH_SOURCES
    ${CH_DIR}/Source/PluginProcessor.cpp
    ${CH_DIR}/Source/dsp/Engine.cpp
    ${CH_DIR}/Source/PluginEditor.cpp)
# the snapshot / test / render executables compile the plug-in sources outside the plug-in wrapper
set(CH_APP_DEFS JucePlugin_Name="Flatwater Vocal Chain" JucePlugin_IsSynth=0 JucePlugin_WantsMidiInput=0
    JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0 JucePlugin_VersionString="${PROJECT_VERSION}")

# the reverb's measured responses per rate (plugins/FwVocalChain/scripts/pack_reverb.py)
juce_add_binary_data(FwVocalChainIR HEADER_NAME FwVocalChainIR.h NAMESPACE FwVocalChainIR SOURCES
    ${CH_DIR}/resources/ir/reverb_44100.bin ${CH_DIR}/resources/ir/reverb_48000.bin
    ${CH_DIR}/resources/ir/reverb_88200.bin ${CH_DIR}/resources/ir/reverb_96000.bin)
set_target_properties(FwVocalChainIR PROPERTIES POSITION_INDEPENDENT_CODE ON)

# The manufacturer and plug-in codes, the bundle and CLAP ids make the plug-in's identity in hosts: keep them.
fw_add_plugin(FwVocalChain "Flatwater Vocal Chain" Fch1 "com.flatwater.vocalchain"
    CATEGORIES Fx
    SOURCES ${CH_SOURCES})
target_include_directories(FwVocalChain PRIVATE ${CH_DIR}/Source)
target_link_libraries(FwVocalChain PRIVATE FwVocalChainIR)

# Editor snapshot tool (renders the GUI to a PNG for the visual comparison; docs/screenshots/README.md).
juce_add_console_app(FwVocalChainSnapshot PRODUCT_NAME "FwVocalChainSnapshot")
target_sources(FwVocalChainSnapshot PRIVATE ${CMAKE_SOURCE_DIR}/tools/Snapshot.cpp ${CH_SOURCES})
target_include_directories(FwVocalChainSnapshot PRIVATE ${CH_DIR}/Source)
target_compile_definitions(FwVocalChainSnapshot PRIVATE ${CH_APP_DEFS})
target_link_libraries(FwVocalChainSnapshot PRIVATE FwCommon FwVocalChainIR juce::juce_recommended_config_flags)

# Unit tests (DSP + processor):  cmake --build build --target FwVocalChainTests, then ctest --test-dir build -C Release
juce_add_console_app(FwVocalChainTests PRODUCT_NAME "FwVocalChainTests")
target_sources(FwVocalChainTests PRIVATE ${CH_DIR}/tests/FwVocalChainTests.cpp ${CH_SOURCES})
target_include_directories(FwVocalChainTests PRIVATE ${CH_DIR}/Source)
target_compile_definitions(FwVocalChainTests PRIVATE ${CH_APP_DEFS})
target_link_libraries(FwVocalChainTests PRIVATE FwCommon FwVocalChainIR juce::juce_recommended_config_flags)
add_test(NAME FwVocalChainTests COMMAND FwVocalChainTests)

# Offline renderer for the null tests and the render regression: the plug-in's processor on raw float files
# (tools/ChRender.cpp)
juce_add_console_app(ChRender PRODUCT_NAME "ChRender")
target_sources(ChRender PRIVATE ${CH_DIR}/tools/ChRender.cpp ${CH_SOURCES})
target_include_directories(ChRender PRIVATE ${CH_DIR}/Source)
target_compile_definitions(ChRender PRIVATE ${CH_APP_DEFS})
target_link_libraries(ChRender PRIVATE FwCommon FwVocalChainIR juce::juce_recommended_config_flags)

# What CI builds (the ci_targets aggregate of the root CMake file); the snapshot tool on Windows only.
add_dependencies(ci_targets FwVocalChain_VST3 FwVocalChainTests ChRender)
if(WIN32)
    add_dependencies(ci_targets FwVocalChainSnapshot)
endif()
