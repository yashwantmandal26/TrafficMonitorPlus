cmake_minimum_required(VERSION 3.20)
project(TrafficMonitorPlus VERSION 2.0.0 LANGUAGES CXX RC)

if(NOT WIN32 OR NOT MSVC)
    message(FATAL_ERROR "Build with MSVC on Windows (Visual Studio C++ desktop workload).")
endif()
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8 OR CMAKE_GENERATOR_PLATFORM MATCHES "ARM")
    message(FATAL_ERROR "The supplied sensor library requires an x64 build. Configure with -A x64.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Collect all sources and headers
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "src/*.cpp"
)
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS
    "src/*.h"
)

set(RESOURCES
    "res/app.rc"
)

# Executable target (Windows GUI application)
add_executable(${PROJECT_NAME} WIN32
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/res
)

link_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

# Link Windows libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    d2d1
    dwrite
    dwmapi
    iphlpapi
    ws2_32
    pdh
    powrprof
    shell32
    shlwapi
    comdlg32
    comctl32
    ole32
    oleaut32
    advapi32
    user32
    gdi32
    setupapi
    delayimp
    winhttp
    taskschd
    windowsapp
    uxtheme
    version
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/OpenHardwareMonitorApi.lib"
)

# MSVC-specific compile options
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W3
        /utf-8
        /permissive-
        /Zc:__cplusplus
        /EHsc
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        UNICODE
        _UNICODE
        _CRT_SECURE_NO_WARNINGS
    )
    
    # Release optimizations
    set_property(TARGET ${PROJECT_NAME} PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
    target_compile_options(${PROJECT_NAME} PRIVATE /GS /sdl /guard:cf)
    target_link_options(${PROJECT_NAME} PRIVATE /DYNAMICBASE /NXCOMPAT /HIGHENTROPYVA /guard:cf)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
    if(ipo_supported)
        set_property(TARGET ${PROJECT_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    endif()
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE TRUE
    OUTPUT_NAME "TrafficMonitorPlus"
    LINK_FLAGS "/MANIFEST:NO /DELAYLOAD:OpenHardwareMonitorApi.dll"
)

foreach(runtime OpenHardwareMonitorApi.dll LibreHardwareMonitorLib.dll HidSharp.dll)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/${runtime}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
endforeach()
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/res/TrafficMonitorPlus.exe.config" "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
install(FILES res/TrafficMonitorPlus.exe.config DESTINATION .)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION .)
install(FILES OpenHardwareMonitorApi.dll LibreHardwareMonitorLib.dll HidSharp.dll README.md THIRD_PARTY_NOTICES.md DESTINATION .)
install(DIRECTORY licenses/ DESTINATION licenses)
install(FILES install.bat uninstall.bat RELEASE_VALIDATION.md DESTINATION .)
install(FILES scripts/install.ps1 DESTINATION scripts)
install(FILES scripts/sensor-diagnostics.ps1 DESTINATION scripts)
install(FILES scripts/uninstall.ps1 DESTINATION scripts)
include(CTest)
if(BUILD_TESTING)
    add_executable(live_network_tests tests/live_network_tests.cpp src/telemetry/live_network_monitor.cpp src/utils/string_utils.cpp)
    target_include_directories(live_network_tests PRIVATE src lib)
    target_compile_definitions(live_network_tests PRIVATE UNICODE _UNICODE)
    target_compile_options(live_network_tests PRIVATE /utf-8)
    set_property(TARGET live_network_tests PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_link_libraries(live_network_tests PRIVATE advapi32 ole32 user32 ws2_32 iphlpapi)
    add_test(NAME live_network_decoding_rates_idle_rollover_and_history COMMAND live_network_tests)
    add_executable(telemetry_accuracy_tests tests/telemetry_accuracy_tests.cpp src/telemetry/network_monitor.cpp src/telemetry/memory_monitor.cpp src/utils/string_utils.cpp)
    target_include_directories(telemetry_accuracy_tests PRIVATE src lib)
    target_compile_definitions(telemetry_accuracy_tests PRIVATE UNICODE _UNICODE)
    target_compile_options(telemetry_accuracy_tests PRIVATE /utf-8)
    set_property(TARGET telemetry_accuracy_tests PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_link_libraries(telemetry_accuracy_tests PRIVATE user32 iphlpapi ws2_32 ole32)
    add_test(NAME telemetry_exact_values_missing_data_and_deduplication COMMAND telemetry_accuracy_tests)
    foreach(dashboard_target dashboard_preview dashboard_tests)
        if(dashboard_target STREQUAL "dashboard_preview")
            add_executable(${dashboard_target} WIN32 EXCLUDE_FROM_ALL tests/${dashboard_target}.cpp)
        else()
            add_executable(${dashboard_target} tests/${dashboard_target}.cpp)
        endif()
        target_sources(${dashboard_target} PRIVATE src/windows/flyout_window.cpp src/rendering/d2d_renderer.cpp src/rendering/dpi_helper.cpp src/rendering/theme.cpp src/rendering/sparkline.cpp src/utils/string_utils.cpp)
        target_include_directories(${dashboard_target} PRIVATE src)
        target_compile_options(${dashboard_target} PRIVATE /utf-8)
        target_compile_definitions(${dashboard_target} PRIVATE UNICODE _UNICODE)
        set_property(TARGET ${dashboard_target} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        target_link_libraries(${dashboard_target} PRIVATE user32 gdi32 d2d1 dwrite dwmapi ole32 ws2_32 iphlpapi)
    endforeach()
    add_test(NAME dashboard_navigation_pin_actions_and_storage COMMAND dashboard_tests)
    add_executable(menu_tests tests/menu_tests.cpp src/windows/tray_icon.cpp)
    target_include_directories(menu_tests PRIVATE src)
    target_compile_options(menu_tests PRIVATE /utf-8)
    target_compile_definitions(menu_tests PRIVATE UNICODE _UNICODE)
    set_property(TARGET menu_tests PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_link_libraries(menu_tests PRIVATE user32 shell32)
    add_test(NAME essential_context_menu COMMAND menu_tests)
    add_executable(usage_tooltip_tests tests/usage_tooltip_tests.cpp src/windows/taskbar_host.cpp res/app.rc)
    target_include_directories(usage_tooltip_tests PRIVATE src res)
    target_link_options(usage_tooltip_tests PRIVATE /MANIFEST:NO)
    target_compile_options(usage_tooltip_tests PRIVATE /utf-8)
    target_compile_definitions(usage_tooltip_tests PRIVATE UNICODE _UNICODE)
    set_property(TARGET usage_tooltip_tests PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_link_libraries(usage_tooltip_tests PRIVATE user32 gdi32 comctl32 uxtheme)
    add_test(NAME daily_usage_hover_tooltip COMMAND usage_tooltip_tests)
    add_executable(taskbar_interaction_tests tests/taskbar_interaction_tests.cpp
        src/windows/taskbar_host.cpp src/windows/flyout_window.cpp src/rendering/d2d_renderer.cpp
        src/rendering/dpi_helper.cpp src/rendering/theme.cpp src/rendering/sparkline.cpp
        src/utils/string_utils.cpp res/app.rc)
    target_include_directories(taskbar_interaction_tests PRIVATE src res)
    target_link_options(taskbar_interaction_tests PRIVATE /MANIFEST:NO)
    target_compile_options(taskbar_interaction_tests PRIVATE /utf-8)
    target_compile_definitions(taskbar_interaction_tests PRIVATE UNICODE _UNICODE)
    set_property(TARGET taskbar_interaction_tests PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_link_libraries(taskbar_interaction_tests PRIVATE user32 gdi32 comctl32 uxtheme d2d1 dwrite dwmapi ole32 ws2_32 iphlpapi)
    add_test(NAME taskbar_whitespace_clicks_open_dashboard COMMAND taskbar_interaction_tests)
    add_executable(stable_layout_tests tests/stable_layout_tests.cpp src/config/settings.cpp src/rendering/d2d_renderer.cpp src/rendering/text_renderer.cpp src/rendering/theme.cpp src/utils/string_utils.cpp)
    target_include_directories(stable_layout_tests PRIVATE src lib)
    target_compile_options(stable_layout_tests PRIVATE /utf-8)
    target_compile_definitions(stable_layout_tests PRIVATE UNICODE _UNICODE)
    set_property(TARGET stable_layout_tests PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_link_libraries(stable_layout_tests PRIVATE d2d1 dwrite ole32 user32 ws2_32 iphlpapi)
    add_test(NAME stable_positions_white_labels_dynamic_values COMMAND stable_layout_tests)
    add_executable(settings_preview WIN32 EXCLUDE_FROM_ALL tests/settings_preview.cpp src/windows/settings_window.cpp src/config/settings.cpp src/rendering/d2d_renderer.cpp src/rendering/text_renderer.cpp src/rendering/theme.cpp src/utils/string_utils.cpp)
    target_include_directories(settings_preview PRIVATE src lib)
    target_compile_options(settings_preview PRIVATE /utf-8)
    target_compile_definitions(settings_preview PRIVATE UNICODE _UNICODE)
    set_property(TARGET settings_preview PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_link_libraries(settings_preview PRIVATE user32 gdi32 ws2_32 iphlpapi d2d1 dwrite dwmapi comdlg32 comctl32 ole32 uxtheme)
    add_executable(settings_window_tests tests/settings_window_tests.cpp src/windows/settings_window.cpp src/config/settings.cpp src/rendering/d2d_renderer.cpp src/rendering/text_renderer.cpp src/rendering/theme.cpp src/utils/string_utils.cpp)
    target_include_directories(settings_window_tests PRIVATE src lib)
    target_compile_options(settings_window_tests PRIVATE /utf-8)
    target_compile_definitions(settings_window_tests PRIVATE UNICODE _UNICODE)
    set_property(TARGET settings_window_tests PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_link_libraries(settings_window_tests PRIVATE user32 gdi32 ws2_32 iphlpapi d2d1 dwrite dwmapi comdlg32 comctl32 ole32 uxtheme)
    add_test(NAME appearance_sliders_apply_without_closing COMMAND settings_window_tests)
    add_executable(usage_history_tests tests/usage_history_tests.cpp src/telemetry/usage_history.cpp)
    target_include_directories(usage_history_tests PRIVATE src lib)
    target_compile_options(usage_history_tests PRIVATE /utf-8)
    target_compile_definitions(usage_history_tests PRIVATE UNICODE _UNICODE)
    set_property(TARGET usage_history_tests PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_test(NAME daily_monthly_usage_persistence COMMAND usage_history_tests)
    add_executable(gpu_usage_tests tests/gpu_usage_tests.cpp)
    target_include_directories(gpu_usage_tests PRIVATE src)
    set_property(TARGET gpu_usage_tests PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_test(NAME gpu_physical_engine_aggregation COMMAND gpu_usage_tests)
    add_executable(settings_tests tests/settings_tests.cpp src/config/settings.cpp src/utils/string_utils.cpp)
    target_include_directories(settings_tests PRIVATE src lib)
    target_compile_definitions(settings_tests PRIVATE UNICODE _UNICODE)
    target_compile_options(settings_tests PRIVATE /utf-8)
    set_property(TARGET settings_tests PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_link_libraries(settings_tests PRIVATE user32 ws2_32 iphlpapi)
    add_test(NAME settings_roundtrip_and_recovery COMMAND settings_tests)
    add_executable(network_usage_tests tests/network_usage_tests.cpp src/telemetry/windows_network_usage.cpp src/utils/string_utils.cpp)
    target_include_directories(network_usage_tests PRIVATE src lib)
    target_compile_definitions(network_usage_tests PRIVATE UNICODE _UNICODE)
    target_compile_options(network_usage_tests PRIVATE /utf-8)
    set_property(TARGET network_usage_tests PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_link_libraries(network_usage_tests PRIVATE user32 ws2_32 iphlpapi advapi32 version windowsapp)
    add_test(NAME windows_app_service_usage_tracking COMMAND network_usage_tests)

    # Phase 1: Safety, Ownership and Startup Correctness Regressions
    add_executable(autostart_tests tests/autostart_tests.cpp src/config/autostart.cpp)
    target_include_directories(autostart_tests PRIVATE src lib)
    target_compile_definitions(autostart_tests PRIVATE UNICODE _UNICODE)
    target_compile_options(autostart_tests PRIVATE /utf-8)
    set_property(TARGET autostart_tests PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_link_libraries(autostart_tests PRIVATE ole32 oleaut32 advapi32 taskschd)
    add_test(NAME autostart_ownership_and_validation COMMAND autostart_tests)

    add_executable(process_sort_tests tests/process_sort_tests.cpp src/telemetry/process_monitor.cpp)
    target_include_directories(process_sort_tests PRIVATE src lib)
    target_compile_definitions(process_sort_tests PRIVATE UNICODE _UNICODE)
    target_compile_options(process_sort_tests PRIVATE /utf-8)
    set_property(TARGET process_sort_tests PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_test(NAME process_strict_weak_ordering COMMAND process_sort_tests)

    add_executable(render_failure_tests tests/render_failure_tests.cpp src/rendering/d2d_renderer.cpp src/rendering/dpi_helper.cpp res/app.rc)
    target_link_options(render_failure_tests PRIVATE /MANIFEST:NO)
    target_include_directories(render_failure_tests PRIVATE src lib)
    target_compile_definitions(render_failure_tests PRIVATE UNICODE _UNICODE)
    target_compile_options(render_failure_tests PRIVATE /utf-8)
    set_property(TARGET render_failure_tests PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_link_libraries(render_failure_tests PRIVATE d2d1 dwrite user32 gdi32 ole32 dwmapi)
    add_test(NAME render_target_failure_and_recovery COMMAND render_failure_tests)

    add_executable(usage_worker_tests tests/usage_worker_tests.cpp)
    target_include_directories(usage_worker_tests PRIVATE src lib res)
    target_compile_definitions(usage_worker_tests PRIVATE UNICODE _UNICODE)
    target_compile_options(usage_worker_tests PRIVATE /utf-8)
    set_property(TARGET usage_worker_tests PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_link_libraries(usage_worker_tests PRIVATE user32)
    add_test(NAME usage_worker_lifetime_and_snapshots COMMAND usage_worker_tests)

    add_executable(cli_startup_tests tests/cli_startup_tests.cpp src/config/autostart.cpp)
    target_include_directories(cli_startup_tests PRIVATE src lib)
    target_compile_definitions(cli_startup_tests PRIVATE UNICODE _UNICODE)
    target_compile_options(cli_startup_tests PRIVATE /utf-8)
    set_property(TARGET cli_startup_tests PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_link_libraries(cli_startup_tests PRIVATE ole32 oleaut32 advapi32 taskschd user32)
    add_test(NAME cli_startup_dispatch_and_scoping COMMAND cli_startup_tests)

    # Phase 2: Telemetry, History & Exports Regressions
    add_executable(telemetry_history_tests tests/telemetry_history_tests.cpp src/telemetry/windows_network_usage.cpp src/telemetry/network_monitor.cpp src/utils/string_utils.cpp)
    target_include_directories(telemetry_history_tests PRIVATE src lib)
    target_compile_definitions(telemetry_history_tests PRIVATE UNICODE _UNICODE)
    target_compile_options(telemetry_history_tests PRIVATE /utf-8)
    set_property(TARGET telemetry_history_tests PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_link_libraries(telemetry_history_tests PRIVATE user32 ws2_32 iphlpapi advapi32 version windowsapp ole32)
    add_test(NAME telemetry_history_and_exports COMMAND telemetry_history_tests)

    # Phase 3: UX, Visual Polish & Packaging Regressions
    add_executable(ux_polish_tests
        tests/ux_polish_tests.cpp
        src/config/settings.cpp
        src/config/updater.cpp
        src/rendering/d2d_renderer.cpp
        src/rendering/sparkline.cpp
        src/rendering/text_renderer.cpp
        src/rendering/theme.cpp
        src/rendering/dpi_helper.cpp
        src/windows/settings_window.cpp
        src/windows/flyout_window.cpp
        src/windows/tray_icon.cpp
        src/utils/string_utils.cpp
        src/telemetry/process_monitor.cpp
        src/telemetry/usage_history.cpp
        src/telemetry/windows_network_usage.cpp
    )
    target_include_directories(ux_polish_tests PRIVATE src lib res)
    target_compile_definitions(ux_polish_tests PRIVATE UNICODE _UNICODE)
    target_compile_options(ux_polish_tests PRIVATE /utf-8)
    set_property(TARGET ux_polish_tests PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_link_libraries(ux_polish_tests PRIVATE
        d2d1 dwrite dwmapi winhttp user32 gdi32 ole32 oleaut32
        advapi32 comctl32 comdlg32 shell32 iphlpapi ws2_32 version windowsapp uxtheme
    )
    add_test(NAME ux_polish_and_packaging COMMAND ux_polish_tests)
    # Release tests must execute assertions as well as Debug tests.
    get_property(test_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
    foreach(test_target IN LISTS test_targets)
        if(test_target MATCHES "_tests$")
            target_compile_options(${test_target} PRIVATE /UNDEBUG)
        endif()
    endforeach()
endif()
set(CPACK_GENERATOR ZIP)
set(CPACK_PACKAGE_FILE_NAME "TrafficMonitorPlus-${PROJECT_VERSION}-windows-x64")
if(BUILD_TESTING)
    # These checks need an interactive Explorer/compositor session, unlike hosted CI.
    set_tests_properties(dashboard_navigation_pin_actions_and_storage taskbar_whitespace_clicks_open_dashboard daily_usage_hover_tooltip appearance_sliders_apply_without_closing render_target_failure_and_recovery ux_polish_and_packaging PROPERTIES LABELS desktop)
endif()
install(FILES LICENSE DESTINATION .)
include(CPack)



