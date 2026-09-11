#pragma once

#include "stay_awake/application_state.hpp"

#include <windows.h>

#include <stdexcept>
#include <string_view>
#include <utility>

namespace stay_awake::windows
{
// Only owned resources use these wrappers. Control HWNDs and stock brushes are borrowed from their owners.
template <typename T, auto Destroy> class UniqueResource
{
  public:
    explicit UniqueResource(T value = nullptr) : value_(value)
    {
    }
    ~UniqueResource()
    {
        Reset();
    }
    UniqueResource(const UniqueResource&) = delete;
    UniqueResource& operator=(const UniqueResource&) = delete;
    T Get() const
    {
        return value_;
    }
    T Release() noexcept
    {
        return std::exchange(value_, nullptr);
    }
    void Reset(T value = nullptr) noexcept
    {
        if (value_)
        {
            Destroy(value_);
        }
        value_ = value;
    }

  private:
    T value_;
};

using UniqueHandle = UniqueResource<HANDLE, CloseHandle>;
using UniqueWindow = UniqueResource<HWND, DestroyWindow>;
using UniqueFont = UniqueResource<HFONT, DeleteObject>;
using UniqueIcon = UniqueResource<HICON, DestroyIcon>;
using UniqueMenu = UniqueResource<HMENU, DestroyMenu>;

std::wstring ToWide(std::string_view text);
std::string ToUtf8(std::wstring_view text);
std::string NativeError(std::string_view operation, DWORD error = GetLastError());
void Require(bool success, std::string_view operation);
HWND NativeWindow(WindowIdentity identity);
} // namespace stay_awake::windows
