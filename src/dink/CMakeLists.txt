#
# \file
# copyright (c) 2025 Frank Secilia
#

# -----------------------------------------------------------------------------
# generated configuration
# -----------------------------------------------------------------------------

set(dink_config_template_file config.template.hpp)
set(dink_config_gen_filename config.gen.hpp)
set(dink_config_gen_root "${CMAKE_BINARY_DIR}/gen")
set(dink_config_gen_file
  "${dink_config_gen_root}/dink/${dink_config_gen_filename}")
configure_file("${dink_config_template_file}" "${dink_config_gen_file}")
