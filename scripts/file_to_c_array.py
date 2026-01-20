#  Convert a binary file to a C header array.
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


def file_to_c_array(filepath: Path, array_name: str):
    with open(filepath, "rb") as f:
        content = f.read()

    hex_bytes = [f"0x{b:02x}" for b in content]
    lines = [", ".join(hex_bytes[i : i + 16]) for i in range(0, len(hex_bytes), 16)]

    c_array_str = (
        "#pragma once\n\n"
        f"const unsigned char {array_name}[] = {{\n  "
        + ",\n  ".join(lines)
        + f"\n}};\n\nconst unsigned int {array_name}_len = {len(content)};\n"
    )
    return c_array_str


def main():
    parser = argparse.ArgumentParser(
        description="Convert a binary file to a C header array."
    )
    parser.add_argument(
        "--input",
        type=Path,
        required=True,
        help="Input binary file path (e.g., opencc.7z).",
    )
    parser.add_argument(
        "--output", type=Path, required=True, help="Output C header file path."
    )
    parser.add_argument(
        "--array-name",
        type=str,
        default="opencc_data",
        help="Name of the C array variable.",
    )
    args = parser.parse_args()

    if not args.input.exists():
        print(f"Error: Input file not found at {args.input}")
        exit(1)

    args.output.parent.mkdir(exist_ok=True, parents=True)

    c_code = file_to_c_array(args.input, args.array_name)

    with open(args.output, "w", encoding="utf-8") as f:
        f.write(c_code)

    print(f"Successfully generated header file at {args.output}")


if __name__ == "__main__":
    main()
