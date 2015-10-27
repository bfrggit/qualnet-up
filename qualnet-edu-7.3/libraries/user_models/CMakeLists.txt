option(WITH_USER_MODELS "Build the User Models library" ON)

if (WITH_USER_MODELS)
  add_global_definitions(-DUSER_MODELS_LIB)
  add_global_include_dir(src)
  add_srcs(
    src/app_hello.cpp
    src/app_hello.h
    src/app_up.cpp
    src/app_up.h)
  #add_scenario_dir(user_models)
  add_doxygen_inputs(src)
endif ()

add_feature_info(user_models WITH_USER_MODELS "User Models library")
