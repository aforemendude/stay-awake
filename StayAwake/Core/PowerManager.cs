namespace StayAwake.Core
{
    public class PowerManager
    {
        private bool _isAwake;

        public bool IsAwake => _isAwake;

        public void KeepAwake(bool enable)
        {
            if (enable)
            {
                // Prevent sleep: System required context | Display required | Continuous
                NativeMethods.SetThreadExecutionState(
                    NativeMethods.EXECUTION_STATE.ES_CONTINUOUS |
                    NativeMethods.EXECUTION_STATE.ES_SYSTEM_REQUIRED |
                    NativeMethods.EXECUTION_STATE.ES_DISPLAY_REQUIRED);
                _isAwake = true;
            }
            else
            {
                // Allow sleep: Clear flags (just Continuous)
                NativeMethods.SetThreadExecutionState(NativeMethods.EXECUTION_STATE.ES_CONTINUOUS);
                _isAwake = false;
            }
        }
    }
}
