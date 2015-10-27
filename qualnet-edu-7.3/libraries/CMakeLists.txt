add_subdirectory(developer)
add_subdirectory(multimedia_enterprise)
add_subdirectory(wireless)
foreach (optional_lib
    advanced_wireless
    cellular
    lte
    military
    sensor_networks
    tirem
    umts
    urban
    user_models)
  if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${optional_lib})
    add_subdirectory(${optional_lib})
  endif ()
endforeach ()
if (IS_EXATA AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cyber)
  add_subdirectory(cyber)
endif ()
