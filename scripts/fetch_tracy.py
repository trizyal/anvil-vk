import os
import sys
import platform
import urllib.request
import zipfile
import shutil
from pathlib import Path

# The exact Tracy version to fetch.
TRACY_VERSION = "0.14.1"

# Directory configuration (adjusted for script living in root/scripts/)
PROJECT_ROOT = Path(__file__).parent.parent.absolute()
EXTERNAL_DIR = PROJECT_ROOT / "external" / "tracy"
TOOLS_DIR = PROJECT_ROOT / "tools" / "tracy"

def get_release_zip_name():
    system = platform.system().lower()
    if system == "windows":
        return f"windows-{TRACY_VERSION}.zip"
    elif system == "linux":
        return f"linux-{TRACY_VERSION}.zip"
    elif system == "darwin":
        return f"macos-{TRACY_VERSION}.zip"
    else:
        print(f"Unsupported OS: {system}")
        sys.exit(1)

def download_file(url, dest_path):
    print(f"Downloading {url}...")
    try:
        urllib.request.urlretrieve(url, dest_path)
        print("Download complete.")
    except Exception as e:
        print(f"Failed to download {url}: {e}")
        sys.exit(1)

def fetch_client_source():
    print("\n--- Fetching Tracy Client Source ---")
    source_url = f"https://github.com/wolfpld/tracy/archive/refs/tags/v{TRACY_VERSION}.zip"
    zip_path = PROJECT_ROOT / "tracy_source_temp.zip"

    download_file(source_url, zip_path)

    if EXTERNAL_DIR.exists():
        shutil.rmtree(EXTERNAL_DIR)
    EXTERNAL_DIR.mkdir(parents=True, exist_ok=True)

    print(f"Extracting client files to {EXTERNAL_DIR.relative_to(PROJECT_ROOT)}...")
    with zipfile.ZipFile(zip_path, 'r') as zf:
        prefix = f"tracy-{TRACY_VERSION}/public/"
        for file_info in zf.infolist():
            if file_info.filename.startswith(prefix) and not file_info.is_dir():
                relative_path = file_info.filename[len(prefix):]
                target_path = EXTERNAL_DIR / relative_path
                target_path.parent.mkdir(parents=True, exist_ok=True)
                with zf.open(file_info) as source, open(target_path, 'wb') as target:
                    shutil.copyfileobj(source, target)

    zip_path.unlink()
    print("Client source extraction complete.")

def fetch_server_executable():
    print("\n--- Fetching Tracy Server Binaries ---")
    zip_name = get_release_zip_name()
    release_url = f"https://github.com/wolfpld/tracy/releases/download/v{TRACY_VERSION}/{zip_name}"
    zip_path = PROJECT_ROOT / "tracy_release_temp.zip"

    download_file(release_url, zip_path)

    if TOOLS_DIR.exists():
        shutil.rmtree(TOOLS_DIR)
    TOOLS_DIR.mkdir(parents=True, exist_ok=True)

    print(f"Extracting all binaries to {TOOLS_DIR.relative_to(PROJECT_ROOT)}...")
    with zipfile.ZipFile(zip_path, 'r') as zf:
        for file_info in zf.infolist():
            if not file_info.is_dir():
                target_path = TOOLS_DIR / file_info.filename
                with zf.open(file_info) as source, open(target_path, 'wb') as target:
                    shutil.copyfileobj(source, target)

                if platform.system().lower() != "windows":
                    target_path.chmod(0o755)

    print("Extracted all binaries successfully.")
    zip_path.unlink()

if __name__ == "__main__":
    print(f"Setting up Tracy Profiler v{TRACY_VERSION}...")
    fetch_client_source()
    fetch_server_executable()
    print("\nTracy setup complete! You are ready to profile.")
