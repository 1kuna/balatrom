using System;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using BalatROM.Setup;

// Visual half of SetupForm. The core partial owns the fields status, detail, start, cancel and bar,
// the Run lifecycle, cancellation, the duplicate-run gate and teardown. Everything here is look and copy.
// Written for C# 5 (no interpolation or expression-bodied members) to match the core file's compiler.
internal sealed partial class SetupForm : Form {
 // Tokens shared with the Electron renderer (style.css :root).
 static readonly Color Bg = Color.FromArgb(18, 22, 21);
 static readonly Color Line = Color.FromArgb(38, 45, 43);
 static readonly Color LineStrong = Color.FromArgb(53, 61, 59);
 static readonly Color Ink = Color.FromArgb(236, 238, 234);
 static readonly Color Muted = Color.FromArgb(142, 152, 147);
 static readonly Color Faint = Color.FromArgb(102, 112, 108);
 static readonly Color Accent = Color.FromArgb(23, 105, 192);
 static readonly Color AccentHover = Color.FromArgb(28, 116, 207);
 static readonly Color AccentPress = Color.FromArgb(19, 90, 166);
 static readonly Color AccentBusy = Color.FromArgb(29, 53, 80);
 static readonly Color BusyInk = Color.FromArgb(169, 187, 205);
 static readonly Color Fill = Color.FromArgb(61, 155, 255);
 static readonly Color Bad = Color.FromArgb(242, 118, 107);
 static readonly Color Card = Color.FromArgb(238, 233, 220);
 static readonly Color Pip = Color.FromArgb(232, 71, 60);
 static readonly Color FocusRing = Color.FromArgb(140, 196, 255);
 static readonly Color Well = Color.FromArgb(14, 18, 17);

 const string DownloadSize = "186 MB";
 const string IdleStatus = "First-time setup";
 const string IdleDetail = "Downloads up to " + DownloadSize + " of official runtimes not yet cached.\nVerified before use. No admin rights needed.";
 const string IdleMore = "Nothing downloads until you choose Set up & open.\nOfficial runtimes only, each checked with SHA-256 before use. Cached copies are reused.\nInstalls to %LOCALAPPDATA%\\BalatROM. Internet is needed only for this setup.";

 Mark mark; Meter meter; Label wordmark; LinkLabel more; Panel morePanel; TextBox moreText;
 Font baseFont, strongFont, statusFont, monoFont;
 string fullError = "", moreLabel = "Download details"; int collapsedHeight; bool keyboardCues;
 // Optional: the core may set this from its config before BuildUi(), one "publisher — package" per line.
 // Shown under "Download details" ahead of IdleMore. Left empty, the panel shows IdleMore only.
 string sources;

