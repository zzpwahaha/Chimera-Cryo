# below is a group of commonly used function for experiment automation
import os
import re
import shutil
from ExperimentProcedure import ConfigurationFile, ExperimentProcedure


RT_BIAS_FIELD = {
    'pgc_bias_x': 0.9,
    'pgc_bias_y': -1.0,
    'pgc_bias_z': -0.05,

    'pgc_twz_bias_x': 0.8,
    'pgc_twz_bias_y': -1.1,
    'pgc_twz_bias_z': -0.05,

    'pgc_img_bias_x': 0.8,
    'pgc_img_bias_y': -1.1,
    'pgc_img_bias_z': 0.05,
}

CRYO_BIAS_FIELD = {
    'pgc_bias_x': 3.25,
    'pgc_bias_y': -1.75,
    'pgc_bias_z': -0.05,

    'pgc_twz_bias_x': 3.2,
    'pgc_twz_bias_y': -4.5,
    'pgc_twz_bias_z': 0.1,

    'pgc_img_bias_x': 3.25,
    'pgc_img_bias_y': -4.6,
    'pgc_img_bias_z': 0.11,
}


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

def update_imaging_cooling_bias_field(config_file: ConfigurationFile, fields = CRYO_BIAS_FIELD):
    for key,value in fields.items():
        config_file.config_param.update_variable(key, constant_value = value)


def shuttle_grid_files(grid_file_name: str):
    # move the grid file already in the GRID folder to archived to make space for the new grid file 
    move_files(parent_dir=ExperimentProcedure.GRID_FILE_LOCATION, new_parent_dir=ExperimentProcedure.ARCHIVED_GRID_FILE_LOCATION, file_extension=".grid")
    # move the required grid file in the archived folder to GRID folder 
    move_files(parent_dir=ExperimentProcedure.ARCHIVED_GRID_FILE_LOCATION, new_parent_dir=ExperimentProcedure.GRID_FILE_LOCATION, file_extension=".grid", file_name=grid_file_name, throw=True)


def set_2pic_da(config_file: ConfigurationFile):
    for _ in range(config_file.get_section("DATA_ANALYSIS").get_num_active_plots()):
        config_file.delete_plot_da(0)
    config_file.add_plot_da("Histogram-2Pic", 0)
    config_file.add_plot_da("Loadingrate-2Pic", 0)
    config_file.add_plot_da("Survival-2Pic", 0)
    config_file.modify_parameter("CAMERA_SETTINGS", "Andor Pics Per Rep:", 2)


def set_3pic_da(config_file: ConfigurationFile):
    for _ in range(config_file.get_section("DATA_ANALYSIS").get_num_active_plots()):
        config_file.delete_plot_da(0)
    config_file.add_plot_da("Histogram-3Pic-Pic0", 0)
    config_file.add_plot_da("Histogram-3Pic-Pic1", 0)
    config_file.add_plot_da("Loadingrate-3Pic", 0)
    config_file.add_plot_da("Survival-3Pic-Pic1-0", 0)
    config_file.add_plot_da("Survival-3Pic-Pic2-0", 0)
    config_file.add_plot_da("Survival-3Pic-Pic2-1", 0)
    config_file.modify_parameter("CAMERA_SETTINGS", "Andor Pics Per Rep:", 3)


