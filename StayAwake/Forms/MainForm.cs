using System;
using System.Drawing;
using System.Windows.Forms;
using StayAwake.Core;

namespace StayAwake.Forms
{
    public partial class MainForm : Form
    {
        private readonly PowerManager _powerManager;
        private readonly AppTerminator _appTerminator;

        private DateTime? _sleepUntil;
        private DateTime? _killUntil;

        public MainForm()
        {
            InitializeComponent();
            _powerManager = new PowerManager();
            _appTerminator = new AppTerminator();

            LoadDurations();
            RefreshProcesses();

            // Hook up events
            chkStayAwake.CheckedChanged += ChkStayAwake_CheckedChanged;
            chkKillApp.CheckedChanged += ChkKillApp_CheckedChanged;
        }

        private void LoadDurations()
        {
            var durations = new object[] {
                new DurationItem("Indefinite", TimeSpan.MaxValue),
                new DurationItem("30 Minutes", TimeSpan.FromMinutes(30)),
                new DurationItem("1 Hour", TimeSpan.FromHours(1)),
                new DurationItem("2 Hours", TimeSpan.FromHours(2))
            };

            cmbSleepDuration.Items.AddRange(durations);
            cmbSleepDuration.SelectedIndex = 0;

            cmbKillDuration.Items.AddRange(new object[] {
                new DurationItem("1 Minute", TimeSpan.FromMinutes(1)),
                new DurationItem("30 Minutes", TimeSpan.FromMinutes(30)),
                new DurationItem("1 Hour", TimeSpan.FromHours(1)),
                new DurationItem("2 Hours", TimeSpan.FromHours(2))
            });
            cmbKillDuration.SelectedIndex = 0;
        }

        private void RefreshProcesses()
        {
            var current = cmbProcesses.SelectedItem as string;
            cmbProcesses.Items.Clear();
            cmbProcesses.Items.AddRange(_appTerminator.GetRunningProcesses());

            if (current != null && cmbProcesses.Items.Contains(current))
            {
                cmbProcesses.SelectedItem = current;
            }
            else if (cmbProcesses.Items.Count > 0)
            {
                cmbProcesses.SelectedIndex = 0;
            }
        }

        private void btnRefresh_Click(object sender, EventArgs e)
        {
            RefreshProcesses();
        }

        private void ChkStayAwake_CheckedChanged(object? sender, EventArgs e)
        {
            bool enable = chkStayAwake.Checked;
            cmbSleepDuration.Enabled = !enable;

            if (enable)
            {
                if (cmbSleepDuration.SelectedItem is DurationItem item)
                {
                    var duration = item.Duration;
                    if (duration != TimeSpan.MaxValue)
                    {
                        _sleepUntil = DateTime.Now.Add(duration);
                    }
                    else
                    {
                        _sleepUntil = null;
                    }
                }

                _powerManager.KeepAwake(true);
                lblStatus.Text = "System will stay awake.";
            }
            else
            {
                _powerManager.KeepAwake(false);
                _sleepUntil = null;
                lblStatus.Text = "Stay awake processing disabled.";
            }

            UpdateTimerState();
        }

        private void ChkKillApp_CheckedChanged(object? sender, EventArgs e)
        {
            bool enable = chkKillApp.Checked;
            cmbProcesses.Enabled = !enable;
            cmbKillDuration.Enabled = !enable;
            btnRefresh.Enabled = !enable;

            if (enable)
            {
                if (cmbProcesses.SelectedItem == null)
                {
                    MessageBox.Show("Please select a process.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    chkKillApp.Checked = false;
                    return;
                }

                if (cmbKillDuration.SelectedItem is DurationItem item)
                {
                    var duration = item.Duration;
                    _killUntil = DateTime.Now.Add(duration);
                    lblStatus.Text = $"Terminator active for {cmbProcesses.SelectedItem}.";
                }
            }
            else
            {
                _killUntil = null;
                lblStatus.Text = "Terminator disabled.";
            }

            UpdateTimerState();
        }

        private void UpdateTimerState()
        {
            if (chkStayAwake.Checked || chkKillApp.Checked)
            {
                timer1.Start();
            }
            else
            {
                timer1.Stop();
                lblStatus.Text = "Ready";
            }
        }

        private void timer1_Tick(object sender, EventArgs e)
        {
            var now = DateTime.Now;

            // Handle Sleep
            if (chkStayAwake.Checked && _sleepUntil.HasValue)
            {
                if (now >= _sleepUntil.Value)
                {
                    chkStayAwake.Checked = false; // Will trigger event and disable KeepAwake
                    lblStatus.Text = "Stay awake duration expired.";
                    MessageBox.Show("Stay awake duration expired.", "Info", MessageBoxButtons.OK, MessageBoxIcon.Information);
                }
                else
                {
                    // Optionally update status with remaining time
                }
            }

            // Handle Kill
            if (chkKillApp.Checked && _killUntil.HasValue)
            {
                if (now >= _killUntil.Value)
                {
                    string? target = cmbProcesses.SelectedItem as string;
                    chkKillApp.Checked = false;

                    if (!string.IsNullOrEmpty(target))
                    {
                        bool killed = _appTerminator.KillProcess(target);
                        string msg = killed ? $"Process {target} terminated." : $"Process {target} not found or could not be terminated.";
                        lblStatus.Text = msg;
                        MessageBox.Show(msg, "Terminator", MessageBoxButtons.OK, MessageBoxIcon.Information);
                    }
                }
                else
                {
                    // Optionally update status with remaining time
                }
            }
        }

        private class DurationItem
        {
            public string Name { get; }
            public TimeSpan Duration { get; }

            public DurationItem(string name, TimeSpan duration)
            {
                Name = name;
                Duration = duration;
            }

            public override string ToString()
            {
                return Name;
            }
        }
    }
}
