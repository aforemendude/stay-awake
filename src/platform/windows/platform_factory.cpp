#include "stay_awake/platform_factory.hpp"

#include "platform/windows/windows_platform_binding.hpp"

namespace stay_awake
{
std::unique_ptr<PlatformBinding> CreatePlatformBinding()
{
    return std::make_unique<windows::WindowsPlatformBinding>();
}
} // namespace stay_awake
