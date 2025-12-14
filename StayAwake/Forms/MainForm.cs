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
        private bool _isStayAwakeActive;

        public MainForm()
        {
            InitializeComponent();
            _powerManager = new PowerManager();
            _appTerminator = new AppTerminator();

            LoadDurations();
            RefreshProcesses();

            // Hook up events
            btnStayAwake.Click += btnStayAwake_Click;
            chkKillApp.CheckedChanged += ChkKillApp_CheckedChanged;
        }

        private void LoadDurations()
        {
            cmbSleepDuration.Items.Clear();

            // 30 min to 8 hours in 15 min increments
            var durations = new System.Collections.Generic.List<DurationItem>();
            TimeSpan current = TimeSpan.FromMinutes(30);
            TimeSpan end = TimeSpan.FromHours(8);

            while (current <= end)
            {
                string label;
                if (current.TotalHours >= 1)
                {
                    if (current.Minutes > 0)
                        label = $"{current.Hours} Hour{(current.Hours > 1 ? "s" : "")} {current.Minutes} Mins";
                    else
                        label = $"{current.Hours} Hour{(current.Hours > 1 ? "s" : "")}";
                }
                else
                {
                    label = $"{current.Minutes} Minutes";
                }

                durations.Add(new DurationItem(label, current));
                current = current.Add(TimeSpan.FromMinutes(15));
            }

            cmbSleepDuration.Items.AddRange(durations.ToArray());
            if (cmbSleepDuration.Items.Count > 0)
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

        private void btnStayAwake_Click(object? sender, EventArgs e)
        {
            if (!_isStayAwakeActive)
            {
                // Start
                if (cmbSleepDuration.SelectedItem is DurationItem item)
                {
                    var duration = item.Duration;
                    _sleepUntil = DateTime.Now.Add(duration);

                    _isStayAwakeActive = true;
                    btnStayAwake.Text = "Stop";

                    // UI changes
                    cmbSleepDuration.Visible = false;
                    lblRemainingTime.Visible = true;
                    lblStatus.Text = "System will stay awake.";

                    _powerManager.KeepAwake(true);
                }
            }
            else
            {
                // Stop
                StopStayAwake();
            }

            UpdateTimerState();
        }

        private void StopStayAwake()
        {
            _isStayAwakeActive = false;
            btnStayAwake.Text = "Stay Awake";

            _sleepUntil = null;

            // UI changes
            cmbSleepDuration.Visible = true;
            lblRemainingTime.Visible = false;

            _powerManager.KeepAwake(false);
            lblStatus.Text = "Stay awake processing disabled.";
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
            if (_isStayAwakeActive || chkKillApp.Checked)
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
            if (_isStayAwakeActive && _sleepUntil.HasValue)
            {
                TimeSpan remaining = _sleepUntil.Value - now;

                if (remaining <= TimeSpan.Zero)
                {
                    StopStayAwake();
                    UpdateTimerState();
                    lblStatus.Text = "Stay awake duration expired.";
                    MessageBox.Show("Stay awake duration expired.", "Info", MessageBoxButtons.OK, MessageBoxIcon.Information);
                }
                else
                {
                    lblRemainingTime.Text = remaining.ToString(@"hh\:mm\:ss");
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
