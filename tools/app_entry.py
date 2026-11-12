"""Legacy app entry point. The Qt GUI is retired; the released app is in desktop/."""
import sys
from builder.__main__ import main
raise SystemExit(main(sys.argv[1:] if sys.argv[1:2] == ['--update-worker'] else ['app']))
