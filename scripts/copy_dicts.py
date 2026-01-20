#  Copy OpenCC dictionary and config files.
#
#  Copyright (C) 2026  lkk9898969 <lkk9898969@gmail.com>
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <https://www.gnu.org/licenses/>.

import argparse
from pathlib import Path
import shutil


def main():
    parser = argparse.ArgumentParser(
        description="Copy OpenCC dictionary and config files."
    )
    parser.add_argument(
        "--compiled-dir",
        type=Path,
        required=True,
        help="Directory containing compiled .ocd2 files.",
    )
    parser.add_argument(
        "--config-dir",
        type=Path,
        required=True,
        help="Directory containing .json config files.",
    )
    parser.add_argument(
        "--dest-dir",
        type=Path,
        required=True,
        help="Destination directory to copy files to.",
    )
    args = parser.parse_args()

    args.dest_dir.mkdir(exist_ok=True, parents=True)

    print(f"Copying .ocd2 files from {args.compiled_dir} to {args.dest_dir}")
    for file in args.compiled_dir.glob("*.ocd2"):
        shutil.copy(file, args.dest_dir / file.name)

    print(f"Copying .json files from {args.config_dir} to {args.dest_dir}")
    for file in args.config_dir.glob("*.json"):
        shutil.copy(file, args.dest_dir / file.name)


if __name__ == "__main__":
    main()
