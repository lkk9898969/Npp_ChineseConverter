import argparse
from pathlib import Path
import shutil

def main():
    parser = argparse.ArgumentParser(description="Copy OpenCC dictionary and config files.")
    parser.add_argument("--compiled-dir", type=Path, required=True, help="Directory containing compiled .ocd2 files.")
    parser.add_argument("--config-dir", type=Path, required=True, help="Directory containing .json config files.")
    parser.add_argument("--dest-dir", type=Path, required=True, help="Destination directory to copy files to.")
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