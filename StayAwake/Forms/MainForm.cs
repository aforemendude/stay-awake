using StayAwake.Core;

namespace StayAwake.Forms
{
    public partial class MainForm : Form
    {
        private DateTime? _sleepUntil;
        private DateTime? _closeUntil;
        private bool _isStayAwakeActive;
        private bool _isCloseWindowActive;
        private bool _isFirstShowProcessed = false;
        private bool _isExplicitClose = false;
        private bool _requireDisplay;
        private OverlayForm? _overlay;
        private System.Windows.Forms.Timer _highlightTimer;

        public MainForm(EventWaitHandle? showEvent = null)
        {
            InitializeComponent();

            _highlightTimer = new System.Windows.Forms.Timer { Interval = 1500 };
            _highlightTimer.Tick += (s, e) =>
            {
                _overlay?.Hide();
                _highlightTimer.Stop();
            };

            if (showEvent != null)
            {
                Task.Factory.StartNew(() =>
                {
                    while (!_isExplicitClose)
                    {
                        showEvent.WaitOne();
                        Invoke(new Action(() =>
                        {
                            ShowForm();
                        }));
                    }
                }, TaskCreationOptions.LongRunning);
            }

            notifyIcon.Icon = Icon;
            LoadDurations();

            Hide();
        }

        private void LoadDurations()
        {
            // Stay Awake Durations: 30 min to 8 hours in 15 min increments
            cmbSleepDuration.Items.Clear();

            var durations = new List<DurationItem>();
            TimeSpan current = TimeSpan.FromMinutes(30);
            TimeSpan end = TimeSpan.FromHours(8);

            while (current <= end)
            {
                string label = current.ToString(@"hh\:mm\:ss");
                durations.Add(new DurationItem(label, current));
                current = current.Add(TimeSpan.FromMinutes(15));
            }

            // Add 10 seconds option for debugging
            durations.Add(new DurationItem("00:00:10", TimeSpan.FromSeconds(10)));

            cmbSleepDuration.Items.AddRange([.. durations]);

            // Default 2 hours. 
            // 30min(0), 45min(1), 1h(2), 1h15(3), 1h30(4), 1h45(5), 2h(6).
            // Hard coded index without checking as requested.
            cmbSleepDuration.SelectedIndex = 6;

            // Close Window Durations: 15 min to 8h in 15 min increments
            cmbCloseDuration.Items.Clear();

            var closeDurations = new List<DurationItem>();
            TimeSpan closeCurrent = TimeSpan.FromMinutes(15);
            TimeSpan closeEnd = TimeSpan.FromHours(8);

            while (closeCurrent <= closeEnd)
            {
                string label = closeCurrent.ToString(@"hh\:mm\:ss");
                closeDurations.Add(new DurationItem(label, closeCurrent));
                closeCurrent = closeCurrent.Add(TimeSpan.FromMinutes(15));
            }

            // Add 10 seconds option for debugging
            closeDurations.Add(new DurationItem("00:00:10", TimeSpan.FromSeconds(10)));

            cmbCloseDuration.Items.AddRange([.. closeDurations]);

            // Default 1h.
            // 15min(0), 30min(1), 45min(2), 1h(3).
            cmbCloseDuration.SelectedIndex = 3;
        }

        private void RefreshWindows()
        {
            lstWindows.Items.Clear();
            txtProcessName.Text = string.Empty;
            txtWindowHandle.Text = string.Empty;
            lstWindows.Items.AddRange([.. WindowCloser.GetOpenWindows()]);
        }

        private void MainForm_Shown(object sender, EventArgs e)
        {
            if (!_isFirstShowProcessed)
            {
                _isFirstShowProcessed = true;
                Hide();
            }
        }

