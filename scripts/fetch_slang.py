import os
import sys
import urllib.request
import zipfile
import shutil
import platform

# --- Configuration ---
# Update this string to pull a newer version in the future
SLANG_VERSION = "2026.18.2"

# Set up paths relative to this script (now located in root/scripts)
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR) # Steps back up to the project root
EXTERNAL_DIR = os.path.join(PROJECT_ROOT, "external")

SLANG_DIR = os.path.join(EXTERNAL_DIR, "slang")

def get_platform_key():
    system = platform.system().lower()
    if system == "windows":
        return "windows"
    elif system == "linux":
        return "linux"
    else:
        print(f"Error: Unsupported operating system '{system}'.")
        sys.exit(1)
        return None

def get_download_url():
    system = get_platform_key();
    filename = f"slang-{SLANG_VERSION}-{system}-x86_64.zip"

    return f"https://github.com/shader-slang/slang/releases/download/v{SLANG_VERSION}/{filename}"

def download_progress(count, block_size, total_size):
    # Quick progress bar for the console
    if total_size > 0:
        percent = int(count * block_size * 100 / total_size)
        sys.stdout.write(f"\rDownloading Slang v{SLANG_VERSION}... {min(percent, 100)}%")
        sys.stdout.flush()

def main():
    print("--- Slang Dependency Fetcher ---")
    system = get_platform_key()

    # Platform-specific subdirectory to prevent overwriting
    slang_dir = os.path.join(SLANG_DIR, system)
    zip_path = os.path.join(SLANG_DIR, f"slang_download{system}.zip")


    # Ensure external directory exists
    os.makedirs(EXTERNAL_DIR, exist_ok=True)

    # Clean up existing Slang folder to prevent version collisions
    if os.path.exists(slang_dir):
        print("Cleaning up old Slang directory for {system}...")
        shutil.rmtree(slang_dir)

    # Download the ZIP
    url = get_download_url()
    try:
        print(f"Fetching from: {url}")
        urllib.request.urlretrieve(url, zip_path, reporthook=download_progress)
        print("\nDownload complete.")
    except Exception as e:
        print(f"\nError downloading file. Please check your internet connection or the version number: {e}")
        sys.exit(1)

    # Extract the ZIP
    try:
        print(f"Extracting to {slang_dir}...")
        with zipfile.ZipFile(zip_path, 'r') as zip_ref:
            zip_ref.extractall(slang_dir)
        print("Extraction complete.")
    except Exception as e:
        print(f"Error extracting file: {e}")
        sys.exit(1)
    finally:
        # Always clean up the downloaded ZIP
        if os.path.exists(zip_path):
            os.remove(zip_path)

    print(f"Success! Slang has been installed to your external/slang/{system} directory.")

if __name__ == "__main__":
    main()
