cmake_minimum_required(VERSION 3.22)
project(Psaltikon VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(JUCE_PATH "" CACHE PATH "Path to the JUCE source folder")
if(JUCE_PATH)
    add_subdirectory(${JUCE_PATH} JUCE EXCLUDE_FROM_ALL)
else()
    find_package(JUCE CONFIG REQUIRED)
endif()

option(PSALTIKON_COPY_AFTER_BUILD "Install the plug-ins into the system plug-in folders after building" ON)

# AU and VST3 everywhere they can be built (AU is macOS only), plus a standalone app.
set(PSALTIKON_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND PSALTIKON_FORMATS AU)
endif()

juce_add_plugin(Psaltikon
    COMPANY_NAME "Psaltikon"
    BUNDLE_ID com.psaltikon.psaltikon
    PLUGIN_MANUFACTURER_CODE Pslt
    PLUGIN_CODE Psk1
    FORMATS ${PSALTIKON_FORMATS}
    PRODUCT_NAME "Psaltikon"
    DESCRIPTION "Byzantine chant workspace: notation, modes, ison and chanting voice"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    VST3_CATEGORIES Instrument Synth
    COPY_PLUGIN_AFTER_BUILD ${PSALTIKON_COPY_AFTER_BUILD}
)

juce_generate_juce_header(Psaltikon)

target_sources(Psaltikon PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_compile_definitions(Psaltikon PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

if(MSVC)
    target_compile_options(Psaltikon PRIVATE /utf-8)
endif()

if(UNIX AND NOT APPLE)
    # Linux builds are for compile checks; no audio device back-ends needed.
    target_compile_definitions(Psaltikon PUBLIC JUCE_ALSA=0 JUCE_JACK=0)
endif()

target_link_libraries(Psaltikon PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags
)

option(PSALTIKON_TESTS "Build the offline render test" OFF)
if(PSALTIKON_TESTS)
    juce_add_console_app(PsaltikonTests PRODUCT_NAME "PsaltikonTests")
    target_sources(PsaltikonTests PRIVATE Tests/RenderTest.cpp)
    target_include_directories(PsaltikonTests PRIVATE Source)
    juce_generate_juce_header(PsaltikonTests)
    target_compile_definitions(PsaltikonTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    if(UNIX AND NOT APPLE)
        target_compile_definitions(PsaltikonTests PRIVATE JUCE_ALSA=0 JUCE_JACK=0)
    endif()
    target_link_libraries(PsaltikonTests PRIVATE juce::juce_audio_utils juce::juce_dsp
        PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
endif()
