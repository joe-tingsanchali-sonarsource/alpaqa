#pragma once

#include <alpaqa/export.h>

namespace alpaqa {

/// Flags to be passed to `dlopen`.
struct ALPAQA_EXPORT DynamicLoadFlags {
    /// `RTLD_GLOBAL` (true) or `RTLD_LOCAL` (false).
    bool global = false;
    /// `RTLD_LAZY` (true) or `RTLD_NOW` (false).
    bool lazy = false;
    /// `RTLD_NODELETE`
    bool nodelete = true;
    /// `RTLD_DEEPBIND`
    bool deepbind = true;

    operator int() const;
};

} // namespace alpaqa
