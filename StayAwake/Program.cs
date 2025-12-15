using StayAwake.Forms;

namespace StayAwake
{
    static class Program
    {
        [STAThread]
        static void Main()
        {
            const string mutexKey = "StayAwake-Mutex-1927b19d-4cad-4589-9ed1-17ae32b96c1b";
            const string eventKey = "StayAwake-Event-1927b19d-4cad-4589-9ed1-17ae32b96c1b";

            using var mutex = new Mutex(true, mutexKey, out bool createdNew);
            if (!createdNew)
            {
                // Signal the other instance
                try
                {
                    using var eventHandle = EventWaitHandle.OpenExisting(eventKey);
                    eventHandle.Set();
                }
                catch (WaitHandleCannotBeOpenedException)
                {
                    // If we can't open it, maybe it's closing or in a weird state.
                    // Fallback or ignore.
                }

                // Exit the current (second) instance
                return;
            }

            using var showEvent = new EventWaitHandle(false, EventResetMode.AutoReset, eventKey);

            // To customize application configuration such as set high DPI settings or default font,
            // see https://aka.ms/applicationconfiguration.
            ApplicationConfiguration.Initialize();
            Application.Run(new MainForm(showEvent));
        }
    }
}