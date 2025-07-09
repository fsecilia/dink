/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

// clang-format off

#define ${target}_version_major ${major}
#define ${target}_version_minor ${minor}
#define ${target}_version_patch ${patch}

/*!
    max number of params dispatcher will try to deduce before erroring out

    This value is mostly arbitrary, just higher than the number of parameters likely in generated code.
*/
#define dink_max_deduced_params ${dink_max_deduced_params}

// clang-format on
