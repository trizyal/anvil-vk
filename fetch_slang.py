import os
import sys
import urllib.request
import zipfile
import shutil
import platform

# --- Configuration ---
# Update this string to pull a newer version in the future
SLANG_VERSION = "2026.13"

# Set up paths relative to this script
PROJECT_ROOT = os.path.dirname(os.path.abspath(__file__))
EXTERNAL_DIR = os.path.join(PROJECT_ROOT, "external")
SLANG_DIR = os.path.join(EXTERNAL_DIR, "slang")
ZIP_PATH = os.path.join(EXTERNAL_DIR, "slang_download.zip")

def get_download_url():
    system = platform.system().lower()

    if system == "windows":
        filename = f"slang-{SLANG_VERSION}-windows-x86_64.zip"
    else:
        print(f"Error: Unsupported operating system '{system}'.")
        sys.exit(1)

    return f"https://github.com/shader-slang/slang/releases/download/v{SLANG_VERSION}/{filename}"

def download_progress(count, block_size, total_size):
    # Quick progress bar for the console
    if total_size > 0:
        percent = int(count * block_size * 100 / total_size)
        sys.stdout.write(f"\rDownloading Slang v{SLANG_VERSION}... {min(percent, 100)}%")
        sys.stdout.flush()

def main():
    print("--- Slang Dependency Fetcher ---")
    url = get_download_url()

    # 1. Ensure external directory exists
    os.makedirs(EXTERNAL_DIR, exist_ok=True)

    # 2. Clean up existing Slang folder to prevent version collisions
    if os.path.exists(SLANG_DIR):
        print("Cleaning up old Slang directory...")
        shutil.rmtree(SLANG_DIR)

    # 3. Download the ZIP
    try:
        print(f"Fetching from: {url}")
        urllib.request.urlretrieve(url, ZIP_PATH, reporthook=download_progress)
        print("\nDownload complete.")
    except Exception as e:
        print(f"\nError downloading file. Please check your internet connection or the version number: {e}")
        sys.exit(1)

    # 4. Extract the ZIP
    try:
        print(f"Extracting to {SLANG_DIR}...")
        with zipfile.ZipFile(ZIP_PATH, 'r') as zip_ref:
            zip_ref.extractall(SLANG_DIR)
        print("Extraction complete.")
    except Exception as e:
        print(f"Error extracting file: {e}")
        sys.exit(1)
    finally:
        # 5. Always clean up the downloaded ZIP
        if os.path.exists(ZIP_PATH):
            os.remove(ZIP_PATH)

    print("Success! Slang has been installed to your external/slang directory.")

if __name__ == "__main__":
    main()