 void BuildUi() {
  SuspendLayout();
  AutoScaleDimensions = new SizeF(96F, 96F);
  AutoScaleMode = AutoScaleMode.Dpi;
  baseFont = new Font("Segoe UI", 9.75F);
  strongFont = new Font("Segoe UI Semibold", 9.75F);
  statusFont = new Font("Segoe UI Semibold", 12F);
  monoFont = new Font("Consolas", 9F);
  Text = "BalatROM setup";
  Font = baseFont;
  BackColor = Bg; ForeColor = Ink;
  ClientSize = new Size(460, 232);
  FormBorderStyle = FormBorderStyle.FixedDialog;
  MaximizeBox = false; MinimizeBox = true; ShowIcon = false;
  StartPosition = FormStartPosition.CenterScreen;

  mark = new Mark { Location = new Point(23, 20), Size = new Size(18, 22), BackColor = Bg };
  wordmark = new Label { Text = "BalatROM", Font = strongFont, ForeColor = Ink, BackColor = Bg, Location = new Point(46, 20), Size = new Size(200, 22), TextAlign = ContentAlignment.MiddleLeft, UseMnemonic = false };

  status.Text = IdleStatus; status.Font = statusFont; status.ForeColor = Ink; status.BackColor = Bg;
  status.Location = new Point(24, 58); status.Size = new Size(412, 26); status.AutoEllipsis = true; status.UseMnemonic = false;
  status.AccessibleName = "Setup status";

  detail.Text = IdleDetail; detail.Font = baseFont; detail.ForeColor = Muted; detail.BackColor = Bg;
  detail.Location = new Point(24, 86); detail.Size = new Size(412, 40); detail.AutoEllipsis = true; detail.UseMnemonic = false;
  detail.AccessibleName = "Setup detail";

  // The native bar can't take these colors under visual styles; it stays in the tree, hidden, for the core's bookkeeping.
  bar.Visible = false; bar.Location = new Point(24, 140); bar.Size = new Size(412, 4); bar.Style = ProgressBarStyle.Continuous;
  // Hidden until setup starts; its slot holds the "Download details" link before approval and "Show details" after an error.
  meter = new Meter { Location = new Point(24, 140), Size = new Size(412, 4), BackColor = Bg, AccessibleName = "Setup progress", Visible = false };

  more = new LinkLabel { Text = moreLabel, Font = baseFont, Location = new Point(21, 132), Size = new Size(160, 20), BackColor = Bg, Visible = true,
   LinkColor = Muted, ActiveLinkColor = Ink, VisitedLinkColor = Muted, LinkBehavior = LinkBehavior.HoverUnderline, UseMnemonic = false, TabIndex = 2 };
  more.LinkClicked += (s, e) => ToggleDetail();

  morePanel = new Panel { Location = new Point(24, 160), Size = new Size(412, 116), BackColor = Well, Padding = new Padding(10, 8, 4, 8), Visible = false };
  moreText = new TextBox { Multiline = true, ReadOnly = true, WordWrap = true, ScrollBars = ScrollBars.Vertical, BorderStyle = BorderStyle.None,
   BackColor = Well, ForeColor = Color.FromArgb(195, 202, 198), Font = baseFont, Dock = DockStyle.Fill, TabIndex = 3, AccessibleName = "Download details" };
  moreText.Text = ((String.IsNullOrEmpty(sources) ? "" : "Sources\n" + sources.Trim() + "\n\n") + IdleMore).Replace("\n", Environment.NewLine);
  morePanel.Controls.Add(moreText);
  morePanel.Paint += (s, e) => { using (var pen = new Pen(Line)) e.Graphics.DrawRectangle(pen, 0, 0, morePanel.Width - 1, morePanel.Height - 1); };

  start.Text = "Set up && open"; start.AccessibleName = "Set up and open BalatROM";
  start.Location = new Point(308, 176); start.Size = new Size(128, 32); start.Anchor = AnchorStyles.Bottom | AnchorStyles.Right; start.TabIndex = 0;
  cancel.Text = "Cancel"; cancel.AccessibleName = "Cancel setup";
  cancel.Location = new Point(208, 176); cancel.Size = new Size(92, 32); cancel.Anchor = AnchorStyles.Bottom | AnchorStyles.Right; cancel.TabIndex = 1;
  Skin(start, true); Skin(cancel, false);

  Controls.AddRange(new Control[] { mark, wordmark, status, detail, bar, meter, more, morePanel, cancel, start });
  AcceptButton = start; CancelButton = cancel;

  HandleCreated += (s, e) => UseDarkTitleBar();
  Disposed += (s, e) => { baseFont.Dispose(); strongFont.Dispose(); statusFont.Dispose(); monoFont.Dispose(); };
  ResumeLayout(false);
  PerformLayout();
 }

 void ShowBusyVisual() {
  start.Enabled = false;
  start.Text = "Setting up…"; start.AccessibleName = "Setting up";
  AcceptButton = null;
  cancel.Text = "Cancel"; cancel.AccessibleName = "Cancel setup";
  HideError();
  status.Text = "Starting…";
  meter.Visible = true; meter.Sweep();
  start.Invalidate();
 }

 void ShowRetryVisual() {
  meter.Clear();
  start.Text = "Retry setup"; start.AccessibleName = "Retry setup";
  start.Enabled = true;
  cancel.Text = "Close"; cancel.AccessibleName = "Close setup";
  AcceptButton = start;
  start.Invalidate();
 }

