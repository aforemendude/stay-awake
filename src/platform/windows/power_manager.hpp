#pragma once

#include "stay_awake/application_state.hpp"

namespace stay_awake::windows
{
class PowerManager
{
  public:
    ~PowerManager();
    OperationResult Set(std::optional<AwakeMode> mode);
    void Reset() noexcept;

  private:
    bool active_ = false;
};
} // namespace stay_awake::windows
