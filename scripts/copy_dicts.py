from pathlib import Path
import shutil

if __name__ == "__main__":
    root = Path(__file__).parent.parent
    dict_ = root / "dict"
    dict_.mkdir(exist_ok=True)
    data = root / "OpenCC" / "build" / "data"
    configs = root / "OpenCC" / "data" / "config"
    for file in data.glob("*.ocd2"):
        shutil.copy(file, dict_ / file.name)
    for file in configs.glob("*.json"):
        shutil.copy(file, dict_ / file.name)
