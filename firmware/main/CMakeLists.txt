set(APP_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")
idf_component_register(
 SRCS main.cpp Board.cpp FlashStorage.cpp Wifi.cpp Bluetooth.cpp FirmwareUpdate.cpp
  "${APP_ROOT}/src/core/GameState.cpp"
  "${APP_ROOT}/src/core/RandomTools.cpp"
  "${APP_ROOT}/src/core/DeviceProfile.cpp"
  "${APP_ROOT}/src/core/Multiplayer.cpp"
  "${APP_ROOT}/src/ui/AppUI.cpp"
  "${APP_ROOT}/src/ui/AppUIDial.cpp"
  "${APP_ROOT}/src/ui/FaceDesigns.cpp"
  "${APP_ROOT}/src/ui/FaceLayouts.cpp"
  "${APP_ROOT}/src/ui/AppUITools.cpp"
  "${APP_ROOT}/src/ui/AppUIInformation.cpp"
  "${APP_ROOT}/src/ui/AppUIMultiplayer.cpp"
  "${APP_ROOT}/src/ui/Widgets/ToolIcons.cpp"
  "${APP_ROOT}/src/assets/life_digits.c"
  "${APP_ROOT}/src/assets/mtg_symbols.c"
  "${APP_ROOT}/src/assets/mana_pips.c"
  "${APP_ROOT}/src/assets/mana_life_digits.c"
  "${APP_ROOT}/src/assets/heart_symbol.c"
  "${APP_ROOT}/src/assets/tab_font.c"
  "${APP_ROOT}/simulator/LanTransport.cpp"
 INCLUDE_DIRS "." "${APP_ROOT}/src" "${APP_ROOT}/simulator"
 REQUIRES driver esp_lcd esp_timer esp_wifi esp_netif nvs_flash spiffs esp_psram esp_pm esp_http_client esp_http_server app_update mbedtls bt)
include("${APP_ROOT}/cmake/WatchfaceArt.cmake")
arcana_watchface_art(ARCANA_WATCHFACE_ART)
target_sources(${COMPONENT_LIB} PRIVATE "${ARCANA_WATCHFACE_ART}")
target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=missing-field-initializers -Wno-error=misleading-indentation)
# Give the LVGL component the embedded configuration, independently of desktop LVGL.
idf_component_get_property(LVGL_LIB lvgl__lvgl COMPONENT_LIB)
# Component Manager can materialize cached directories as Windows junctions.
# LVGL 9.2.2's own glob does not follow them and can silently omit core sources.
file(GLOB_RECURSE LVGL_COMPLETE_SOURCES FOLLOW_SYMLINKS
 "${APP_ROOT}/firmware/managed_components/lvgl__lvgl/src/*.c"
 "${APP_ROOT}/firmware/managed_components/lvgl__lvgl/src/*.cpp")
target_sources(${LVGL_LIB} PRIVATE ${LVGL_COMPLETE_SOURCES})
target_include_directories(${LVGL_LIB} PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
target_compile_definitions(${LVGL_LIB} PUBLIC LV_CONF_INCLUDE_SIMPLE LV_KCONFIG_IGNORE)

