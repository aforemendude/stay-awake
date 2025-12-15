using System.Runtime.InteropServices;
using System.Text;

namespace StayAwake.Core
{
    public class WindowInfo
    {
        public string Title { get; set; } = string.Empty;
        public IntPtr Handle { get; set; }

        public override string ToString()
        {
            return Title;
        }
    }

    public class WindowCloser
    {
        public static List<WindowInfo> GetOpenWindows()
        {
            var windows = new List<WindowInfo>();
            IntPtr shellWindow = NativeMethods.GetShellWindow();

            NativeMethods.EnumWindows(delegate (IntPtr hWnd, IntPtr lParam)
            {
                if (hWnd == shellWindow) return true;
                if (!NativeMethods.IsWindowVisible(hWnd)) return true;

                int length = NativeMethods.GetWindowTextLength(hWnd);
                if (length == 0) return true;

                var builder = new StringBuilder(length + 1);
                NativeMethods.GetWindowText(hWnd, builder, builder.Capacity);
                string title = builder.ToString();

                // Filter out empty titles or Program Manager
                if (string.IsNullOrWhiteSpace(title) || title == "Program Manager") return true;

                windows.Add(new WindowInfo { Title = title, Handle = hWnd });
                return true;

            }, IntPtr.Zero);

            return [.. windows.OrderBy(w => w.Title)];
        }

        public static void CloseWindow(IntPtr hWnd)
        {
            NativeMethods.SendMessage(hWnd, NativeMethods.WM_CLOSE, IntPtr.Zero, IntPtr.Zero);
        }
    }
}