 void ShowProgressVisual(ProgressInfo p) {
  if (IsDisposed) return;
  if (!String.IsNullOrEmpty(p.Message)) status.Text = p.Message;
  status.ForeColor = Ink;
  double done = p.Completed, total = p.Total;
  meter.Visible = true;
  if (total > 0) {
   // A real total with nothing counted yet is unknown speed, not zero work: keep the sweep until bytes arrive.
   if (done > 0) meter.Set(done / total); else meter.Sweep();
   detail.Text = String.Format("{0:N1} of {1:N1} MB", done / 1000000.0, total / 1000000.0);
   bar.Style = ProgressBarStyle.Continuous; bar.Value = (int)Math.Max(0, Math.Min(100, 100.0 * done / total));
  } else {
   meter.Sweep();
   detail.Text = "";
  }
  meter.AccessibleDescription = status.Text;
 }

 void ShowErrorVisual(string title, string fullDetail) {
  meter.Clear(); meter.Visible = false;
  status.Text = String.IsNullOrEmpty(title) ? "Setup could not finish" : title;
  status.ForeColor = Bad;
  fullError = (fullDetail ?? "").Trim();
  string summary = FirstLine(fullError);
  detail.Text = summary;
  CollapseDetail();
  moreLabel = "Show details"; more.Text = moreLabel;
  moreText.Font = monoFont; moreText.AccessibleName = "Error details";
  moreText.Text = fullError.Replace("\r\n", "\n").Replace("\n", Environment.NewLine);
  more.Visible = fullError.Length > 0 && (fullError != summary || summary.Length > 110);
 }

 static string FirstLine(string text) {
  int cut = text.IndexOfAny(new[] { '\r', '\n' });
  return cut < 0 ? text : text.Substring(0, cut).Trim();
 }

 void HideError() {
  status.ForeColor = Ink;
  CollapseDetail();
  more.Visible = false;
  fullError = "";
 }

 void ToggleDetail() {
  if (morePanel.Visible) { CollapseDetail(); return; }
  if (collapsedHeight == 0) collapsedHeight = ClientSize.Height;
  int gap = LogicalToDeviceUnits(6);
  morePanel.Top = more.Bottom + gap;
  morePanel.Visible = true;
  ClientSize = new Size(ClientSize.Width, collapsedHeight + morePanel.Height + LogicalToDeviceUnits(12));
  more.Text = "Hide details";
 }

 void CollapseDetail() {
  if (more != null) more.Text = moreLabel;
  if (morePanel == null || !morePanel.Visible) return;
  morePanel.Visible = false;
  if (collapsedHeight > 0) ClientSize = new Size(ClientSize.Width, collapsedHeight);
 }

 // Keyboard focus rings appear once the keyboard is used, like native dialogs.
 protected override bool ProcessDialogKey(Keys keyData) {
  if (!keyboardCues) { keyboardCues = true; start.Invalidate(); cancel.Invalidate(); }
  return base.ProcessDialogKey(keyData);
 }

 void Skin(Button b, bool primary) {
  bool hot = false, down = false;
  b.FlatStyle = FlatStyle.Flat; b.FlatAppearance.BorderSize = 0; b.UseVisualStyleBackColor = false;
  b.BackColor = Bg; b.ForeColor = Ink; b.Font = strongFont;
  b.MouseEnter += (s, e) => { hot = true; b.Invalidate(); };
  b.MouseLeave += (s, e) => { hot = false; down = false; b.Invalidate(); };
  b.MouseDown += (s, e) => { if (e.Button == MouseButtons.Left) { down = true; b.Invalidate(); } };
  b.MouseUp += (s, e) => { down = false; b.Invalidate(); };
  b.GotFocus += (s, e) => b.Invalidate();
  b.LostFocus += (s, e) => b.Invalidate();
  b.EnabledChanged += (s, e) => b.Invalidate();
  b.TextChanged += (s, e) => b.Invalidate();
  b.Paint += (s, e) => PaintButton(b, e.Graphics, primary, hot && b.Enabled, down && b.Enabled, b.Focused && (keyboardCues || ShowFocusCues));
 }

