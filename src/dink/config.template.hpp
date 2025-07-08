/*!
    \file
    Copyright (C) 2025 Frank Secilia
*/

#pragma once

// clang-format off

#define dink_version_major ${dink_version_major}
#define dink_version_minor ${dink_version_minor}
#define dink_version_patch ${dink_version_patch}

/*!
    max number of params dispatcher will try to deduce before erroring out

    This value is mostly arbitrary, just higher than the number of parameters likely in generated code.
*/
#define dink_max_deduced_params ${dink_max_deduced_params}

// clang-format on
