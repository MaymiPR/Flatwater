# Code every Flatwater plug-in uses: the toolbar, the editor looks (ren, flat, hybrid), the theme, the meters, the
# preset and settings helpers (shared/ui/, shared/Presets.h) and the unfused math (shared/dsp/Math.h). Included from
# the top-level CMakeLists.txt before the plug-ins. Built once as the static library FlatwaterShared, which every plug-in,
# test, snapshot and renderer with JUCE links; its include path is the repository root (#include "shared/ui/...").
add_library(FlatwaterShared STATIC
    ${CMAKE_CURRENT_LIST_DIR}/ui/Widgets.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/HostFocus.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/RenUI.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/FlatUI.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/HybridUI.cpp)
target_include_directories(FlatwaterShared PUBLIC ${CMAKE_SOURCE_DIR})
set_target_properties(FlatwaterShared PROPERTIES POSITION_INDEPENDENT_CODE ON)   # linked into shared plugins (Linux)
target_compile_definitions(FlatwaterShared PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(FlatwaterShared PUBLIC
    juce::juce_audio_processors
    juce::juce_gui_basics
    juce::juce_dsp
    PRIVATE juce::juce_recommended_config_flags)
