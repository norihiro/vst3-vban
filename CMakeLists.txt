cmake_minimum_required(VERSION 3.14.0)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.12 CACHE STRING "")

option(vst3sdk_SOURCE_DIR "Path to vst3sdk")
if(NOT vst3sdk_SOURCE_DIR)
    message(FATAL_ERROR "Path to VST3 SDK is empty!")
endif()

project(VBANPlugin
    # This is your plug-in version number. Change it here only.
    # Version number symbols usable in C++ can be found in
    # source/version.h and ${PROJECT_BINARY_DIR}/projectversion.h.
    VERSION 1.0.0.0 
    DESCRIPTION "VBANPlugin VST 3 Plug-in"
)

set(SMTG_VSTGUI_ROOT "${vst3sdk_SOURCE_DIR}")

set(SMTG_CXX_STANDARD "17")

option(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES "" OFF)
option(SMTG_ENABLE_VSTGUI_SUPPORT "" OFF)
add_subdirectory(${vst3sdk_SOURCE_DIR} ${PROJECT_BINARY_DIR}/vst3sdk)
smtg_enable_vst3_sdk()

smtg_add_vst3plugin(VBANPlugin
    source/version.h
    source/vban_cids.h
    source/vban_processor.h
    source/vban_processor.cpp
    source/vban_processor_thread.cc
    source/vban_controller.h
    source/vban_controller.cpp
    source/vban_entry.cpp
    source/audio_buffer.cc
)

target_include_directories(VBANPlugin
    PRIVATE deps/vban
)

#- VSTGUI Wanted ----
if(SMTG_ENABLE_VSTGUI_SUPPORT)
    target_sources(VBANPlugin
        PRIVATE
            resource/vban_editor.uidesc
    )
    target_link_libraries(VBANPlugin
        PRIVATE
            vstgui_support
    )
    smtg_target_add_plugin_resources(VBANPlugin
        RESOURCES
            "resource/vban_editor.uidesc"
    )
endif(SMTG_ENABLE_VSTGUI_SUPPORT)
# -------------------

smtg_target_add_plugin_snapshots (VBANPlugin
    RESOURCES
        resource/14B7F584641B57E085DC53914E5369FA_snapshot.png
        resource/14B7F584641B57E085DC53914E5369FA_snapshot_2.0x.png
)

target_link_libraries(VBANPlugin
    PRIVATE
        sdk
)

smtg_target_configure_version_file(VBANPlugin)

if(SMTG_MAC)
    smtg_target_set_bundle(VBANPlugin
        BUNDLE_IDENTIFIER net.nagater.vst3.vban
        COMPANY_NAME "Nagater Networks"
    )
    smtg_target_set_debug_executable(VBANPlugin
        "/Applications/VST3PluginTestHost.app"
        "--pluginfolder;$(BUILT_PRODUCTS_DIR)"
    )
elseif(SMTG_WIN)
    target_sources(VBANPlugin PRIVATE 
        resource/win32resource.rc
    )
    if(MSVC)
        set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT VBANPlugin)

        smtg_target_set_debug_executable(VBANPlugin
            "$(ProgramW6432)/Steinberg/VST3PluginTestHost/VST3PluginTestHost.exe"
            "--pluginfolder \"$(OutDir)/\""
        )
    endif()
endif(SMTG_MAC)

file(GENERATE OUTPUT .gitignore CONTENT "*\n")
