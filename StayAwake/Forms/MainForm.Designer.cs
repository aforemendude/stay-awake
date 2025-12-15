namespace StayAwake.Forms
{
    partial class MainForm
    {
        private System.ComponentModel.IContainer components = null;

        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        private void InitializeComponent()
        {
            components = new System.ComponentModel.Container();
            grpSleep = new GroupBox();
            lblSleepRemainingTime = new Label();
            lblRemainingTime = new Label();
            lblSleepDuration = new Label();
            cmbSleepDuration = new ComboBox();
            btnStayAwake = new Button();
            grpClose = new GroupBox();
            btnRefresh = new Button();
            lblCloseDuration = new Label();
            cmbCloseDuration = new ComboBox();
            lblWindow = new Label();
            lstWindows = new ListBox();
            chkCloseWindow = new CheckBox();
            timer1 = new System.Windows.Forms.Timer(components);
            grpSleep.SuspendLayout();
            grpClose.SuspendLayout();
            SuspendLayout();
            // 
            // grpSleep
            // 
            grpSleep.Controls.Add(lblSleepRemainingTime);
            grpSleep.Controls.Add(lblRemainingTime);
            grpSleep.Controls.Add(lblSleepDuration);
            grpSleep.Controls.Add(cmbSleepDuration);
            grpSleep.Controls.Add(btnStayAwake);
            grpSleep.Font = new Font("Segoe UI", 12F);
            grpSleep.Location = new Point(12, 11);
            grpSleep.Margin = new Padding(3, 2, 3, 2);
            grpSleep.Name = "grpSleep";
            grpSleep.Padding = new Padding(3, 2, 3, 2);
            grpSleep.Size = new Size(460, 100);
            grpSleep.TabIndex = 0;
            grpSleep.TabStop = false;
            grpSleep.Text = "Stay Awake";
            // 
            // lblSleepRemainingTime
            // 
            lblSleepRemainingTime.AutoSize = true;
            lblSleepRemainingTime.Location = new Point(148, 63);
            lblSleepRemainingTime.Name = "lblSleepRemainingTime";
            lblSleepRemainingTime.Size = new Size(130, 21);
            lblSleepRemainingTime.TabIndex = 4;
            lblSleepRemainingTime.Text = "Remaining Time: ";
            // 
            // lblRemainingTime
            // 
            lblRemainingTime.AutoSize = true;
            lblRemainingTime.Location = new Point(284, 63);
            lblRemainingTime.Name = "lblRemainingTime";
            lblRemainingTime.Size = new Size(95, 21);
            lblRemainingTime.TabIndex = 2;
            lblRemainingTime.Text = "Not Enabled";
            // 
            // lblSleepDuration
            // 
            lblSleepDuration.AutoSize = true;
            lblSleepDuration.Location = new Point(148, 31);
            lblSleepDuration.Name = "lblSleepDuration";
            lblSleepDuration.Size = new Size(74, 21);
            lblSleepDuration.TabIndex = 3;
            lblSleepDuration.Text = "Duration:";
            // 
            // cmbSleepDuration
            // 
            cmbSleepDuration.DropDownStyle = ComboBoxStyle.DropDownList;
            cmbSleepDuration.FormattingEnabled = true;
            cmbSleepDuration.Location = new Point(228, 28);
            cmbSleepDuration.Margin = new Padding(3, 2, 3, 2);
            cmbSleepDuration.Name = "cmbSleepDuration";
            cmbSleepDuration.Size = new Size(226, 29);
            cmbSleepDuration.TabIndex = 1;
            // 
            // btnStayAwake
            // 
            btnStayAwake.Location = new Point(6, 26);
            btnStayAwake.Margin = new Padding(3, 2, 3, 2);
            btnStayAwake.Name = "btnStayAwake";
            btnStayAwake.Size = new Size(136, 30);
            btnStayAwake.TabIndex = 0;
            btnStayAwake.Text = "Stay Awake";
            btnStayAwake.UseVisualStyleBackColor = true;
            btnStayAwake.Click += BtnStayAwake_Click;
            // 
            // grpClose
            // 
            grpClose.Controls.Add(btnRefresh);
            grpClose.Controls.Add(lblCloseDuration);
            grpClose.Controls.Add(cmbCloseDuration);
            grpClose.Controls.Add(lblWindow);
            grpClose.Controls.Add(lstWindows);
            grpClose.Controls.Add(chkCloseWindow);
            grpClose.Font = new Font("Segoe UI", 12F);
            grpClose.Location = new Point(12, 115);
            grpClose.Margin = new Padding(3, 2, 3, 2);
            grpClose.Name = "grpClose";
            grpClose.Padding = new Padding(3, 2, 3, 2);
            grpClose.Size = new Size(460, 235);
            grpClose.TabIndex = 1;
            grpClose.TabStop = false;
            grpClose.Text = "Window Closer";
            // 
            // btnRefresh
            // 
            btnRefresh.Location = new Point(402, 50);
            btnRefresh.Margin = new Padding(3, 2, 3, 2);
            btnRefresh.Name = "btnRefresh";
            btnRefresh.Size = new Size(52, 23);
            btnRefresh.TabIndex = 2;
            btnRefresh.Text = "Ref";
            btnRefresh.UseVisualStyleBackColor = true;
            btnRefresh.Click += BtnRefresh_Click;
            // 
            // lblCloseDuration
            // 
            lblCloseDuration.AutoSize = true;
            lblCloseDuration.Location = new Point(18, 170);
            lblCloseDuration.Name = "lblCloseDuration";
            lblCloseDuration.Size = new Size(47, 21);
            lblCloseDuration.TabIndex = 3;
            lblCloseDuration.Text = "After:";
            // 
            // cmbCloseDuration
            // 
            cmbCloseDuration.DropDownStyle = ComboBoxStyle.DropDownList;
            cmbCloseDuration.FormattingEnabled = true;
            cmbCloseDuration.Location = new Point(79, 167);
            cmbCloseDuration.Margin = new Padding(3, 2, 3, 2);
            cmbCloseDuration.Name = "cmbCloseDuration";
            cmbCloseDuration.Size = new Size(158, 29);
            cmbCloseDuration.TabIndex = 3;
            // 
            // lblWindow
            // 
            lblWindow.AutoSize = true;
            lblWindow.Location = new Point(18, 52);
            lblWindow.Name = "lblWindow";
            lblWindow.Size = new Size(55, 21);
            lblWindow.TabIndex = 4;
            lblWindow.Text = "Target:";
            // 
            // lstWindows
            // 
            lstWindows.FormattingEnabled = true;
            lstWindows.Location = new Point(79, 50);
            lstWindows.Margin = new Padding(3, 2, 3, 2);
            lstWindows.Name = "lstWindows";
            lstWindows.ScrollAlwaysVisible = true;
            lstWindows.Size = new Size(317, 109);
            lstWindows.TabIndex = 1;
            // 
            // chkCloseWindow
            // 
            chkCloseWindow.AutoSize = true;
            chkCloseWindow.Location = new Point(18, 26);
            chkCloseWindow.Margin = new Padding(3, 2, 3, 2);
            chkCloseWindow.Name = "chkCloseWindow";
            chkCloseWindow.Size = new Size(129, 25);
            chkCloseWindow.TabIndex = 0;
            chkCloseWindow.Text = "Close Window";
            chkCloseWindow.UseVisualStyleBackColor = true;
            chkCloseWindow.CheckedChanged += ChkCloseWindow_CheckedChanged;
            // 
            // timer1
            // 
            timer1.Interval = 1000;
            timer1.Tick += Timer1_Tick;
            // 
            // MainForm
            // 
            AutoScaleDimensions = new SizeF(7F, 15F);
            AutoScaleMode = AutoScaleMode.Font;
            ClientSize = new Size(484, 361);
            Controls.Add(grpClose);
            Controls.Add(grpSleep);
            FormBorderStyle = FormBorderStyle.FixedSingle;
            Margin = new Padding(3, 2, 3, 2);
            MaximizeBox = false;
            Name = "MainForm";
            Text = "Stay Awake";
            grpSleep.ResumeLayout(false);
            grpSleep.PerformLayout();
            grpClose.ResumeLayout(false);
            grpClose.PerformLayout();
            ResumeLayout(false);
        }

        private System.Windows.Forms.GroupBox grpSleep;
        private System.Windows.Forms.Button btnStayAwake;
        private System.Windows.Forms.ComboBox cmbSleepDuration;
        private System.Windows.Forms.Label lblSleepDuration;
        private System.Windows.Forms.Label lblRemainingTime;
        private System.Windows.Forms.GroupBox grpClose;
        private System.Windows.Forms.CheckBox chkCloseWindow;
        private System.Windows.Forms.ListBox lstWindows;
        private System.Windows.Forms.Label lblWindow;
        private System.Windows.Forms.ComboBox cmbCloseDuration;
        private System.Windows.Forms.Label lblCloseDuration;
        private System.Windows.Forms.Button btnRefresh;
        private System.Windows.Forms.Timer timer1;
        private Label lblSleepRemainingTime;
    }
}
