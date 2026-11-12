"""Bundled headless backend. The visible application is Electron."""
import sys
if sys.argv[1:2] == ['--update-worker']:
    from builder.update_worker import main
    raise SystemExit(main(sys.argv[2:]))
from builder.desktop_service import main
raise SystemExit(main())