        private void LstWindows_SelectedIndexChanged(object? sender, EventArgs e)
        {
            if (lstWindows.SelectedItem is WindowInfo info)
            {
                txtProcessName.Text = info.ProcessName;
                txtWindowHandle.Text = info.Handle.ToString("X");

                // Highlight window
                if (NativeMethods.GetWindowRect(info.Handle, out NativeMethods.RECT rect))
                {
                    if (_overlay == null || _overlay.IsDisposed)
                    {
                        _overlay = new OverlayForm();
                    }

                    int width = rect.Right - rect.Left;
                    int height = rect.Bottom - rect.Top;

                    if (width > 0 && height > 0)
                    {
                        _overlay.Bounds = new Rectangle(rect.Left, rect.Top, width, height);
                        _overlay.Show();
                        _highlightTimer.Stop();
                        _highlightTimer.Start();
                    }
                }
                else
                {
                    MessageBox.Show("Unable to retrieve window position.", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                }
            }
            else
            {
                txtProcessName.Text = string.Empty;
                txtWindowHandle.Text = string.Empty;
            }
        }

        private void BtnRefresh_Click(object sender, EventArgs e)
        {
            RefreshWindows();
        }

        private void BtnStayAwakeRequireDisplay_Click(object? sender, EventArgs e)
        {
            StartStayAwake(true);
        }

        private void BtnStayAwakeRequireSystem_Click(object? sender, EventArgs e)
        {
            StartStayAwake(false);
        }

        private void StartStayAwake(bool requireDisplay)
        {
            if (!_isStayAwakeActive)
            {
                // Start
                if (cmbSleepDuration.SelectedItem is DurationItem item)
                {
                    var duration = item.Duration;
                    _sleepUntil = DateTime.Now.Add(duration);
                    _isStayAwakeActive = true;
                    _requireDisplay = requireDisplay;

                    // UI changes
                    if (requireDisplay)
                    {
                        btnStayAwakeRequireDisplay.Text = "Stop Require Display";
                        btnStayAwakeRequireSystem.Enabled = false;
                    }
                    else
                    {
                        btnStayAwakeRequireDisplay.Enabled = false;
                        btnStayAwakeRequireSystem.Text = "Stop Require System";
                    }
                    cmbSleepDuration.Enabled = false;
                    grpSleep.Text = "Stay Awake";

                    PowerManager.KeepAwake(true, requireDisplay);
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
            _sleepUntil = null;

            // UI changes
            btnStayAwakeRequireDisplay.Text = "Require Display";
            btnStayAwakeRequireDisplay.Enabled = true;

            btnStayAwakeRequireSystem.Text = "Require System";
            btnStayAwakeRequireSystem.Enabled = true;

            cmbSleepDuration.Enabled = true;

            lblSleepRemainingTimeValue.Text = "Not Enabled";

            PowerManager.KeepAwake(false);
        }

        private void BtnCloseWindow_Click(object? sender, EventArgs e)
        {
            if (!_isCloseWindowActive)
            {
                // Start
                if (lstWindows.SelectedItem == null)
                {
                    MessageBox.Show("Please select a window.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    return;
                }

                if (cmbCloseDuration.SelectedItem is DurationItem item)
                {
                    var duration = item.Duration;
                    _closeUntil = DateTime.Now.Add(duration);

                    _isCloseWindowActive = true;
                    btnCloseWindow.Text = "Stop";

                    // UI changes
                    lstWindows.Enabled = false;
                    cmbCloseDuration.Enabled = false;
                    btnRefresh.Enabled = false;

                    grpClose.Text = "Window Closer";
                }
            }
            else
            {
                // Stop
                StopCloseWindow();
            }

            UpdateTimerState();
        }

        private void StopCloseWindow()
        {
            _isCloseWindowActive = false;
            btnCloseWindow.Text = "Close Window";
            _closeUntil = null;

            // UI changes
            lstWindows.Enabled = true;
            cmbCloseDuration.Enabled = true;
            btnRefresh.Enabled = true;
            lblCloseRemainingTimeValue.Text = "Not Enabled";
        }

        private void UpdateTimerState()
        {
            if (_isStayAwakeActive || _isCloseWindowActive)
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
                    string type = _requireDisplay ? "Display" : "System";
                    StopStayAwake();
                    UpdateTimerState();
                    grpSleep.Text = $"Stay Awake - Require {type} Ended At {now:MM/dd HH:mm:ss}";
                }
                else
                {
                    lblSleepRemainingTimeValue.Text = remaining.ToString(@"hh\:mm\:ss");
                }
            }

            // Handle Close Window
            if (_isCloseWindowActive && _closeUntil.HasValue)
            {
                if (now >= _closeUntil.Value)
                {
                    StopCloseWindow();
                    UpdateTimerState();

                    if (lstWindows.SelectedItem is WindowInfo target)
                    {
                        WindowCloser.CloseWindow(target.Handle);
                        grpClose.Text = $"Window Closer - Closed {target.Handle:X} At {now:MM/dd HH:mm} ({target.ProcessName})";
                        RefreshWindows();
                    }
                }
                else
                {
                    TimeSpan remaining = _closeUntil.Value - now;
                    lblCloseRemainingTimeValue.Text = remaining.ToString(@"hh\:mm\:ss");
                }
            }
        }

        private void MainForm_FormClosing(object? sender, FormClosingEventArgs e)
        {
            if (!_isExplicitClose)
            {
                e.Cancel = true;
                Hide();

                GC.Collect(GC.MaxGeneration, GCCollectionMode.Aggressive, true, true);
            }
        }

        private void NotifyIcon_MouseClick(object? sender, MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Left)
            {
                ShowForm();
            }
        }

        private void ShowToolStripMenuItem_Click(object? sender, EventArgs e)
        {
            ShowForm();
        }

        private void QuitToolStripMenuItem_Click(object? sender, EventArgs e)
        {
            _isExplicitClose = true;
            Application.Exit();
        }

        private void ShowForm()
        {
            if (!_isCloseWindowActive)
            {
                RefreshWindows();
            }
            Show();
            WindowState = FormWindowState.Normal;
            Activate();
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
