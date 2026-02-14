namespace StayAwake.Forms
{
    public class OverlayForm : Form
    {
        public OverlayForm()
        {
            FormBorderStyle = FormBorderStyle.None;
            StartPosition = FormStartPosition.Manual;
            ShowInTaskbar = false;
            TopMost = true;
            BackColor = Color.Red;
            Opacity = 0.25;
        }

        protected override CreateParams CreateParams
        {
            get
            {
                var cp = base.CreateParams;

                // WS_EX_TRANSPARENT (0x20) - Mouse clicks pass through
                cp.ExStyle |= 0x20;

                return cp;
            }
        }
    }
}
