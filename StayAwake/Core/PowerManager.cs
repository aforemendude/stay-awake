namespace StayAwake.Core
{
    public static class PowerManager
    {
        public static void KeepAwake(bool enable, bool requireDisplay = true)
        {
            if (enable)
            {
                NativeMethods.EXECUTION_STATE state = NativeMethods.EXECUTION_STATE.ES_CONTINUOUS | NativeMethods.EXECUTION_STATE.ES_SYSTEM_REQUIRED;
                if (requireDisplay)
                {
                    state |= NativeMethods.EXECUTION_STATE.ES_DISPLAY_REQUIRED;
                }

                // Prevent sleep: Continuous | required states
                NativeMethods.SetThreadExecutionState(state);
            }
            else
            {
                // Allow sleep: Clear flags (just Continuous)
                NativeMethods.SetThreadExecutionState(NativeMethods.EXECUTION_STATE.ES_CONTINUOUS);
            }
        }
    }
}
