from pathlib import Path


def file_to_c_array(filepath, array_name):
    with open(filepath, "rb") as f:
        content = f.read()

    hex_bytes = [f"0x{b:02x}" for b in content]

    # 每行 16 個位元組
    lines = []
    for i in range(0, len(hex_bytes), 16):
        lines.append(", ".join(hex_bytes[i : i + 16]))

    c_array_str = (
        "#pragma once\n"
        + f"const unsigned char {array_name}[] = {{\n  "
        + ",\n  ".join(lines)
        + f"\n}};\nconst unsigned int {array_name}_len = {len(content)};\n"
    )
    return c_array_str


if __name__ == "__main__":
    root = Path(__file__).parent.parent
    _7z = root / "dict" / "opencc.7z"
    if not _7z.exists():
        print(f"File not found: {_7z}")
        exit(1)
    output = file_to_c_array(_7z, "opencc_data")
    with open(root / "src" / "opencc_data.h", "w") as f:
        f.write(output)
