# \file
# copyright (c) 2025 Frank Secilia

# -----------------------------------------------------------------------------
# project files
# -----------------------------------------------------------------------------

list(APPEND dink_library_files
  config.template.hpp
  lib.hpp
)

# -----------------------------------------------------------------------------
# generated configuration
# -----------------------------------------------------------------------------

set(dink_config_template_file config.template.hpp)
set(dink_config_gen_filename config.gen.hpp)
set(dink_config_gen_root "${CMAKE_BINARY_DIR}/gen")
set(dink_config_gen_file
  "${dink_config_gen_root}/dink/${dink_config_gen_filename}")
configure_file("${dink_config_template_file}" "${dink_config_gen_file}")

add_library(dink INTERFACE ${dink_library_files} "${dink_config_gen_file}")
target_include_directories(dink INTERFACE
  "$<BUILD_INTERFACE:${dink_config_gen_root}>"
  "$<BUILD_INTERFACE:${PROJECT_DIR}/src>"
)
