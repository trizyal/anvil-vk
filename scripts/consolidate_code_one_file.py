import os
import sys

# Resolve paths relative to this script's location (root/scripts)
script_dir = os.path.dirname(os.path.abspath(__file__))
source_dir = os.path.abspath(os.path.join(script_dir, '..', 'source'))
examples_dir = os.path.abspath(os.path.join(script_dir, '..', 'examples'))
output_dir = os.path.abspath(os.path.join(script_dir, 'output'))

# Default configuration
directories_to_walk = [source_dir]
output_filename = 'AnvilCodebase.txt'

# Check for command-line arguments
if len(sys.argv) > 1:
    arg = sys.argv[1]

    if arg in ('-all', '--all'):
        output_filename = 'AllExamplesAnvilCodebase.txt'
        print(f"Targeting source and all example projects under: {examples_dir}")
        if os.path.exists(examples_dir):
            directories_to_walk.append(examples_dir)
        else:
            print(f"Warning: Examples directory '{examples_dir}' not found.")
    else:
        project_name = arg
        project_dir = os.path.join(examples_dir, project_name)

        if os.path.exists(project_dir):
            directories_to_walk.append(project_dir)
            output_filename = f"{project_name}AnvilCodebase.txt"
            print(f"Targeting additional project directory: {project_dir}")
        else:
            print(f"Warning: Project directory '{project_dir}' not found. Skipping.")

# Ensure the output directory exists before creating the file
os.makedirs(output_dir, exist_ok=True)

# Save the output file directly in the scripts directory
output_file = os.path.abspath(os.path.join(output_dir, output_filename))

allowed_extensions = ('.cpp', '.h', '.slang')
# Folders to ignore so we don't grab third-party code
exclude_folders = {'cmake-build-debug', 'cmake-build-release', 'external'}

with open(output_file, 'w', encoding='utf-8') as outfile:
    # Iterate through all target directories (source + optional examples)
    for target_dir in directories_to_walk:
        for root, dirs, files in os.walk(target_dir):
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