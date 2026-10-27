#!/usr/bin/env python3
"""ROM setup, quest classification and native mixed-layout regression gate."""
from pathlib import Path
import argparse
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser();parser.add_argument('--sanitize',action='store_true');args=parser.parse_args()
for test in ['screen_behavior_test.py','launcher_behavior_test.py','classifier_behavior_test.py','async_progress_test.py']:
    subprocess.run([sys.executable,'-B',str(ROOT/'tests/rom_setup'/test)]+(['--sanitize'] if args.sanitize else []),check=True)
subprocess.run([sys.executable,'-B',str(ROOT/'tests/item_receipts/run_seed_settings_tests.py'),'--mq-layouts']+
               (['--sanitizers'] if args.sanitize else []),check=True)
