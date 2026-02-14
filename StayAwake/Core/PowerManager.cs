using System.ComponentModel;
using System.Runtime.InteropServices;

namespace StayAwake.Core
{
    public static class PowerManager
    {
        public static void KeepAwake(bool enable, bool requireDisplay = true)
        {
            NativeMethods.EXECUTION_STATE state;
            if (enable)
            {
                // Prevent sleep: Continuous | required states
                state = NativeMethods.EXECUTION_STATE.ES_CONTINUOUS | NativeMethods.EXECUTION_STATE.ES_SYSTEM_REQUIRED;
                if (requireDisplay)
                {
                    state |= NativeMethods.EXECUTION_STATE.ES_DISPLAY_REQUIRED;
                }
            }
            else
            {
                // Allow sleep: Clear flags (just Continuous)
                state = NativeMethods.EXECUTION_STATE.ES_CONTINUOUS;
            }

            // Set state and check for error
            if (NativeMethods.SetThreadExecutionState(state) == 0)
            {
                throw new Win32Exception(Marshal.GetLastWin32Error());
            }
        }
    }
}