def set_AWG_rabi(config_file: ConfigurationFile, exp: ExperimentProcedure):
    config_file.modify_parameter_AWG("AGILENT_AWG", 1, "Channel Mode:", "dc")
    config_file.modify_parameter_AWG("AGILENT_AWG", 2, "Channel Mode:", "dc")
    config_file.modify_parameter_AWG("SIGLENT_AWG", 1, "Channel Mode:", "dc")
    config_file.modify_parameter_AWG("SIGLENT_AWG", 2, "Channel Mode:", "square")
    config_file.modify_parameter_AWG("SIGLENT2_AWG", 1, "Channel Mode:", "dc")
    config_file.modify_parameter_AWG("SIGLENT2_AWG", 2, "Channel Mode:", "dc")
    config_file.modify_parameter_AWG("SIGLENT3_AWG", 1, "Channel Mode:", "script")
    config_file.modify_parameter_AWG("SIGLENT3_AWG", 2, "Channel Mode:", "dc")
    config_file.save()
    exp.open_configuration(config_file.file_path)
    for arbgen_name in exp.ARBGEN_NAMES:
        exp.setArbGen(arbgen_name)
    config_file.modify_parameter_AWG("AGILENT_AWG", 1, "Channel Mode:", "no_control")
    config_file.modify_parameter_AWG("AGILENT_AWG", 2, "Channel Mode:", "no_control")
    config_file.modify_parameter_AWG("SIGLENT_AWG", 1, "Channel Mode:", "no_control")
    config_file.modify_parameter_AWG("SIGLENT_AWG", 2, "Channel Mode:", "no_control")
    config_file.modify_parameter_AWG("SIGLENT2_AWG", 1, "Channel Mode:", "no_control")
    config_file.modify_parameter_AWG("SIGLENT2_AWG", 2, "Channel Mode:", "no_control")
    config_file.modify_parameter_AWG("SIGLENT3_AWG", 1, "Channel Mode:", "no_control")
    config_file.modify_parameter_AWG("SIGLENT3_AWG", 2, "Channel Mode:", "no_control")
    config_file.save()
    exp.open_configuration(config_file.file_path)


def set_AWG_avalanche(config_file: ConfigurationFile, exp: ExperimentProcedure):
    config_file.modify_parameter_AWG("AGILENT_AWG", 1, "Channel Mode:", "square")
    config_file.modify_parameter_AWG("AGILENT_AWG", 2, "Channel Mode:", "square")
    config_file.modify_parameter_AWG("SIGLENT_AWG", 1, "Channel Mode:", "square")
    config_file.modify_parameter_AWG("SIGLENT_AWG", 2, "Channel Mode:", "dc")
    config_file.modify_parameter_AWG("SIGLENT2_AWG", 1, "Channel Mode:", "square")
    config_file.modify_parameter_AWG("SIGLENT2_AWG", 2, "Channel Mode:", "square")
    config_file.modify_parameter_AWG("SIGLENT3_AWG", 1, "Channel Mode:", "script")
    config_file.modify_parameter_AWG("SIGLENT3_AWG", 2, "Channel Mode:", "dc")
    config_file.save()
    exp.open_configuration(config_file.file_path)
    for arbgen_name in exp.ARBGEN_NAMES:
        exp.setArbGen(arbgen_name)
    config_file.modify_parameter_AWG("AGILENT_AWG", 1, "Channel Mode:", "no_control")
    config_file.modify_parameter_AWG("AGILENT_AWG", 2, "Channel Mode:", "no_control")
    config_file.modify_parameter_AWG("SIGLENT_AWG", 1, "Channel Mode:", "no_control")
    config_file.modify_parameter_AWG("SIGLENT_AWG", 2, "Channel Mode:", "no_control")
    config_file.modify_parameter_AWG("SIGLENT2_AWG", 1, "Channel Mode:", "no_control")
    config_file.modify_parameter_AWG("SIGLENT2_AWG", 2, "Channel Mode:", "no_control")
    config_file.modify_parameter_AWG("SIGLENT3_AWG", 1, "Channel Mode:", "no_control")
    config_file.modify_parameter_AWG("SIGLENT3_AWG", 2, "Channel Mode:", "no_control")
    config_file.save()
    exp.open_configuration(config_file.file_path)


