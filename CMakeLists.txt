cmake_minimum_required(VERSION 3.22)

project(NeoDelay VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.6
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(NeoDelay
    COMPANY_NAME "Andreas Neophitou"
    BUNDLE_ID com.aneophitou.neodelay
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE AnNp
    PLUGIN_CODE NeoD
    FORMATS VST3 LV2 Standalone
    PRODUCT_NAME "NeoDelay"
    LV2URI "https://aneophitou.audio/plugins/neodelay"
)

juce_generate_juce_header(NeoDelay)

target_sources(NeoDelay
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/dsp/ChorusEngine.cpp
        Source/dsp/DelayEngine.cpp
        Source/dsp/DemoToneGenerator.cpp
        Source/dsp/ShimmerEngine.cpp
)

target_compile_definitions(NeoDelay
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(NeoDelay
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

set(DIST_DIR "${CMAKE_SOURCE_DIR}/Dist")
add_custom_target(copy_dist ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_DIR}"
    COMMENT "Preparing Dist/ folder"
)

add_dependencies(copy_dist NeoDelay_VST3 NeoDelay_LV2)

add_custom_command(TARGET NeoDelay_VST3 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${DIST_DIR}/VST3"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_DIR}/VST3"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "$<TARGET_FILE_DIR:NeoDelay_VST3>/../.."
        "${DIST_DIR}/VST3/NeoDelay.vst3"
    COMMENT "Copying VST3 into Dist/"
)

add_custom_command(TARGET NeoDelay_LV2 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${DIST_DIR}/LV2"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_DIR}/LV2"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "$<TARGET_FILE_DIR:NeoDelay_LV2>"
        "${DIST_DIR}/LV2/NeoDelay.lv2"
    COMMENT "Copying LV2 into Dist/"
)

juce_add_console_app(NeoDelayRenderSamples
    PRODUCT_NAME "NeoDelayRenderSamples"
)

juce_generate_juce_header(NeoDelayRenderSamples)

target_sources(NeoDelayRenderSamples
    PRIVATE
        tools/RenderSamples.cpp
        Source/dsp/ChorusEngine.cpp
        Source/dsp/DelayEngine.cpp
        Source/dsp/ShimmerEngine.cpp
)

target_compile_definitions(NeoDelayRenderSamples
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(NeoDelayRenderSamples
    PRIVATE
        juce::juce_audio_formats
        juce::juce_audio_basics
        juce::juce_dsp
        juce::juce_events
        juce::juce_core
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_data_structures
    PUBLIC
        juce::juce_recommended_config_flags
)
