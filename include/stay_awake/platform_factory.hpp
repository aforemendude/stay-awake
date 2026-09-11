#pragma once

#include "stay_awake/platform_binding.hpp"

#include <memory>

namespace stay_awake
{
std::unique_ptr<PlatformBinding> CreatePlatformBinding();
} // namespace stay_awake
