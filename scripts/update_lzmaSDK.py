import argparse
import shutil
import os
from pathlib import Path


def update_files(new_sdk_path: Path, target_dir: Path, extension: str):
    """將 new_sdk_path 中符合 extension 的檔案，覆蓋更新到 target_dir 中同名的檔案"""
    if not target_dir.exists():
        return

    # 取得目標目錄中現有的所有檔名 (例如: {'7zTypes.h', 'LzmaDec.h'})
    target_files = {f.name: f for f in target_dir.glob(f"*{extension}")}

    if not target_files:
        return

    # 遞迴搜尋新 SDK 目錄中符合副檔名的檔案 (避免檔案在子資料夾找不到)
    new_files = list(new_sdk_path.glob(f"**/C/*{extension}"))

    updated_count = 0
    for new_file in new_files:
        if new_file.name in target_files:
            dest_file = target_files[new_file.name]
            print(f"Updating {dest_file.name} -> {target_dir}")

            # 若目標檔案為唯讀，先賦予寫入權限再覆蓋
            try:
                os.chmod(dest_file, 0o666)
            except OSError:
                pass

            # 直接覆蓋目標檔案，不會影響到其他不需要更新的檔案
            shutil.copy2(new_file, dest_file)
            updated_count += 1

    print(
        f"Finished updating {target_dir.name}: {updated_count}/{len(target_files)} files updated."
    )


def main(new_sdk_path: Path):
    lzma_sdk_path = Path("deps/lzma_sdk")
    if not lzma_sdk_path.exists():
        print(
            "Error: lzma_sdk submodule not found. Please ensure it is initialized and updated."
        )
        return

    if not new_sdk_path.exists():
        print(f"Error: New SDK path '{new_sdk_path}' does not exist.")
        return

    # 更新 inc 目錄下的 .h 檔
    update_files(new_sdk_path, lzma_sdk_path / "inc", ".h")

    # 更新 src 目錄下的 .c 檔
    update_files(new_sdk_path, lzma_sdk_path / "src", ".c")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Update lzmaSDK submodule with new SDK files."
    )
    parser.add_argument(
        "new_sdk_path",
        type=Path,
        help="Path to the new lzmaSDK files. The directory should contain .h and .c files.",
    )
    args = parser.parse_args()
    main(Path(args.new_sdk_path))
