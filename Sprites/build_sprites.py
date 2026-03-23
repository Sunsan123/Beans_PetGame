#!/usr/bin/env python3

import sys
from pathlib import Path


def main():
    repo_root = Path(__file__).resolve().parents[1]
    sys.path.insert(0, str(repo_root))
    from tools.asset_pipeline.build_assets import main as build_main

    build_main()


if __name__ == "__main__":
    main()