if __name__ == '__main__':

    # analysis grid for 5x7 grid - 20250922
    # window = [0, 0, 65, 40]
    # thresholds = 100
    # binnings = np.linspace(0, 240, 241)
    # analysis_locs = da.DataAnalysis(year='2025', month='September', day='18', data_name='data_18', 
    #                                 window=window, thresholds=thresholds, binnings=binnings)

    NUM_OF_PIC = 3

    grid_file_name = 'atomgrid_5x7_8points_20251203_SLM'
    camera_image_dim = {'Left:':1026, 'Right:':1090, 'H-Bin:':1, 'Bottom:': 928, 'Top:': 967, 'V-Bin:': 1}
    tweezer_intensity_setpoint = 2.9 #V
    repetitions = 4
    # AOD - 6x6_60umx60um
    # camera_image_dim = {'Left:':961, 'Right:':1105, 'H-Bin:':1, 'Bottom:': 880, 'Top:': 1015, 'V-Bin:': 1}
    # tweezer_intensity_setpoint = 2.9 # 3.75 V for AOD
    # repetitions = 4

    grid_file_name = 'atomgrid_5x20_6points_20260123_SLM'
    camera_image_dim = {'Left:':971, 'Right:':1140, 'H-Bin:':1, 'Bottom:': 921, 'Top:': 974, 'V-Bin:': 1}
    tweezer_intensity_setpoint = 8.7 #8 #6.5 #V
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


    # analysis grid for 1x7 grid - 20250930
    # window = [0,0,90,20]
    # thresholds = 100
    # binnings = np.linspace(0, 240, 241)
    # analysis_locs = da.DataAnalysis(year='2025', month='September', day='30', data_name='data_19', 
    #                                 window=window, thresholds=thresholds, binnings=binnings)

    # grid_file_name = 'atomgrid_1x7_4points_2025-11-2'
    # camera_image_dim = {'Left:':971, 'Right:':1150, 'H-Bin:':2, 'Bottom:': 925, 'Top:': 964, 'V-Bin:': 2}
    # tweezer_intensity_setpoint = 0.61 #V
    # repetitions = 8

    # grid_file_name = 'atomgrid_1x4_3points_20260104_SLM'
    # camera_image_dim = {'Left:':971, 'Right:':1150, 'H-Bin:':2, 'Bottom:': 925, 'Top:': 964, 'V-Bin:': 2}
    # tweezer_intensity_setpoint = 0.42 #V
    # repetitions = 8

    # grid_file_name = 'atomgrid_1x1_5points_20260105_SLM'
    # camera_image_dim = {'Left:':1045, 'Right:':1070, 'H-Bin:':1, 'Bottom:': 939, 'Top:': 960, 'V-Bin:': 1}
    # tweezer_intensity_setpoint = 0.59 #V
    # repetitions = 8

    # grid_file_name = 'atomgrid_7x11_5points_2025-11-7'
    # camera_image_dim = {'Left:':1026, 'Right:':1090, 'H-Bin:':1, 'Bottom:': 926, 'Top:': 965, 'V-Bin:': 1}
    # tweezer_intensity_setpoint = 4 #V
    # repetitions = 4



    # 1x37 SLM
    # grid_file_name = 'atomgrid_1x37_8points_20251205_SLM'
    # camera_image_dim = {'Left:':836, 'Right:':1275, 'H-Bin:':1, 'Bottom:': 941, 'Top:': 970, 'V-Bin:': 1}
    # tweezer_intensity_setpoint = 2.9 
    # repetitions = 4
    # AOD - 3x12_60umx180um
    # camera_image_dim = {'Left:':836, 'Right:':1275, 'H-Bin:':1, 'Bottom:': 881, 'Top:': 1030, 'V-Bin:': 1}
    # tweezer_intensity_setpoint = 2.9 # 3.5 V for AOD
    # repetitions = 4




    # config_name = "420alignment_with_d1.Config"
    # config_name = "1013alignment_with_d1.Config"
    # config_path = ExperimentProcedure.CONFIGURATION_DIR + config_name

    config_name = "tweezerloading.Config"
    config_path = 'C:/Chimera/Chimera-Cryo/Configurations/CryoTweezerLoading/' + config_name
    
    config_file = ConfigurationFile(config_path)


    shuttle_grid_files(grid_file_name=grid_file_name)
    config_file.modify_parameter("DATA_ANALYSIS", "Grid File Name:", grid_file_name)
    update_camera_image_dimension(config_file=config_file, camera_image_dim=camera_image_dim)
    config_file.config_param.update_variable("slm_twz_intensity", constant_value = tweezer_intensity_setpoint)
    
    if NUM_OF_PIC==2:
        set_2pic_da(config_file=config_file)
    elif NUM_OF_PIC==3:
        set_3pic_da(config_file=config_file)

    config_file.save()

    # update_imaging_cooling_bias_field(config_file=config_file,fields=CRYO_BIAS_FIELD)
    # config_file.save()

    pass