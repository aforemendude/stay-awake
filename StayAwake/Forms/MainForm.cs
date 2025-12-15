using StayAwake.Core;

namespace StayAwake.Forms
{
    public partial class MainForm : Form
    {
        private readonly PowerManager _powerManager;

        private DateTime? _sleepUntil;
        private DateTime? _closeUntil;
        private bool _isStayAwakeActive;

        public MainForm()
        {
            InitializeComponent();

            _powerManager = new PowerManager();

            LoadDurations();
            RefreshWindows();
        }

        private void LoadDurations()
        {
            cmbSleepDuration.Items.Clear();

            // 30 min to 8 hours in 15 min increments
            var durations = new List<DurationItem>();
            TimeSpan current = TimeSpan.FromMinutes(30);
            TimeSpan end = TimeSpan.FromHours(8);

            while (current <= end)
            {
                string label = current.ToString(@"hh\:mm\:ss");
                durations.Add(new DurationItem(label, current));
                current = current.Add(TimeSpan.FromMinutes(15));
            }

            cmbSleepDuration.Items.AddRange([.. durations]);
            if (cmbSleepDuration.Items.Count > 0)
                cmbSleepDuration.SelectedIndex = 0;

            cmbCloseDuration.Items.AddRange([
                new DurationItem(TimeSpan.FromMinutes(1).ToString(@"hh\:mm\:ss"), TimeSpan.FromMinutes(1)),
                new DurationItem(TimeSpan.FromMinutes(30).ToString(@"hh\:mm\:ss"), TimeSpan.FromMinutes(30)),
                new DurationItem(TimeSpan.FromHours(1).ToString(@"hh\:mm\:ss"), TimeSpan.FromHours(1)),
                new DurationItem(TimeSpan.FromHours(2).ToString(@"hh\:mm\:ss"), TimeSpan.FromHours(2))
            ]);
            cmbCloseDuration.SelectedIndex = 0;
        }

        private void RefreshWindows()
        {
            lstWindows.Items.Clear();
            lstWindows.Items.AddRange([.. WindowCloser.GetOpenWindows()]);
        }

        private void BtnRefresh_Click(object sender, EventArgs e)
        {
            RefreshWindows();
        }

        private void BtnStayAwake_Click(object? sender, EventArgs e)
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
                    cmbSleepDuration.Enabled = false;

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
            cmbSleepDuration.Enabled = true;
            lblRemainingTime.Text = "Not Enabled";

            _powerManager.KeepAwake(false);
        }

        private void ChkCloseWindow_CheckedChanged(object? sender, EventArgs e)
        {
            bool enable = chkCloseWindow.Checked;
            lstWindows.Enabled = !enable;
            cmbCloseDuration.Enabled = !enable;
            btnRefresh.Enabled = !enable;

            if (enable)
            {
                if (lstWindows.SelectedItem == null)
                {
                    MessageBox.Show("Please select a window.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    chkCloseWindow.Checked = false;
                    return;
                }

                if (cmbCloseDuration.SelectedItem is DurationItem item)
                {
                    var duration = item.Duration;
                    _closeUntil = DateTime.Now.Add(duration);
                }
            }
            else
            {
                _closeUntil = null;
            }

            UpdateTimerState();
        }

        private void UpdateTimerState()
        {
            if (_isStayAwakeActive || chkCloseWindow.Checked)
            {
                timer1.Start();
            }
            else
            {
                timer1.Stop();

            }
        }

        private void Timer1_Tick(object sender, EventArgs e)
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
                }
                else
                {
                    lblRemainingTime.Text = remaining.ToString(@"hh\:mm\:ss");
                }
            }

            // Handle Close Window
            if (chkCloseWindow.Checked && _closeUntil.HasValue)
            {
                if (now >= _closeUntil.Value)
                {
                    WindowInfo? target = lstWindows.SelectedItem as WindowInfo;
                    chkCloseWindow.Checked = false;

                    if (target != null)
                    {
                        WindowCloser.CloseWindow(target.Handle);
                        MessageBox.Show($"Window '{target.Title}' closed.", "Window Closer", MessageBoxButtons.OK, MessageBoxIcon.Information);
                    }
                }
                else
                {
                    // Optionally update status with remaining time
                }
            }
        }

        private class DurationItem(string name, TimeSpan duration)
        {
            public string Name { get; } = name;
            public TimeSpan Duration { get; } = duration;

            public override string ToString()
            {
                return Name;
            }
        }
    }
}
