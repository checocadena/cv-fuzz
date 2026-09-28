cmake_minimum_required(VERSION 3.22)
project(CVFuzz VERSION 1.0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Universal binary on macOS so it loads natively on Apple Silicon and Intel.
if(APPLE AND NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
endif()
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

if(APPLE)
    set(CVFUZZ_FORMATS AU VST3 Standalone)
else()
    set(CVFUZZ_FORMATS VST3 Standalone)
endif()

juce_add_plugin(CVFuzz
    COMPANY_NAME "Checo Cadena"
    BUNDLE_ID com.checocadena.cvfuzz
    PRODUCT_NAME "CV Fuzz"
    PLUGIN_MANUFACTURER_CODE Chca
    PLUGIN_CODE Cvf1
    AU_MAIN_TYPE kAudioUnitType_Effect
    FORMATS ${CVFUZZ_FORMATS}
    VST3_CATEGORIES Fx Distortion
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "CV Fuzz processes audio from your interface when running as a standalone app."
    NEEDS_WEBVIEW2 TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE)

juce_generate_juce_header(CVFuzz)

# The interface, plus JUCE's JavaScript bridge, compiled into the plugin.
set(JUCE_JS_DIR ${JUCE_SOURCE_DIR}/modules/juce_gui_extra/native/javascript)
juce_add_binary_data(CVFuzzUI SOURCES
    Source/ui/index.html
    Source/assets/Checo_Cadena_Resume.pdf
    ${JUCE_JS_DIR}/index.js
    ${JUCE_JS_DIR}/check_native_interop.js)

target_sources(CVFuzz PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(CVFuzz PUBLIC
    JUCE_WEB_BROWSER=1
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1)

target_link_libraries(CVFuzz
    PRIVATE
        CVFuzzUI
        juce::juce_audio_utils
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Linux only: the embedded web view needs GTK and WebKitGTK. macOS uses the system WebKit.
if(UNIX AND NOT APPLE)
    target_link_libraries(CVFuzz PRIVATE juce::pkgconfig_JUCE_BROWSER_LINUX_DEPS)
endif()