 static void PaintButton(Button b, Graphics g, bool primary, bool hot, bool down, bool focused) {
  float s = b.DeviceDpi / 96F;
  g.SmoothingMode = SmoothingMode.AntiAlias;
  g.Clear(Bg);
  Color fill, edge, ink;
  if (primary) {
   fill = !b.Enabled ? AccentBusy : down ? AccentPress : hot ? AccentHover : Accent;
   edge = fill; ink = b.Enabled ? Color.White : BusyInk;
  } else {
   fill = down ? Color.FromArgb(37, 44, 43) : hot ? Color.FromArgb(31, 37, 36) : Bg;
   edge = LineStrong; ink = b.Enabled ? Ink : Faint;
  }
  var box = new RectangleF(0.5F, 0.5F, b.Width - 1F, b.Height - 1F);
  using (var path = Round(box, 7 * s))
  using (var brush = new SolidBrush(fill))
  using (var pen = new Pen(edge, 1F)) { g.FillPath(brush, path); g.DrawPath(pen, path); }
  if (focused) {
   float inset = 1.5F * s;
   using (var ring = Round(new RectangleF(inset, inset, b.Width - 2 * inset, b.Height - 2 * inset), 6 * s))
   using (var pen = new Pen(FocusRing, 2F * s)) g.DrawPath(pen, ring);
  }
  TextRenderer.DrawText(g, b.Text, b.Font, new Rectangle(0, 0, b.Width, b.Height), ink,
   TextFormatFlags.HorizontalCenter | TextFormatFlags.VerticalCenter | TextFormatFlags.SingleLine | TextFormatFlags.EndEllipsis | TextFormatFlags.HidePrefix);
 }

 static GraphicsPath Round(RectangleF r, float radius) {
  var path = new GraphicsPath();
  float d = Math.Min(radius * 2, Math.Min(r.Width, r.Height));
  if (d <= 0) { path.AddRectangle(r); return path; }
  path.AddArc(r.X, r.Y, d, d, 180, 90);
  path.AddArc(r.Right - d, r.Y, d, d, 270, 90);
  path.AddArc(r.Right - d, r.Bottom - d, d, d, 0, 90);
  path.AddArc(r.X, r.Bottom - d, d, d, 90, 90);
  path.CloseFigure();
  return path;
 }

 // Dark caption on Windows 10 20H1+ (attribute 20; 19 on older builds) and a matching caption color on Windows 11.
 // Purely cosmetic and failure-tolerant: older Windows keeps its default title bar.
 [DllImport("dwmapi.dll")] static extern int DwmSetWindowAttribute(IntPtr hwnd, int attribute, ref int value, int size);
 void UseDarkTitleBar() {
  try {
   int on = 1, caption = Bg.R | (Bg.G << 8) | (Bg.B << 16);
   if (DwmSetWindowAttribute(Handle, 20, ref on, 4) != 0) DwmSetWindowAttribute(Handle, 19, ref on, 4);
   DwmSetWindowAttribute(Handle, 35, ref caption, 4);
  } catch (DllNotFoundException) { } catch (EntryPointNotFoundException) { }
 }

 // The playing-card mark, drawn to match the renderer's .mark.
 sealed class Mark : Control {
  public Mark() {
   SetStyle(ControlStyles.UserPaint | ControlStyles.AllPaintingInWmPaint | ControlStyles.OptimizedDoubleBuffer | ControlStyles.ResizeRedraw, true);
   SetStyle(ControlStyles.Selectable, false);
   TabStop = false; AccessibleRole = AccessibleRole.Graphic; AccessibleName = "BalatROM";
  }
  protected override void OnPaint(PaintEventArgs e) {
   var g = e.Graphics; float s = Height / 22F;
   g.Clear(BackColor);
   g.SmoothingMode = SmoothingMode.AntiAlias;
   g.TranslateTransform(Width / 2F, Height / 2F);
   g.RotateTransform(-8F);
   using (var card = Round(new RectangleF(-6.5F * s, -9F * s, 13F * s, 18F * s), 2.5F * s))
   using (var brush = new SolidBrush(Card)) g.FillPath(brush, card);
   float r = 4.2F * s;
   using (var brush = new SolidBrush(Pip)) g.FillPolygon(brush, new[] { new PointF(0, -r), new PointF(r, 0), new PointF(0, r), new PointF(-r, 0) });
   g.ResetTransform();
  }
 }

