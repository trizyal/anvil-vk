import os

output_file = 'AnvilCodebase.txt'
allowed_extensions = ('.cpp', '.h', '.slang')
# Folders to ignore so we don't grab third-party code
exclude_folders = {'cmake-build-debug', 'cmake-build-release', 'external'}

with open(output_file, 'w', encoding='utf-8') as outfile:
    for root, dirs, files in os.walk('.'):
        # Remove excluded directories from the search
        dirs[:] = [d for d in dirs if d not in exclude_folders]

        for file in files:
            if file.endswith(allowed_extensions):
                filepath = os.path.join(root, file)

                # Write the header
                outfile.write(f"\n// {'=' * 42}\n")
                outfile.write(f"// {file}\n")
                outfile.write(f"// {'=' * 42}\n\n")

                # Write the source code
                try:
                    with open(filepath, 'r', encoding='utf-8') as infile:
                        outfile.write(infile.read() + '\n')
                except Exception as e:
                    outfile.write(f"// Could not read file: {e}\n")

print(f"Successfully combined files into {output_file}")