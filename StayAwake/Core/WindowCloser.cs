using System.ComponentModel;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;

namespace StayAwake.Core
{
    public class WindowInfo
    {
        public string Title { get; set; } = string.Empty;
        public IntPtr Handle { get; set; }
        public string ProcessName { get; set; } = string.Empty;

        public override string ToString()
        {
            return Title;
        }
    }

    public class WindowCloser
    {
        public static List<WindowInfo> GetOpenWindows()
        {
            List<WindowInfo> windows = [];

            var shellWindow = NativeMethods.GetShellWindow();

            if (!NativeMethods.EnumWindows(delegate (IntPtr hWnd, IntPtr lParam)
            {
                // Exclude Shell (explorer.exe)
                if (hWnd == shellWindow)
                {
                    return true;
                }

                // Exclude hidden windows
                if (!NativeMethods.IsWindowVisible(hWnd))
                {
                    return true;
                }

                // Exclude windows with no title (or if the title could not be retrieved)
                var length = NativeMethods.GetWindowTextLength(hWnd);
                if (length == 0)
                {
                    return true;
                }
                var builder = new StringBuilder(length + 1);
                if (NativeMethods.GetWindowText(hWnd, builder, builder.Capacity) == 0)
                {
                    return true;
                }

                var title = builder.ToString();

                // Filter out empty title or Program Manager
                if (string.IsNullOrWhiteSpace(title) || title == "Program Manager")
                {
                    return true;
                }

                NativeMethods.GetWindowThreadProcessId(hWnd, out var processId);
                var processName = "Unknown";
                try
                {
                    using var process = Process.GetProcessById((int)processId);
                    processName = process.ProcessName;
                }
                catch { }

                windows.Add(new WindowInfo
                {
                    Title = title,
                    Handle = hWnd,
                    ProcessName = processName
                });
                return true;
            }, IntPtr.Zero))
            {
                throw new Win32Exception(Marshal.GetLastWin32Error());
            }

            return [.. windows.OrderBy(w => w.Title)];
        }

        public static void CloseWindow(IntPtr hWnd)
        {
            // TODO: SendMessage is blocking, should run this on another thread or use PostMessage

            // Checking the last error is more reliable than checking the return value
            NativeMethods.SendMessage(hWnd, NativeMethods.WM_CLOSE, IntPtr.Zero, IntPtr.Zero);
            var lastError = Marshal.GetLastWin32Error();
            if (lastError != 0)
            {
                throw new Win32Exception(lastError);
            }
        }
    }
}
