namespace StayAwake.Core
{
    public static class PowerManager
    {
        public static void KeepAwake(bool enable)
        {
            if (enable)
            {
                // Prevent sleep: System required context | Display required | Continuous
                NativeMethods.SetThreadExecutionState(
                    NativeMethods.EXECUTION_STATE.ES_CONTINUOUS |
                    NativeMethods.EXECUTION_STATE.ES_SYSTEM_REQUIRED |
                    NativeMethods.EXECUTION_STATE.ES_DISPLAY_REQUIRED);
            }
            else
            {
                // Allow sleep: Clear flags (just Continuous)
                NativeMethods.SetThreadExecutionState(NativeMethods.EXECUTION_STATE.ES_CONTINUOUS);
            }
        }
    }
}
