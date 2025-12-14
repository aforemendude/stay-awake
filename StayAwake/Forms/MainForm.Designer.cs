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
            lblRemainingTime = new Label();
            lblSleepDuration = new Label();
            cmbSleepDuration = new ComboBox();
            btnStayAwake = new Button();
            grpKill = new GroupBox();
            btnRefresh = new Button();
            lblKillDuration = new Label();
            cmbKillDuration = new ComboBox();
            lblProcess = new Label();
            cmbProcesses = new ComboBox();
            chkKillApp = new CheckBox();
            statusStrip = new StatusStrip();
            lblStatus = new ToolStripStatusLabel();
            timer1 = new System.Windows.Forms.Timer(components);
            grpSleep.SuspendLayout();
            grpKill.SuspendLayout();
            statusStrip.SuspendLayout();
            SuspendLayout();
            // 
            // grpSleep
            // 
            grpSleep.Controls.Add(lblRemainingTime);
            grpSleep.Controls.Add(lblSleepDuration);
            grpSleep.Controls.Add(cmbSleepDuration);
            grpSleep.Controls.Add(btnStayAwake);
            grpSleep.Font = new Font("Segoe UI", 12F);
            grpSleep.Location = new Point(12, 11);
            grpSleep.Margin = new Padding(3, 2, 3, 2);
            grpSleep.Name = "grpSleep";
            grpSleep.Padding = new Padding(3, 2, 3, 2);
            grpSleep.Size = new Size(460, 75);
            grpSleep.TabIndex = 0;
            grpSleep.TabStop = false;
            grpSleep.Text = "Sleep Prevention";
            // 
            // lblRemainingTime
            // 
            lblRemainingTime.AutoSize = true;
            lblRemainingTime.Location = new Point(121, 31);
            lblRemainingTime.Name = "lblRemainingTime";
            lblRemainingTime.Size = new Size(70, 21);
            lblRemainingTime.TabIndex = 2;
            lblRemainingTime.Text = "00:00:00";
            lblRemainingTime.Visible = false;
            // 
            // lblSleepDuration
            // 
            lblSleepDuration.AutoSize = true;
            lblSleepDuration.Location = new Point(117, 31);
            lblSleepDuration.Name = "lblSleepDuration";
            lblSleepDuration.Size = new Size(74, 21);
            lblSleepDuration.TabIndex = 3;
            lblSleepDuration.Text = "Duration:";
            // 
            // cmbSleepDuration
            // 
            cmbSleepDuration.DropDownStyle = ComboBoxStyle.DropDownList;
            cmbSleepDuration.FormattingEnabled = true;
            cmbSleepDuration.Location = new Point(197, 28);
            cmbSleepDuration.Margin = new Padding(3, 2, 3, 2);
            cmbSleepDuration.Name = "cmbSleepDuration";
            cmbSleepDuration.Size = new Size(257, 29);
            cmbSleepDuration.TabIndex = 1;
            // 
            // btnStayAwake
            // 
            btnStayAwake.Location = new Point(6, 26);
            btnStayAwake.Margin = new Padding(3, 2, 3, 2);
            btnStayAwake.Name = "btnStayAwake";
            btnStayAwake.Size = new Size(105, 30);
            btnStayAwake.TabIndex = 0;
            btnStayAwake.Text = "Stay Awake";
            btnStayAwake.UseVisualStyleBackColor = true;
            // 
            // grpKill
            // 
            grpKill.Controls.Add(btnRefresh);
            grpKill.Controls.Add(lblKillDuration);
            grpKill.Controls.Add(cmbKillDuration);
            grpKill.Controls.Add(lblProcess);
            grpKill.Controls.Add(cmbProcesses);
            grpKill.Controls.Add(chkKillApp);
            grpKill.Font = new Font("Segoe UI", 12F);
            grpKill.Location = new Point(12, 90);
            grpKill.Margin = new Padding(3, 2, 3, 2);
            grpKill.Name = "grpKill";
            grpKill.Padding = new Padding(3, 2, 3, 2);
            grpKill.Size = new Size(460, 247);
            grpKill.TabIndex = 1;
            grpKill.TabStop = false;
            grpKill.Text = "Process Terminator";
            // 
            // btnRefresh
            // 
            btnRefresh.Location = new Point(315, 50);
            btnRefresh.Margin = new Padding(3, 2, 3, 2);
            btnRefresh.Name = "btnRefresh";
            btnRefresh.Size = new Size(52, 23);
            btnRefresh.TabIndex = 2;
            btnRefresh.Text = "Ref";
            btnRefresh.UseVisualStyleBackColor = true;
            btnRefresh.Click += btnRefresh_Click;
            // 
            // lblKillDuration
            // 
            lblKillDuration.AutoSize = true;
            lblKillDuration.Location = new Point(18, 79);
            lblKillDuration.Name = "lblKillDuration";
            lblKillDuration.Size = new Size(47, 21);
            lblKillDuration.TabIndex = 3;
            lblKillDuration.Text = "After:";
            // 
            // cmbKillDuration
            // 
            cmbKillDuration.DropDownStyle = ComboBoxStyle.DropDownList;
            cmbKillDuration.FormattingEnabled = true;
            cmbKillDuration.Location = new Point(79, 76);
            cmbKillDuration.Margin = new Padding(3, 2, 3, 2);
            cmbKillDuration.Name = "cmbKillDuration";
            cmbKillDuration.Size = new Size(158, 29);
            cmbKillDuration.TabIndex = 3;
            // 
            // lblProcess
            // 
            lblProcess.AutoSize = true;
            lblProcess.Location = new Point(18, 52);
            lblProcess.Name = "lblProcess";
            lblProcess.Size = new Size(55, 21);
            lblProcess.TabIndex = 4;
            lblProcess.Text = "Target:";
            // 
            // cmbProcesses
            // 
            cmbProcesses.DropDownStyle = ComboBoxStyle.DropDownList;
            cmbProcesses.FormattingEnabled = true;
            cmbProcesses.Location = new Point(79, 50);
            cmbProcesses.Margin = new Padding(3, 2, 3, 2);
            cmbProcesses.Name = "cmbProcesses";
            cmbProcesses.Size = new Size(228, 29);
            cmbProcesses.TabIndex = 1;
            // 
            // chkKillApp
            // 
            chkKillApp.AutoSize = true;
            chkKillApp.Location = new Point(18, 26);
            chkKillApp.Margin = new Padding(3, 2, 3, 2);
            chkKillApp.Name = "chkKillApp";
            chkKillApp.Size = new Size(107, 25);
            chkKillApp.TabIndex = 0;
            chkKillApp.Text = "Kill Process";
            chkKillApp.UseVisualStyleBackColor = true;
            // 
            // statusStrip
            // 
            statusStrip.Items.AddRange(new ToolStripItem[] { lblStatus });
            statusStrip.Location = new Point(0, 339);
            statusStrip.Name = "statusStrip";
            statusStrip.Padding = new Padding(1, 0, 12, 0);
            statusStrip.Size = new Size(484, 22);
            statusStrip.TabIndex = 2;
            statusStrip.Text = "statusStrip1";
            // 
            // lblStatus
            // 
            lblStatus.Name = "lblStatus";
            lblStatus.Size = new Size(39, 17);
            lblStatus.Text = "Ready";
            // 
            // timer1
            // 
            timer1.Interval = 1000;
            timer1.Tick += timer1_Tick;
            // 
            // MainForm
            // 
            AutoScaleDimensions = new SizeF(7F, 15F);
            AutoScaleMode = AutoScaleMode.Font;
            ClientSize = new Size(484, 361);
            Controls.Add(statusStrip);
            Controls.Add(grpKill);
            Controls.Add(grpSleep);
            FormBorderStyle = FormBorderStyle.FixedSingle;
            Margin = new Padding(3, 2, 3, 2);
            MaximizeBox = false;
            Name = "MainForm";
            Text = "Stay Awake";
            grpSleep.ResumeLayout(false);
            grpSleep.PerformLayout();
            grpKill.ResumeLayout(false);
            grpKill.PerformLayout();
            statusStrip.ResumeLayout(false);
            statusStrip.PerformLayout();
            ResumeLayout(false);
            PerformLayout();

        }

        private System.Windows.Forms.GroupBox grpSleep;
        private System.Windows.Forms.Button btnStayAwake;
        private System.Windows.Forms.ComboBox cmbSleepDuration;
        private System.Windows.Forms.Label lblSleepDuration;
        private System.Windows.Forms.Label lblRemainingTime;
        private System.Windows.Forms.GroupBox grpKill;
        private System.Windows.Forms.CheckBox chkKillApp;
        private System.Windows.Forms.ComboBox cmbProcesses;
        private System.Windows.Forms.Label lblProcess;
        private System.Windows.Forms.ComboBox cmbKillDuration;
        private System.Windows.Forms.Label lblKillDuration;
        private System.Windows.Forms.Button btnRefresh;
        private System.Windows.Forms.StatusStrip statusStrip;
        private System.Windows.Forms.ToolStripStatusLabel lblStatus;
        private System.Windows.Forms.Timer timer1;
    }
}
