using System.Diagnostics;
using System.Runtime.InteropServices;
using StayAwake.Forms;

namespace StayAwake
{
    static class Program
    {
        [DllImport("user32.dll")]
        private static extern bool SetForegroundWindow(IntPtr hWnd);

        [DllImport("user32.dll")]
        private static extern bool ShowWindowAsync(IntPtr hWnd, int nCmdShow);

        private const int SW_RESTORE = 9;

        /// <summary>
        ///  The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main()
        {
            const string appName = "StayAwake_SingleInstance_Mutex";

            using (var mutex = new Mutex(true, appName, out bool createdNew))
            {
                if (!createdNew)
                {
                    Process current = Process.GetCurrentProcess();
                    foreach (Process process in Process.GetProcessesByName(current.ProcessName))
                    {
                        if (process.Id != current.Id)
                        {
                            // Found the other instance
                            IntPtr handle = process.MainWindowHandle;
                            if (handle != IntPtr.Zero)
                            {
                                ShowWindowAsync(handle, SW_RESTORE);
                                SetForegroundWindow(handle);
                            }
                            break;
                        }
                    }
                    // Exit the current (second) instance
                    return;
                }

                // To customize application configuration such as set high DPI settings or default font,
                // see https://aka.ms/applicationconfiguration.
                ApplicationConfiguration.Initialize();
                Application.Run(new MainForm());
            }
        }
    }
}