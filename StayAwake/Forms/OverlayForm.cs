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
            Opacity = 0.3;
            DoubleBuffered = true;
        }

        protected override void OnPaint(PaintEventArgs e)
        {
            base.OnPaint(e);
            // Draw a border. Since background is Red, let's draw a DarkRed border.
            // Note: With Opacity, this will also receive the alpha value.
            int thickness = 5;
            using var pen = new Pen(Color.DarkRed, thickness);
            pen.Alignment = System.Drawing.Drawing2D.PenAlignment.Inset;
            e.Graphics.DrawRectangle(pen, 0, 0, Width, Height);
        }

        // Ensure it doesn't steal focus
        protected override bool ShowWithoutActivation => true;
    }
}
