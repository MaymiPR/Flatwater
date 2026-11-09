cmake_minimum_required(VERSION 3.22)

project(Flatwater VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# The same floating-point results on every platform: GCC and Clang may fuse a multiply and an add (Apple's Clang does
# by default on arm64), MSVC does not; shared/dsp/Math.h relies on unfused arithmetic.
add_compile_options($<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-ffp-contract=off>)

# Windows: static C++ runtime, so the plugins load on any PC without a redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
# macOS universal builds: -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET=10.13

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 9.0.5
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)
# A tag can be moved: build only from the commit it pointed to when chosen
# (unless FETCHCONTENT_SOURCE_DIR_JUCE points at a JUCE of your own).
set(FW_JUCE_COMMIT d60b7a78030661574a615b586d9325ba7a7fa7dd)
if(NOT FETCHCONTENT_SOURCE_DIR_JUCE)
    find_package(Git REQUIRED)
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse HEAD WORKING_DIRECTORY ${juce_SOURCE_DIR}
        OUTPUT_VARIABLE juce_commit OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT juce_commit STREQUAL FW_JUCE_COMMIT)
        message(FATAL_ERROR "JUCE 9.0.5 is at ${juce_commit}, expected ${FW_JUCE_COMMIT}")
    endif()
endif()

# CLAP through clap-juce-extensions (MIT; brings the CLAP SDK).
option(FW_CLAP "Build CLAP plugins" ON)
if(FW_CLAP)
    FetchContent_Declare(clap_juce_extensions
        GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
        GIT_TAG 7adee3a1bd4684d4caa5601100e364abccff4b4d
        GIT_SUBMODULES_RECURSE ON)
    FetchContent_MakeAvailable(clap_juce_extensions)
endif()

