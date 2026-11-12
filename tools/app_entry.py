"""Proposed installable app entry. Packaging and GUI qualification remain pending."""
import sys
from builder.__main__ import main
raise SystemExit(main(sys.argv[1:] if sys.argv[1:2] == ['--update-worker'] else ['app']))
