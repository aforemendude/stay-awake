#include "platform/windows/list_scrolling.hpp"

#include "platform/windows/native.hpp"

#include <commctrl.h>
#include <utility>

namespace stay_awake::windows
{
namespace
{
bool CanScrollList(const UINT message, const WPARAM wparam) noexcept
{
    switch (message)
    {
    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
    case WM_CHAR: // Native type-to-select can bring an off-screen item into view.
        return true;
    case WM_KEYDOWN:
        return wparam == VK_UP || wparam == VK_DOWN || wparam == VK_LEFT || wparam == VK_RIGHT || wparam == VK_PRIOR ||
               wparam == VK_NEXT || wparam == VK_HOME || wparam == VK_END;
    case WM_VSCROLL:
    case WM_HSCROLL:
        // Thumb dragging already follows the pointer directly; only suppress incremental scrolling.
        return LOWORD(wparam) == SB_LINEUP || LOWORD(wparam) == SB_LINEDOWN || LOWORD(wparam) == SB_PAGEUP ||
               LOWORD(wparam) == SB_PAGEDOWN || LOWORD(wparam) == SB_TOP || LOWORD(wparam) == SB_BOTTOM;
    default:
        return false;
    }
}

struct ListPaintState
{
    explicit ListPaintState(HWND window) noexcept
        : top(SendMessageW(window, LB_GETTOPINDEX, 0, 0)), horizontal(GetScrollPos(window, SB_HORZ)),
          selection(SendMessageW(window, LB_GETCURSEL, 0, 0)), caret(SendMessageW(window, LB_GETCARETINDEX, 0, 0)),
          focused(GetFocus() == window), ui_state(SendMessageW(window, WM_QUERYUISTATE, 0, 0))
    {
    }

    LRESULT top;
    int horizontal;
    LRESULT selection;
    LRESULT caret;
    bool focused;
    LRESULT ui_state;
};

void InvalidateListItem(HWND window, const LRESULT index) noexcept
{
    if (index == LB_ERR)
    {
        return;
    }
    RECT item{};
    RECT client{};
    if (SendMessageW(window, LB_GETITEMRECT, static_cast<WPARAM>(index), reinterpret_cast<LPARAM>(&item)) == LB_ERR ||
        !GetClientRect(window, &client))
    {
        InvalidateRect(window, nullptr, TRUE);
        return;
    }
    // Selection and focus decoration can span the whole row, including past the text's horizontal extent.
    item.left = client.left;
    item.right = client.right;
    RECT visible{};
    if (IntersectRect(&visible, &item, &client))
    {
        InvalidateRect(window, &visible, TRUE);
    }
}

class ListRedrawScope
{
  public:
    ListRedrawScope(HWND window, HWND combo, bool& active) noexcept
        : window_(window), combo_(combo), active_(active), before_(window)
    {
        active_ = true;
        SendMessageW(window_, WM_SETREDRAW, FALSE, 0);
    }
    ~ListRedrawScope() noexcept
    {
        if (IsWindow(window_))
        {
            const bool keep_visible =
                !combo_ || (IsWindowVisible(combo_) && SendMessageW(combo_, CB_GETDROPPEDSTATE, 0, 0) != FALSE);
            SendMessageW(window_, WM_SETREDRAW, TRUE, 0);
            if (keep_visible)
            {
                const ListPaintState after(window_);
                if (before_.top != after.top || before_.horizontal != after.horizontal)
                {
                    // Scrolling changes the viewport and scrollbar thumbs. Queue one repaint so successive
                    // inputs can coalesce; keep erasure for empty space and partially visible rows.
                    RedrawWindow(window_, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_NOCHILDREN);
                }
                else
                {
                    // Navigation within the viewport only changes selection/focus rows. Input at a scroll
                    // boundary (or a partial wheel delta) need not invalidate the whole control again.
                    if (before_.selection != after.selection)
                    {
                        InvalidateListItem(window_, before_.selection);
                        InvalidateListItem(window_, after.selection);
                    }
                    if (before_.caret != after.caret || before_.focused != after.focused ||
                        before_.ui_state != after.ui_state)
                    {
                        InvalidateListItem(window_, before_.caret);
                        InvalidateListItem(window_, after.caret);
                    }
                }
            }
            else
            {
                // WM_SETREDRAW(TRUE) can restore WS_VISIBLE after native input closed the dropdown.
                ShowWindow(window_, SW_HIDE);
            }
        }
        active_ = false;
    }
    ListRedrawScope(const ListRedrawScope&) = delete;
    ListRedrawScope& operator=(const ListRedrawScope&) = delete;

