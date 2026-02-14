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
        private bool _isHighlightActive;

        public MainForm(EventWaitHandle? showEvent = null)
        {
            InitializeComponent();

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
                var label = current.ToString(@"hh\:mm\:ss");
                durations.Add(new DurationItem(label, current));
                current = current.Add(TimeSpan.FromMinutes(15));
            }

            // Add 10 seconds option for debugging
            durations.Add(new DurationItem("00:00:10", TimeSpan.FromSeconds(10)));

            cmbSleepDuration.Items.AddRange([.. durations]);

            // Default 2 hours
            // 30min(0), 45min(1), 1h(2), 1h15(3), 1h30(4), 1h45(5), 2h(6)
            // Hard coded index without checking as requested
            cmbSleepDuration.SelectedIndex = 6;

            // Close Window Durations: 15 min to 8h in 15 min increments
            cmbCloseDuration.Items.Clear();

            var closeDurations = new List<DurationItem>();
            TimeSpan closeCurrent = TimeSpan.FromMinutes(15);
            TimeSpan closeEnd = TimeSpan.FromHours(8);

            while (closeCurrent <= closeEnd)
            {
                var label = closeCurrent.ToString(@"hh\:mm\:ss");
                closeDurations.Add(new DurationItem(label, closeCurrent));
                closeCurrent = closeCurrent.Add(TimeSpan.FromMinutes(15));
            }

            // Add 10 seconds option for debugging
            closeDurations.Add(new DurationItem("00:00:10", TimeSpan.FromSeconds(10)));

            cmbCloseDuration.Items.AddRange([.. closeDurations]);

            // Default 1h
            // 15min(0), 30min(1), 45min(2), 1h(3)
            cmbCloseDuration.SelectedIndex = 3;
        }

        private void RefreshWindows(bool isAsync)
        {
            try
            {
                lstWindows.Items.Clear();
                txtProcessName.Text = string.Empty;
                txtWindowHandle.Text = string.Empty;
                txtWindowPositionValue.Text = string.Empty;
                lstWindows.Items.AddRange([.. WindowCloser.GetOpenWindows()]);
            }
            catch (Exception ex)
            {
                if (!isAsync)
                {
                    MessageBox.Show($"Failed to refresh windows list: {ex.Message}", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }
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
                if (NativeMethods.GetWindowRect(info.Handle, out var rect))
                {
                    var width = rect.Right - rect.Left;
                    var height = rect.Bottom - rect.Top;

                    txtWindowPositionValue.Text = $"X: {rect.Left}, Y: {rect.Top}, Width: {width}, Height: {height}";

                    if (_isHighlightActive)
                    {
                        if (_overlay == null || _overlay.IsDisposed)
                        {
                            _overlay = new OverlayForm();
                        }

                        if (width > 0 && height > 0)
                        {
                            _overlay.Bounds = new Rectangle(rect.Left, rect.Top, width, height);
                            if (!_overlay.Visible)
                            {
                                _overlay.Show();
                                // Take focus back, only needed when showing the overlay, not needed when moving
                                Focus();
                            }
                        }
                    }
                    else
                    {
                        _overlay?.Hide();
                    }
                }
                else
                {
                    txtWindowPositionValue.Text = "Error getting position";
                    _overlay?.Hide();
                }
            }
            else
            {
                txtProcessName.Text = string.Empty;
                txtWindowHandle.Text = string.Empty;
                txtWindowPositionValue.Text = string.Empty;
                _overlay?.Hide();
            }
        }

        private void BtnRefresh_Click(object sender, EventArgs e)
        {
            RefreshWindows(false);
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
                try
                {
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
                catch (Exception ex)
                {
                    StopStayAwake(true);
                    MessageBox.Show($"Failed to start stay awake: {ex.Message}", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }
            else
            {
                StopStayAwake(false);
            }

            UpdateTimerState();
        }

        private bool StopStayAwake(bool isAsync)
        {
            try
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
                return true;
            }
            catch (Exception ex)
            {
                if (!isAsync)
                {
                    MessageBox.Show($"Failed to stop stay awake: {ex.Message}", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
                return false;
            }
        }

        private void BtnCloseWindow_Click(object? sender, EventArgs e)
        {
            if (!_isCloseWindowActive)
            {
                if (lstWindows.SelectedItem == null)
                {
                    MessageBox.Show("No window selected.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
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
                mainTimer.Start();
            }
            else
            {
                mainTimer.Stop();
            }
        }

        private void MainTimer_Tick(object sender, EventArgs e)
        {
            var now = DateTime.Now;

            // Handle Sleep
            if (_isStayAwakeActive && _sleepUntil.HasValue)
            {
                var remaining = _sleepUntil.Value - now;

                if (remaining <= TimeSpan.Zero)
                {
                    var type = _requireDisplay ? "Display" : "System";
                    if (StopStayAwake(true))
                    {
                        grpSleep.Text = $"Stay Awake - Require {type} Ended At {now:MM/dd HH:mm:ss}";
                    }
                    else
                    {
                        grpSleep.Text = $"Stay Awake - Error Ending Require {type} At {now:MM/dd HH:mm:ss}";
                    }
                    UpdateTimerState();
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
                    if (lstWindows.SelectedItem is WindowInfo target)
                    {
                        try
                        {
                            WindowCloser.CloseWindow(target.Handle);
                            grpClose.Text = $"Window Closer - Closed {target.Handle:X} At {now:MM/dd HH:mm} ({target.ProcessName})";
                        }
                        catch
                        {
                            grpClose.Text = $"Window Closer - Error Closing {target.Handle:X} At {now:MM/dd HH:mm} ({target.ProcessName})";
                        }
                        StopCloseWindow();
                        UpdateTimerState();
                        RefreshWindows(true);
                    }
                    else
                    {
                        StopCloseWindow();
                        UpdateTimerState();
                    }
                }
                else
                {
                    var remaining = _closeUntil.Value - now;
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

                if (_isHighlightActive)
                {
                    _isHighlightActive = false;
                    btnHighlightWindow.Text = "Highlight Window";
                    _overlay?.Hide();
                }

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

        private void BtnHighlightWindow_Click(object sender, EventArgs e)
        {
            if (_isHighlightActive)
            {
                _isHighlightActive = false;
                btnHighlightWindow.Text = "Highlight Window";
                _overlay?.Hide();
            }
            else
            {
                _isHighlightActive = true;
                btnHighlightWindow.Text = "Stop Highlighting";
                LstWindows_SelectedIndexChanged(sender, e);
            }
        }

        private void ShowForm()
        {
            if (!_isCloseWindowActive)
            {
                RefreshWindows(true);
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
