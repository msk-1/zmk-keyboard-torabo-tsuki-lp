if(CONFIG_SHIELD_TORABO_TSUKI_LP_LEFT OR CONFIG_SHIELD_TORABO_TSUKI_LP_RIGHT)
  zephyr_library()
  zephyr_include_directories(${APPLICATION_SOURCE_DIR}/include)
  zephyr_library_sources(src/board.c src/mini_trackpad_init_reg.c src/behavior_battery_led.c)

  # hid_listenerより先に呼ばせるため、zephyr_libraryではなくappに追加する
  if((NOT CONFIG_ZMK_SPLIT) OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    target_sources(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/implicit_mods_listener.c)
  endif()
endif()
