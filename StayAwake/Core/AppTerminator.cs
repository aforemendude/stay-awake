using System;
using System.Diagnostics;
using System.Linq;

namespace StayAwake.Core
{
    public class AppTerminator
    {
        public string[] GetRunningProcesses()
        {
            return Process.GetProcesses()
                .Select(p => p.ProcessName)
                .OrderBy(n => n)
                .Distinct()
                .ToArray();
        }

        public bool KillProcess(string processName)
        {
            try
            {
                var processes = Process.GetProcessesByName(processName);
                if (processes.Length == 0) return false;

                foreach (var p in processes)
                {
                    p.Kill();
                }
                return true;
            }
            catch (Exception)
            {
                // Log or handle exception if needed
                return false;
            }
        }
    }
}
