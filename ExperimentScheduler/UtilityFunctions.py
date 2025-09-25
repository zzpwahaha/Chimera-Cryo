# below is a group of commonly used function for experiment automation
import os
import re
import shutil

def find_largest_file_number(directory, name_prefix = "AOD-FREQUENCY-CALIBRATION", extension = ".h5"):
    # List all files in the given directory
    files = os.listdir(directory)
    # Regex pattern to match the filename "AOD-FREQUENCY-CALIBRATION-xx.h5"
    pattern = rf"{name_prefix}-(\d+){extension}"
    # Initialize a list to store the numeric parts (xx)
    numbers = []
    # Loop through files and check for the pattern
    for file in files:
        match = re.match(pattern, file)
        if match:
            # Extract the numeric part (xx)
            number = int(match.group(1))
            numbers.append(number)
    if numbers:
        # Find the largest xx
        largest_number = max(numbers)
        return len(numbers), largest_number
    else:
        return 0, 0-1

def move_files(parent_dir, new_parent_dir, file_extension=".grid", file_name=None, throw=False):
    """
    Move files with a given extension from a source directory into a target directory.

    This function either:
      - Moves all files in `parent_dir` that match `file_extension`, OR
      - Moves a specific file (`file_name` + `file_extension`) if `file_name` is provided.

    Args:
        parent_dir (str): Path to the directory to search for files.
        new_parent_dir (str): Path to the directory where files will be moved.
        file_extension (str, optional): File extension filter (default: ".grid").
        file_name (str, optional): Base name of the file (without extension). 
            If given, only that file will be moved.
        throw (bool, optional): If True, raises FileNotFoundError when `file_name` 
            is specified but not found. If False, logs a warning instead.

    Raises:
        FileNotFoundError: If `file_name` is specified, not found in `parent_dir`, 
            and `throw=True`.
    """
    os.makedirs(new_parent_dir, exist_ok=True)

    if file_name is None:
        # Move all matching files
        for filename in os.listdir(parent_dir):
            if filename.endswith(file_extension):
                src_path = os.path.join(parent_dir, filename)
                dst_path = os.path.join(new_parent_dir, filename)
                if os.path.isfile(src_path):
                    if os.path.exists(dst_path):
                        print(f"Overwrite: {filename} already exists in {new_parent_dir}")
                    shutil.move(src_path, dst_path)
                    print(f"Moved {filename} to {new_parent_dir}")
    else:
        # Move a specific file
        target_filename = file_name + file_extension
        src_path = os.path.join(parent_dir, target_filename)
        dst_path = os.path.join(new_parent_dir, target_filename)

        if not os.path.exists(src_path):
            msg = f"File not found: {target_filename} in {parent_dir}"
            if throw:
                raise FileNotFoundError(msg)
            else:
                print(msg + ". Ignoring this move.")
                return

        if os.path.isfile(src_path):
            if os.path.exists(dst_path):
                print(f"Overwrite: {target_filename} already exists in {new_parent_dir}")
            shutil.move(src_path, dst_path)
            print(f"Moved {target_filename} to {new_parent_dir}")