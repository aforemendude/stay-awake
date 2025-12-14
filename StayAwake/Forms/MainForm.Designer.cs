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
            this.components = new System.ComponentModel.Container();
            this.grpSleep = new System.Windows.Forms.GroupBox();
            this.btnStayAwake = new System.Windows.Forms.Button();
            this.cmbSleepDuration = new System.Windows.Forms.ComboBox();
            this.lblSleepDuration = new System.Windows.Forms.Label();
            this.lblRemainingTime = new System.Windows.Forms.Label();
            this.grpKill = new System.Windows.Forms.GroupBox();
            this.chkKillApp = new System.Windows.Forms.CheckBox();
            this.cmbProcesses = new System.Windows.Forms.ComboBox();
            this.lblProcess = new System.Windows.Forms.Label();
            this.cmbKillDuration = new System.Windows.Forms.ComboBox();
            this.lblKillDuration = new System.Windows.Forms.Label();
            this.btnRefresh = new System.Windows.Forms.Button();
            this.statusStrip = new System.Windows.Forms.StatusStrip();
            this.lblStatus = new System.Windows.Forms.ToolStripStatusLabel();
            this.timer1 = new System.Windows.Forms.Timer(this.components);

            this.grpSleep.SuspendLayout();
            this.grpKill.SuspendLayout();
            this.statusStrip.SuspendLayout();
            this.SuspendLayout();

            // grpSleep
            this.grpSleep.Controls.Add(this.lblRemainingTime);
            this.grpSleep.Controls.Add(this.lblSleepDuration);
            this.grpSleep.Controls.Add(this.cmbSleepDuration);
            this.grpSleep.Controls.Add(this.btnStayAwake);
            this.grpSleep.Location = new System.Drawing.Point(12, 12);
            this.grpSleep.Font = new System.Drawing.Font("Segoe UI", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point);
            this.grpSleep.Name = "grpSleep";
            this.grpSleep.Size = new System.Drawing.Size(460, 100);
            this.grpSleep.TabIndex = 0;
            this.grpSleep.TabStop = false;
            this.grpSleep.Text = "Sleep Prevention";

            // btnStayAwake
            this.btnStayAwake.Location = new System.Drawing.Point(20, 30);
            this.btnStayAwake.Name = "btnStayAwake";
            this.btnStayAwake.Size = new System.Drawing.Size(120, 40);
            this.btnStayAwake.TabIndex = 0;
            this.btnStayAwake.Text = "Stay Awake";
            this.btnStayAwake.UseVisualStyleBackColor = true;

            // lblRemainingTime
            this.lblRemainingTime.AutoSize = true;
            this.lblRemainingTime.Location = new System.Drawing.Point(160, 37);
            this.lblRemainingTime.Name = "lblRemainingTime";
            this.lblRemainingTime.Size = new System.Drawing.Size(71, 21);
            this.lblRemainingTime.TabIndex = 2;
            this.lblRemainingTime.Text = "00:00:00";
            this.lblRemainingTime.Visible = false;

            // lblSleepDuration
            this.lblSleepDuration.AutoSize = true;
            this.lblSleepDuration.Location = new System.Drawing.Point(160, 37);
            this.lblSleepDuration.Name = "lblSleepDuration";
            this.lblSleepDuration.Size = new System.Drawing.Size(76, 21);
            this.lblSleepDuration.Text = "Duration:";

            // cmbSleepDuration
            this.cmbSleepDuration.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cmbSleepDuration.FormattingEnabled = true;
            this.cmbSleepDuration.Location = new System.Drawing.Point(250, 34);
            this.cmbSleepDuration.Name = "cmbSleepDuration";
            this.cmbSleepDuration.Size = new System.Drawing.Size(180, 29);
            this.cmbSleepDuration.TabIndex = 1;

            // grpKill
            this.grpKill.Controls.Add(this.btnRefresh);
            this.grpKill.Controls.Add(this.lblKillDuration);
            this.grpKill.Controls.Add(this.cmbKillDuration);
            this.grpKill.Controls.Add(this.lblProcess);
            this.grpKill.Controls.Add(this.cmbProcesses);
            this.grpKill.Controls.Add(this.chkKillApp);
            // grpKill
            this.grpKill.Controls.Add(this.btnRefresh);
            this.grpKill.Controls.Add(this.lblKillDuration);
            this.grpKill.Controls.Add(this.cmbKillDuration);
            this.grpKill.Controls.Add(this.lblProcess);
            this.grpKill.Controls.Add(this.cmbProcesses);
            this.grpKill.Controls.Add(this.chkKillApp);
            this.grpKill.Location = new System.Drawing.Point(12, 120);
            this.grpKill.Name = "grpKill";
            this.grpKill.Size = new System.Drawing.Size(460, 140);
            this.grpKill.Font = new System.Drawing.Font("Segoe UI", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point);
            this.grpKill.TabIndex = 1;
            this.grpKill.TabStop = false;
            this.grpKill.Text = "Process Terminator";

            // chkKillApp
            this.chkKillApp.AutoSize = true;
            this.chkKillApp.Location = new System.Drawing.Point(20, 35);
            this.chkKillApp.Name = "chkKillApp";
            this.chkKillApp.Size = new System.Drawing.Size(106, 25);
            this.chkKillApp.TabIndex = 0;
            this.chkKillApp.Text = "Kill Process";
            this.chkKillApp.UseVisualStyleBackColor = true;

            // lblProcess
            this.lblProcess.AutoSize = true;
            this.lblProcess.Location = new System.Drawing.Point(20, 70);
            this.lblProcess.Name = "lblProcess";
            this.lblProcess.Size = new System.Drawing.Size(57, 21);
            this.lblProcess.Text = "Target:";

            // cmbProcesses
            this.cmbProcesses.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cmbProcesses.FormattingEnabled = true;
            // cmbProcesses
            this.cmbProcesses.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cmbProcesses.FormattingEnabled = true;
            this.cmbProcesses.Location = new System.Drawing.Point(90, 67);
            this.cmbProcesses.Name = "cmbProcesses";
            this.cmbProcesses.Size = new System.Drawing.Size(260, 29);
            this.cmbProcesses.TabIndex = 1;

            // btnRefresh
            // btnRefresh
            this.btnRefresh.Location = new System.Drawing.Point(360, 66);
            this.btnRefresh.Name = "btnRefresh";
            this.btnRefresh.Size = new System.Drawing.Size(60, 31);
            this.btnRefresh.TabIndex = 2;
            this.btnRefresh.Text = "Ref";
            this.btnRefresh.UseVisualStyleBackColor = true;
            this.btnRefresh.Click += new System.EventHandler(this.btnRefresh_Click);

            // lblKillDuration
            this.lblKillDuration.AutoSize = true;
            this.lblKillDuration.Location = new System.Drawing.Point(20, 105);
            this.lblKillDuration.Name = "lblKillDuration";
            this.lblKillDuration.Size = new System.Drawing.Size(48, 21);
            this.lblKillDuration.Text = "After:";

            // cmbKillDuration
            this.cmbKillDuration.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cmbKillDuration.FormattingEnabled = true;
            // cmbKillDuration
            this.cmbKillDuration.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cmbKillDuration.FormattingEnabled = true;
            this.cmbKillDuration.Location = new System.Drawing.Point(90, 102);
            this.cmbKillDuration.Name = "cmbKillDuration";
            this.cmbKillDuration.Size = new System.Drawing.Size(180, 29);
            this.cmbKillDuration.TabIndex = 3;

            // statusStrip
            this.statusStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.lblStatus});
            this.statusStrip.Location = new System.Drawing.Point(0, 285);
            this.statusStrip.Name = "statusStrip";
            this.statusStrip.Size = new System.Drawing.Size(484, 26);
            this.statusStrip.TabIndex = 2;
            this.statusStrip.Text = "statusStrip1";

            // lblStatus
            this.lblStatus.Name = "lblStatus";
            this.lblStatus.Size = new System.Drawing.Size(50, 20);
            this.lblStatus.Text = "Ready";

            // timer1
            this.timer1.Interval = 1000;
            this.timer1.Tick += new System.EventHandler(this.timer1_Tick);

            // MainForm
            this.AutoScaleDimensions = new System.Drawing.SizeF(8F, 20F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(484, 311);
            this.Controls.Add(this.statusStrip);
            this.Controls.Add(this.grpKill);
            this.Controls.Add(this.grpSleep);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.MaximizeBox = false;
            this.Name = "MainForm";
            this.Text = "Stay Awake";
            this.grpSleep.ResumeLayout(false);
            this.grpSleep.PerformLayout();
            this.grpKill.ResumeLayout(false);
            this.grpKill.PerformLayout();
            this.statusStrip.ResumeLayout(false);
            this.statusStrip.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

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