 // Thin rounded bar. Moves only on reported counts, easing toward each real value; sweeps while the size of work is unknown.
 sealed class Meter : Control {
  readonly System.Windows.Forms.Timer timer = new System.Windows.Forms.Timer { Interval = 15 };
  readonly Stopwatch clock = Stopwatch.StartNew();
  double target, shown; bool sweeping; int announced = -1;
  public bool Sweeping { get { return sweeping; } }
  public double Target { get { return target; } }
  static bool Animate { get { return SystemInformation.UIEffectsEnabled; } }

  public Meter() {
   SetStyle(ControlStyles.UserPaint | ControlStyles.AllPaintingInWmPaint | ControlStyles.OptimizedDoubleBuffer | ControlStyles.ResizeRedraw, true);
   SetStyle(ControlStyles.Selectable, false);
   TabStop = false;
   timer.Tick += (s, e) => Step();
  }

  public void Set(double fraction) {
   fraction = Math.Max(0, Math.Min(1, fraction));
   bool wasSweeping = sweeping;
   sweeping = false; target = fraction;
   if (wasSweeping || fraction < shown || !Animate) shown = Animate && wasSweeping ? 0 : fraction;
   if (Animate && shown != target) timer.Start(); else timer.Stop();
   int percent = (int)(fraction * 100);
   if (percent != announced) { announced = percent; AccessibilityNotifyClients(AccessibleEvents.ValueChange, -1); }
   Invalidate();
  }

  public void Sweep() {
   if (!sweeping) { sweeping = true; announced = -1; clock.Restart(); AccessibilityNotifyClients(AccessibleEvents.ValueChange, -1); }
   if (Animate) timer.Start();
   Invalidate();
  }

  public void Clear() { sweeping = false; target = shown = 0; announced = -1; timer.Stop(); Invalidate(); }

  void Step() {
   if (!sweeping) {
    double gap = target - shown;
    if (Math.Abs(gap) < 0.002) { shown = target; timer.Stop(); } else shown += gap * 0.18;
   }
   Invalidate();
  }

  protected override void OnPaint(PaintEventArgs e) {
   var g = e.Graphics;
   g.Clear(BackColor);
   g.SmoothingMode = SmoothingMode.AntiAlias;
   float radius = Height / 2F;
   using (var track = Round(new RectangleF(0, 0, Width, Height), radius)) {
    using (var brush = new SolidBrush(Line)) g.FillPath(brush, track);
    g.SetClip(track);
   }
   if (sweeping) {
    if (Animate) {
     double t = (clock.ElapsedMilliseconds % 1300) / 1300.0;
     t = t < 0.5 ? 4 * t * t * t : 1 - Math.Pow(-2 * t + 2, 3) / 2;
     float segment = Width * 0.32F, x = (float)(-segment + (Width + segment) * t);
     using (var bar = Round(new RectangleF(x, 0, segment, Height), radius)) using (var brush = new SolidBrush(Fill)) g.FillPath(brush, bar);
    } else using (var brush = new SolidBrush(Color.FromArgb(90, Fill))) g.FillRectangle(brush, 0, 0, Width, Height);
   } else if (shown > 0) {
    float w = Math.Max(Height, (float)(Width * shown));
    using (var bar = Round(new RectangleF(0, 0, w, Height), radius)) using (var brush = new SolidBrush(Fill)) g.FillPath(brush, bar);
   }
   g.ResetClip();
  }

  protected override AccessibleObject CreateAccessibilityInstance() { return new MeterAccessible(this); }
  protected override void Dispose(bool disposing) { if (disposing) timer.Dispose(); base.Dispose(disposing); }

  sealed class MeterAccessible : ControlAccessibleObject {
   readonly Meter meter;
   public MeterAccessible(Meter owner) : base(owner) { meter = owner; }
   public override AccessibleRole Role { get { return AccessibleRole.ProgressBar; } }
   public override string Value { get { return meter.Sweeping ? "" : String.Format("{0:0}%", meter.Target * 100); } }
  }
 }
}
