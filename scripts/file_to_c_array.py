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
