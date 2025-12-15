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
            lblSleepRemainingTimeLabel = new Label();
            lblSleepRemainingTimeValue = new Label();
            lblSleepDuration = new Label();
            cmbSleepDuration = new ComboBox();
            btnStayAwake = new Button();
            grpClose = new GroupBox();
            btnRefresh = new Button();
            lblCloseDuration = new Label();
            cmbCloseDuration = new ComboBox();
            lstWindows = new ListBox();
            lblCloseRemainingTimeValue = new Label();
            lblCloseRemainingTimeLabel = new Label();
            btnCloseWindow = new Button();
            lblProcessNameLabel = new Label();
            txtProcessName = new TextBox();
            lblWindowHandleLabel = new Label();
            txtWindowHandle = new TextBox();
            timer1 = new System.Windows.Forms.Timer(components);
            grpSleep.SuspendLayout();
            grpClose.SuspendLayout();
            SuspendLayout();
            // 
            // grpSleep
            // 
            grpSleep.Controls.Add(lblSleepRemainingTimeLabel);
            grpSleep.Controls.Add(lblSleepRemainingTimeValue);
            grpSleep.Controls.Add(lblSleepDuration);
            grpSleep.Controls.Add(cmbSleepDuration);
            grpSleep.Controls.Add(btnStayAwake);
            grpSleep.Font = new Font("Segoe UI", 12F);
            grpSleep.Location = new Point(12, 11);
            grpSleep.Margin = new Padding(3, 2, 3, 2);
            grpSleep.Name = "grpSleep";
            grpSleep.Padding = new Padding(3, 2, 3, 2);
            grpSleep.Size = new Size(760, 100);
            grpSleep.TabIndex = 0;
            grpSleep.TabStop = false;
            grpSleep.Text = "Stay Awake";
            // 
            // lblSleepRemainingTimeLabel
            // 
            lblSleepRemainingTimeLabel.AutoSize = true;
            lblSleepRemainingTimeLabel.Location = new Point(208, 63);
            lblSleepRemainingTimeLabel.Name = "lblSleepRemainingTimeLabel";
            lblSleepRemainingTimeLabel.Size = new Size(130, 21);
            lblSleepRemainingTimeLabel.TabIndex = 4;
            lblSleepRemainingTimeLabel.Text = "Remaining Time: ";
            // 
            // lblSleepRemainingTimeValue
            // 
            lblSleepRemainingTimeValue.AutoSize = true;
            lblSleepRemainingTimeValue.Location = new Point(344, 63);
            lblSleepRemainingTimeValue.Name = "lblSleepRemainingTimeValue";
            lblSleepRemainingTimeValue.Size = new Size(95, 21);
            lblSleepRemainingTimeValue.TabIndex = 2;
            lblSleepRemainingTimeValue.Text = "Not Enabled";
            // 
            // lblSleepDuration
            // 
            lblSleepDuration.AutoSize = true;
            lblSleepDuration.Location = new Point(208, 31);
            lblSleepDuration.Name = "lblSleepDuration";
            lblSleepDuration.Size = new Size(74, 21);
            lblSleepDuration.TabIndex = 3;
            lblSleepDuration.Text = "Duration:";
            // 
            // cmbSleepDuration
            // 
            cmbSleepDuration.DropDownStyle = ComboBoxStyle.DropDownList;
            cmbSleepDuration.FormattingEnabled = true;
            cmbSleepDuration.Location = new Point(288, 26);
            cmbSleepDuration.Margin = new Padding(3, 2, 3, 2);
            cmbSleepDuration.Name = "cmbSleepDuration";
            cmbSleepDuration.Size = new Size(151, 29);
            cmbSleepDuration.TabIndex = 1;
            // 
            // btnStayAwake
            // 
            btnStayAwake.Location = new Point(6, 26);
            btnStayAwake.Margin = new Padding(3, 2, 3, 2);
            btnStayAwake.Name = "btnStayAwake";
            btnStayAwake.Size = new Size(196, 30);
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
            grpClose.Controls.Add(lstWindows);
            grpClose.Controls.Add(lblCloseRemainingTimeValue);
            grpClose.Controls.Add(lblCloseRemainingTimeLabel);
            grpClose.Controls.Add(btnCloseWindow);
            grpClose.Controls.Add(lblProcessNameLabel);
            grpClose.Controls.Add(txtProcessName);
            grpClose.Controls.Add(lblWindowHandleLabel);
            grpClose.Controls.Add(txtWindowHandle);
            grpClose.Font = new Font("Segoe UI", 12F);
            grpClose.Location = new Point(12, 115);
            grpClose.Margin = new Padding(3, 2, 3, 2);
            grpClose.Name = "grpClose";
            grpClose.Padding = new Padding(3, 2, 3, 2);
            grpClose.Size = new Size(760, 335);
            grpClose.TabIndex = 1;
            grpClose.TabStop = false;
            grpClose.Text = "Window Closer";
            // 
            // btnRefresh
            // 
            btnRefresh.Location = new Point(6, 63);
            btnRefresh.Margin = new Padding(3, 2, 3, 2);
            btnRefresh.Name = "btnRefresh";
            btnRefresh.Size = new Size(196, 30);
            btnRefresh.TabIndex = 2;
            btnRefresh.Text = "Refresh List";
            btnRefresh.UseVisualStyleBackColor = true;
            btnRefresh.Click += BtnRefresh_Click;
            // 
            // lblCloseDuration
            // 
            lblCloseDuration.AutoSize = true;
            lblCloseDuration.Location = new Point(208, 31);
            lblCloseDuration.Name = "lblCloseDuration";
            lblCloseDuration.Size = new Size(47, 21);
            lblCloseDuration.TabIndex = 3;
            lblCloseDuration.Text = "After:";
            // 
            // cmbCloseDuration
            // 
            cmbCloseDuration.DropDownStyle = ComboBoxStyle.DropDownList;
            cmbCloseDuration.FormattingEnabled = true;
            cmbCloseDuration.Location = new Point(288, 26);
            cmbCloseDuration.Margin = new Padding(3, 2, 3, 2);
            cmbCloseDuration.Name = "cmbCloseDuration";
            cmbCloseDuration.Size = new Size(151, 29);
            cmbCloseDuration.TabIndex = 3;
            // 
            // lstWindows
            // 
            lstWindows.FormattingEnabled = true;
            lstWindows.Location = new Point(6, 113);
            lstWindows.Margin = new Padding(3, 2, 3, 2);
            lstWindows.Name = "lstWindows";
            lstWindows.ScrollAlwaysVisible = true;
            lstWindows.Size = new Size(748, 214);
            lstWindows.TabIndex = 1;
            lstWindows.SelectedIndexChanged += LstWindows_SelectedIndexChanged;
            // 
            // lblCloseRemainingTimeValue
            // 
            lblCloseRemainingTimeValue.AutoSize = true;
            lblCloseRemainingTimeValue.Location = new Point(344, 68);
            lblCloseRemainingTimeValue.Name = "lblCloseRemainingTimeValue";
            lblCloseRemainingTimeValue.Size = new Size(95, 21);
            lblCloseRemainingTimeValue.TabIndex = 6;
            lblCloseRemainingTimeValue.Text = "Not Enabled";
            // 
            // lblCloseRemainingTimeLabel
            // 
            lblCloseRemainingTimeLabel.AutoSize = true;
            lblCloseRemainingTimeLabel.Location = new Point(208, 68);
            lblCloseRemainingTimeLabel.Name = "lblCloseRemainingTimeLabel";
            lblCloseRemainingTimeLabel.Size = new Size(130, 21);
            lblCloseRemainingTimeLabel.TabIndex = 5;
            lblCloseRemainingTimeLabel.Text = "Remaining Time: ";
            // 
            // btnCloseWindow
            // 
            btnCloseWindow.Location = new Point(6, 26);
            btnCloseWindow.Margin = new Padding(3, 2, 3, 2);
            btnCloseWindow.Name = "btnCloseWindow";
            btnCloseWindow.Size = new Size(196, 30);
            btnCloseWindow.TabIndex = 0;
            btnCloseWindow.Text = "Schedule Close Window";
            btnCloseWindow.UseVisualStyleBackColor = true;
            btnCloseWindow.Click += BtnCloseWindow_Click;
            // 
            // lblProcessNameLabel
            // 
            lblProcessNameLabel.AutoSize = true;
            lblProcessNameLabel.Location = new Point(445, 31);
            lblProcessNameLabel.Name = "lblProcessNameLabel";
            lblProcessNameLabel.Size = new Size(66, 21);
            lblProcessNameLabel.TabIndex = 7;
            lblProcessNameLabel.Text = "Process:";
            // 
            // txtProcessName
            // 
            txtProcessName.Location = new Point(517, 26);
            txtProcessName.Name = "txtProcessName";
            txtProcessName.ReadOnly = true;
            txtProcessName.Size = new Size(237, 29);
            txtProcessName.TabIndex = 8;
            // 
            // lblWindowHandleLabel
            // 
            lblWindowHandleLabel.AutoSize = true;
            lblWindowHandleLabel.Location = new Point(445, 68);
            lblWindowHandleLabel.Name = "lblWindowHandleLabel";
            lblWindowHandleLabel.Size = new Size(62, 21);
            lblWindowHandleLabel.TabIndex = 9;
            lblWindowHandleLabel.Text = "Handle:";
            // 
            // txtWindowHandle
            // 
            txtWindowHandle.Location = new Point(517, 63);
            txtWindowHandle.Name = "txtWindowHandle";
            txtWindowHandle.ReadOnly = true;
            txtWindowHandle.Size = new Size(237, 29);
            txtWindowHandle.TabIndex = 10;
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
            ClientSize = new Size(784, 461);
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
        private System.Windows.Forms.Label lblSleepRemainingTimeValue;
        private System.Windows.Forms.GroupBox grpClose;
        private System.Windows.Forms.Button btnCloseWindow;
        private System.Windows.Forms.ListBox lstWindows;
        private System.Windows.Forms.ComboBox cmbCloseDuration;
        private System.Windows.Forms.Label lblCloseDuration;
        private System.Windows.Forms.Button btnRefresh;
        private System.Windows.Forms.Timer timer1;
        private Label lblSleepRemainingTimeLabel;
        private Label lblCloseRemainingTimeLabel;
        private Label lblCloseRemainingTimeValue;
        private Label lblProcessNameLabel;
        private TextBox txtProcessName;
        private Label lblWindowHandleLabel;
        private TextBox txtWindowHandle;
    }
}
