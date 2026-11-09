# Code every Flatwater plug-in uses: the toolbar, the editor looks (ren, flat, hybrid), the theme, the meters, the
# preset and settings helpers (shared/ui/, shared/Presets.h) and the unfused math (shared/dsp/Math.h). Included from
# the top-level CMakeLists.txt before the plug-ins. Built once as the static library FwCommon, which every plug-in,
# test, snapshot and renderer with JUCE links; its include path is the repository root (#include "shared/ui/...").
add_library(FwCommon STATIC
    ${CMAKE_CURRENT_LIST_DIR}/ui/Toolbar.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/RenUI.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/FlatUI.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/HybridUI.cpp)
target_include_directories(FwCommon PUBLIC ${CMAKE_SOURCE_DIR})
set_target_properties(FwCommon PROPERTIES POSITION_INDEPENDENT_CODE ON)   # linked into shared plugins (Linux)
target_compile_definitions(FwCommon PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(FwCommon PUBLIC
    juce::juce_audio_processors
    juce::juce_gui_basics
    juce::juce_dsp
    PRIVATE juce::juce_recommended_config_flags)
