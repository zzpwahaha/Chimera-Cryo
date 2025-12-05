# below is a group of commonly used function for experiment automation
import os
import re
import shutil
from ExperimentProcedure import ConfigurationFile, ExperimentProcedure


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

def update_camera_image_dimension(config_file: ConfigurationFile, camera_image_dim: dict):
    config_file.modify_parameter("CAMERA_IMAGE_DIMENSIONS", "Left:", str(camera_image_dim['Left:']))
    config_file.modify_parameter("CAMERA_IMAGE_DIMENSIONS", "Right:", str(camera_image_dim['Right:']))
    config_file.modify_parameter("CAMERA_IMAGE_DIMENSIONS", "H-Bin:", str(camera_image_dim['H-Bin:']))
    config_file.modify_parameter("CAMERA_IMAGE_DIMENSIONS", "Bottom:", str(camera_image_dim['Bottom:']))
    config_file.modify_parameter("CAMERA_IMAGE_DIMENSIONS", "Top:", str(camera_image_dim['Top:']))
    config_file.modify_parameter("CAMERA_IMAGE_DIMENSIONS", "V-Bin:", str(camera_image_dim['V-Bin:']))

def shuttle_grid_files(grid_file_name: str):
    # move the grid file already in the GRID folder to archived to make space for the new grid file 
    move_files(parent_dir=ExperimentProcedure.GRID_FILE_LOCATION, new_parent_dir=ExperimentProcedure.ARCHIVED_GRID_FILE_LOCATION, file_extension=".grid")
    # move the required grid file in the archived folder to GRID folder 
    move_files(parent_dir=ExperimentProcedure.ARCHIVED_GRID_FILE_LOCATION, new_parent_dir=ExperimentProcedure.GRID_FILE_LOCATION, file_extension=".grid", file_name=grid_file_name, throw=True)


if __name__ == '__main__':

    # analysis grid for 5x7 grid - 20250922
    # window = [0, 0, 65, 40]
    # thresholds = 100
    # binnings = np.linspace(0, 240, 241)
    # analysis_locs = da.DataAnalysis(year='2025', month='September', day='18', data_name='data_18', 
    #                                 window=window, thresholds=thresholds, binnings=binnings)

    grid_file_name = 'atomgrid_5x7_8points_20251203_SLM'
    camera_image_dim = {'Left:':1026, 'Right:':1090, 'H-Bin:':1, 'Bottom:': 928, 'Top:': 967, 'V-Bin:': 1}
    tweezer_intensity_setpoint = 2.9 #V
    repetitions = 4


    # # analysis grid for 2x7 grid - 20250922
    # window = [0,0,90,20]
    # thresholds = 100
    # binnings = np.linspace(0, 240, 241)
    # analysis_locs = da.DataAnalysis(year='2025', month='September', day='19', data_name='data_13', 
    #                                 window=window, thresholds=thresholds, binnings=binnings)
    # grid_file_name = 'atomgrid_1x13_4points_2025-9-8'
    # camera_image_dim = {'Left:':971, 'Right:':1150, 'H-Bin:':2, 'Bottom:': 923, 'Top:': 962, 'V-Bin:': 2}
    # tweezer_intensity_setpoint = 1.22 #V
    # repetitions = 4


    # # analysis grid for 1x7 grid - 20250930
    # window = [0,0,90,20]
    # thresholds = 100
    # binnings = np.linspace(0, 240, 241)
    # analysis_locs = da.DataAnalysis(year='2025', month='September', day='30', data_name='data_19', 
    #                                 window=window, thresholds=thresholds, binnings=binnings)

    # grid_file_name = 'atomgrid_1x7_4points_2025-11-2'
    # camera_image_dim = {'Left:':971, 'Right:':1150, 'H-Bin:':2, 'Bottom:': 923, 'Top:': 962, 'V-Bin:': 2}
    # tweezer_intensity_setpoint = 0.61 #V
    # repetitions = 8


    # grid_file_name = 'atomgrid_7x11_5points_2025-11-7'
    # camera_image_dim = {'Left:':1026, 'Right:':1090, 'H-Bin:':1, 'Bottom:': 926, 'Top:': 965, 'V-Bin:': 1}
    # tweezer_intensity_setpoint = 4 #V
    # repetitions = 4

    # AOD
    # camera_image_dim = {'Left:':961, 'Right:':1105, 'H-Bin:':1, 'Bottom:': 880, 'Top:': 1015, 'V-Bin:': 1}
    # tweezer_intensity_setpoint = 3.75 #V
    # repetitions = 4


    # config_name = "420alignment_with_d1.Config"
    # config_path = ExperimentProcedure.CONFIGURATION_DIR + config_name
    config_name = "tweezerloading.Config"
    config_path = 'C:/Chimera/Chimera-Cryo/Configurations/CryoTweezerLoading/' + config_name
    config_file = ConfigurationFile(config_path)


    shuttle_grid_files(grid_file_name=grid_file_name)
    config_file.modify_parameter("DATA_ANALYSIS", "Grid File Name:", grid_file_name)
    update_camera_image_dimension(config_file=config_file, camera_image_dim=camera_image_dim)
    config_file.config_param.update_variable("slm_twz_intensity", constant_value = tweezer_intensity_setpoint)
    config_file.save()

    pass