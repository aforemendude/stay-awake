#pragma once

#include "platform/windows/native.hpp"

namespace stay_awake::windows
{
class SingleInstance
{
  public:
    ~SingleInstance();
    // Returns true for the owner, false after delivering Show to an existing owner; throws on bounded failure.
    bool AcquireOrShow();
    HANDLE ShowEvent() const;

  private:
    UniqueHandle mutex_;
    UniqueHandle event_;
    bool owns_mutex_ = false;
};
} // namespace stay_awake::windows
