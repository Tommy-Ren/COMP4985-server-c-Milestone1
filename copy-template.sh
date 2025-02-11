#!/usr/bin/env bash

# Default values
dest_dir=""

# Function to display usage information
usage()
{
    echo "Usage: $0 <destination_directory>"
    exit 1
}

# Check if exactly one argument is provided
if [ "$#" -ne 1 ]; then
    usage
fi

dest_dir=$1

# Resolve absolute path of source directory
source_dir="$(realpath .)"

# Check if the destination directory exists; if not, create it
if [ ! -d "$dest_dir" ]; then
    mkdir -p "$dest_dir"
    echo "Created destination directory: $dest_dir"
fi

# List of files to link instead of copy
files_to_link=("sanitizers.txt" ".flags" "supported_c_compilers.txt")

for file in "${files_to_link[@]}"; do
    source_file="$source_dir/$file"
    dest_file="$dest_dir/$file"

    if [ -e "$source_file" ]; then
        if [ ! -L "$dest_file" ]; then
            ln -s "$source_file" "$dest_file"
            echo "Linked $file to $dest_dir"
        else
            echo "$file already linked in $dest_dir. Skipping."
        fi
    else
        echo "$file not found in the template directory. Skipping."
    fi
done

# List of files and directories to copy
files_to_copy=(".clang-format" ".gitignore" "build.sh" "build-all.sh" "change-compiler.sh" "check-compilers.sh" "check-env.sh" "create-links.sh" "move.sh" "files.txt" "generate-cmakelists.sh" "generate-flags.sh" "README.md" "src" "include")

# Copy files and directories to the destination directory
for item in "${files_to_copy[@]}"; do
    source_item="$source_dir/$item"
    dest_item="$dest_dir/$item"

    if [ -e "$source_item" ]; then
        if [ ! -e "$dest_item" ]; then
            cp -a "$source_item" "$dest_item"
            echo "Copied $item to $dest_dir"
        else
            echo "$item already exists in $dest_dir. Skipping."
        fi
    else
        echo "$item not found in the template directory. Skipping."
    fi
done

echo "Copy operation complete."

# Navigate to the destination directory
pushd "$dest_dir" || exit

# Check if .flags exists and run scripts if it doesn't
if [ ! -e ".flags" ]; then
    echo "Running check-compilers.sh and generate-flags.sh scripts."
    ./check-compilers.sh
    ./generate-flags.sh
else
    echo ".flags file already exists."
fi

# Return to the original directory
popd || exit