  private:
    HWND window_;
    HWND combo_;
    bool& active_;
    ListPaintState before_;
};
} // namespace

ListScrolling::ListScrolling(std::exception_ptr& failure) noexcept : failure_(failure)
{
}

ListScrolling::~ListScrolling() noexcept
{
    Detach();
}

void ListScrolling::Attach(HWND control, const bool combo, std::function<void()> on_selection)
{
    Detach();
    on_selection_ = std::move(on_selection);
    control_ = control;
    list_ = control;
    combo_ = combo;
    try
    {
        const auto subclass_id = reinterpret_cast<UINT_PTR>(this);
        Require(SetWindowSubclass(control_, WindowProc, subclass_id, reinterpret_cast<DWORD_PTR>(this)) != FALSE,
                "Configure immediate list scrolling");
        if (combo_)
        {
            COMBOBOXINFO info{};
            info.cbSize = sizeof(info);
            Require(GetComboBoxInfo(control_, &info) != FALSE, "Get duration dropdown list");
            list_ = info.hwndList;
            // Mouse input reaches the popup directly; keyboard input can arrive through the combo box.
            // Both callbacks share one redraw scope when native processing forwards input.
            Require(SetWindowSubclass(list_, WindowProc, subclass_id, reinterpret_cast<DWORD_PTR>(this)) != FALSE,
                    "Configure immediate dropdown scrolling");
        }
    }
    catch (...)
    {
        Detach();
        throw;
    }
}

void ListScrolling::Detach() noexcept
{
    const auto subclass_id = reinterpret_cast<UINT_PTR>(this);
    if (list_ && list_ != control_)
    {
        RemoveWindowSubclass(list_, WindowProc, subclass_id);
    }
    if (control_)
    {
        RemoveWindowSubclass(control_, WindowProc, subclass_id);
    }
    control_ = nullptr;
    list_ = nullptr;
    combo_ = false;
    active_ = false;
    selection_pending_ = false;
    on_selection_ = {};
}

bool ListScrolling::DeferSelection() noexcept
{
    if (!active_)
    {
        return false;
    }
    selection_pending_ = true;
    return true;
}

LRESULT CALLBACK ListScrolling::WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam,
                                           UINT_PTR subclass_id, DWORD_PTR reference) noexcept
{
    auto* self = reinterpret_cast<ListScrolling*>(reference);
    try
    {
        if (message == WM_NCDESTROY)
        {
            RemoveWindowSubclass(window, WindowProc, subclass_id);
            if (self->control_ == window)
            {
                self->control_ = nullptr;
            }
            if (self->list_ == window)
            {
                self->list_ = nullptr;
            }
            self->selection_pending_ = false;
            return DefSubclassProc(window, message, wparam, lparam);
        }
        if (!CanScrollList(message, wparam) || self->active_ || !IsWindowVisible(window) || !IsWindowEnabled(window))
        {
            return DefSubclassProc(window, message, wparam, lparam);
        }
        HWND list = window;
        HWND combo = nullptr;
        if (self->combo_)
        {
            combo = self->control_;
            COMBOBOXINFO info{};
            info.cbSize = sizeof(info);
            Require(GetComboBoxInfo(combo, &info) != FALSE, "Get duration dropdown list");
            list = info.hwndList;
            // A collapsed combo still handles selection normally; never enable drawing on its hidden popup.
            if (!IsWindowVisible(list))
            {
                return DefSubclassProc(window, message, wparam, lparam);
            }
        }
        LRESULT result;
        {
            // Let the native control handle wheel deltas, selection and navigation, but paint only the final
            // position. This bypasses its smooth-scroll effect without changing the user's system preference.
            ListRedrawScope redraw(list, combo, self->active_);
            result = DefSubclassProc(window, message, wparam, lparam);
        }
        if (std::exchange(self->selection_pending_, false))
        {
            self->on_selection_();
        }
        return result;
    }
    catch (...)
    {
        self->failure_ = std::current_exception();
        PostQuitMessage(1);
        return 0;
    }
}
} // namespace stay_awake::windows