# fw_add_plugin(<Target> <Product name> <4-char code> <clap id> [CATEGORIES <VST3 categories>] SOURCES ...)
function(fw_add_plugin target product code clapid)
    cmake_parse_arguments(P "" "" "SOURCES;CATEGORIES" ${ARGN})
    if(NOT P_CATEGORIES)
        set(P_CATEGORIES Fx Dynamics)
    endif()
    set(formats VST3 Standalone)   # Standalone: the house decision (an app per plug-in, to hear it without a DAW)
    if(APPLE)
        list(APPEND formats AU)
    endif()
    juce_add_plugin(${target}
        COMPANY_NAME "Flatwater"
        BUNDLE_ID com.flatwater.${target}   # (the default, from the company name, would hold a space)
        PLUGIN_MANUFACTURER_CODE Fltw
        PLUGIN_CODE ${code}
        FORMATS ${formats}
        PRODUCT_NAME "${product}"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE   # the host keeps its transport shortcuts
        VST3_CATEGORIES ${P_CATEGORIES}
        AU_MAIN_TYPE kAudioUnitType_Effect)
    target_sources(${target} PRIVATE ${P_SOURCES})
    target_link_libraries(${target} PRIVATE FwCommon juce::juce_audio_utils juce::juce_recommended_warning_flags)   # audio_utils: the Standalone wrapper
    # (GCC: the VST3 SDK inside JUCE writes four-character codes as character constants)
    foreach(t ${target} ${target}_VST3)
        target_compile_options(${t} PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wno-multichar>)
    endforeach()
    if(FW_CLAP)
        clap_juce_extensions_plugin(TARGET ${target}
            CLAP_ID "${clapid}" CLAP_FEATURES audio-effect)
        # (Xcode: the CLAP bundle's Info.plist identifier, also as the build setting)
        set_target_properties(${target}_CLAP PROPERTIES XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${clapid}")
    endif()
    # Rebuilds land in build/plugins (an FL Studio VST3 scan folder points here).
    add_custom_command(TARGET ${target}_VST3 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "$<TARGET_FILE_DIR:${target}_VST3>/../.."
            "${CMAKE_BINARY_DIR}/plugins/${product}.vst3")
endfunction()

# fw_tool_output(<Target>): a plain executable (a renderer without JUCE) lands where JUCE puts its console apps,
# build/<Target>_artefacts/<Config>, so every renderer is found the same way.
function(fw_tool_output target)
    set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${target}_artefacts/$<CONFIG>")
endfunction()

enable_testing()
# ci_targets: what CI builds on every system (ci.yml). Each plug-in adds its VST3, its unit tests and its renderer,
# and on Windows its snapshot tool, so the workflow builds one target and a new plug-in needs no workflow edit.
add_custom_target(ci_targets)
# The plug-ins. Each is included (not added as a subdirectory), so its build output stays at
# build/<Target>_artefacts. packaging/plugins.txt lists them for the installers and the release workflow.
include(shared/shared.cmake)
include(plugins/FwVocalComp/plugin.cmake)
include(plugins/FwBassLift/plugin.cmake)
include(plugins/FwBassXLift/plugin.cmake)
include(plugins/FwComp/plugin.cmake)
include(plugins/FwDelay/plugin.cmake)
include(plugins/FwBassLeveler/plugin.cmake)
include(plugins/FwVocalLeveler/plugin.cmake)
include(plugins/FwVocalChain/plugin.cmake)
include(plugins/FwPitchBend/plugin.cmake)

# Windows-only check that the plugins leave the keyboard to the host (tools/FocusCheck.cpp)
if(WIN32)
    juce_add_console_app(FocusCheck PRODUCT_NAME "FocusCheck")
    target_sources(FocusCheck PRIVATE tools/FocusCheck.cpp)
    target_compile_definitions(FocusCheck PRIVATE JUCE_PLUGINHOST_VST3=1 JUCE_MODAL_LOOPS_PERMITTED=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(FocusCheck PRIVATE juce::juce_audio_processors juce::juce_gui_basics juce::juce_recommended_config_flags)

    # records a plug-in editor's meters while audio plays (meter ballistics of the references; tools/MeterCapture.cpp)
    juce_add_gui_app(MeterCapture PRODUCT_NAME "MeterCapture")
    target_sources(MeterCapture PRIVATE tools/MeterCapture.cpp)
    target_compile_definitions(MeterCapture PRIVATE JUCE_PLUGINHOST_VST3=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(MeterCapture PRIVATE juce::juce_audio_processors juce::juce_gui_basics juce::juce_recommended_config_flags)

    # renders a reference offline with its side-chain bus fed and a host tempo (tools/RefRender.cpp)
    juce_add_console_app(RefRender PRODUCT_NAME "RefRender")
    target_sources(RefRender PRIVATE tools/RefRender.cpp)
    target_compile_definitions(RefRender PRIVATE JUCE_PLUGINHOST_VST3=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(RefRender PRIVATE juce::juce_audio_processors juce::juce_gui_basics juce::juce_recommended_config_flags)

    # decodes Ogg Vorbis to raw float32, for test material from Ogg-coded samples (tools/OggDecode.cpp)
    juce_add_console_app(OggDecode PRODUCT_NAME "OggDecode")
    target_sources(OggDecode PRIVATE tools/OggDecode.cpp)
    target_compile_definitions(OggDecode PRIVATE JUCE_USE_OGGVORBIS=1)
    target_link_libraries(OggDecode PRIVATE juce::juce_audio_formats juce::juce_recommended_config_flags)
endif()

# GCC 13 reads a reference into a juce::Span view in JUCE 9.0.5's own code (LookAndFeel_V2's alert window buttons) as
# dangling: a false alarm (GCC 14 knows span-like views; GCC before 13 has no such warning). Silenced for JUCE's
# module files only (every target is defined in this directory: the plug-ins are included), so Flatwater's own code
# keeps the warning.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 13)
    file(GLOB fw_juce_module_sources ${juce_SOURCE_DIR}/modules/*/juce_*.cpp)
    set_source_files_properties(${fw_juce_module_sources} DIRECTORY ${CMAKE_SOURCE_DIR}
        PROPERTIES COMPILE_OPTIONS -Wno-dangling-reference)
endif()
