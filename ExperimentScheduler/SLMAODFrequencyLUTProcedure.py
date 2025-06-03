import numpy as np
from ExperimentProcedure import *
from ExperimentProcedure import experiment_monitoring, analog_in_calibration_monitoring
from RydbergBeamMoveProcedure import move_beam_to_target, RYDBERG_BEAM_420_POSITION, RYDBERG_BEAM_1013_POSITION
from EthernetClient import EthernetClient
import time
import os
import re
import shutil


YEAR, MONTH, DAY = today()
exp = ExperimentProcedure()

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

def move_files(parent_dir, new_parent_dir, file_extension=".grid"):
    """
    Moves files with a given extension from all subdirectories of `parent_dir` into a specified new folder
    Args:
        parent_dir (str): Path to the directory containing folders to search.
        file_extension (str): Extension of files to move (e.g., '.npy').
        new_parent_dir (str): Path to the new directory to move files into.
    """
    os.makedirs(new_parent_dir, exist_ok=True)

    for filename in os.listdir(parent_dir):
        if filename.endswith(file_extension):
            src_path = os.path.join(parent_dir, filename)
            dst_path = os.path.join(new_parent_dir, filename)
            if os.path.isfile(src_path):
                if os.path.exists(dst_path):
                    print(f"Overwrite: {filename} already exists in {new_parent_dir}")
                shutil.move(src_path, dst_path)
                print(f"Moved {filename} to {new_parent_dir}")

def AOD_frequency_calibration(exp_idx=None, timeout_control = {'use':False, 'timeout':600}):
    config_name = "AOD_frequency_calibration.Config"
    script_name = "AOD_tweezerloading_statistics.mScript"

    config_path = exp.CONFIGURATION_DIR + config_name
    config_file = ConfigurationFile(config_path)

    config_file.modify_parameter("REPETITIONS", "Reps:", str(50))
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_variable("twz_intensity", constant_value = 6.5)

    # Setup experiment details
    YEAR, MONTH, DAY = today()
    name_prefix = "AOD-FREQUENCY-CALIBRATION"
    if exp_idx is None:
        data_file_path = f"{exp.DATA_FILE_LOCATION}{YEAR}/{MONTH}/{MONTH} {DAY}/Raw Data/"
        file_count, largest_idx = find_largest_file_number(directory=data_file_path, name_prefix=name_prefix)
        exp_idx = largest_idx+1
    exp_name = f"{name_prefix}-{exp_idx}"

    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.open_master_script("\\ExperimentAutomation\\" + script_name)
    exp.run_experiment(exp_name)

    # Monitor experiment status
    experiment_monitoring(exp=exp, timeout_control=timeout_control)

    # analysis grid
    window = [20,40,360,360]
    thresholds = 100
    binnings = np.linspace(50, 150, 201)
    gridShape = [7,7]
    neighborhood_size = 50
    threshold_findLocs = 10
    advanced_option = dict({"active":False, "image_threshold":110, "score_threshold":12})
    multi_points_option = dict({"active":True, "search_square":4, "num_points":12})
    
    # Analyze the data
    analysis_locs = da.DataAnalysis(year='2025', month='June', day='2', data_name='data_1', 
                                    window=window, thresholds=thresholds, binnings=binnings, 
                                    n_cluster_row=7, neighborhood_size=neighborhood_size, threshold_findLocs=threshold_findLocs, 
                                    advanced_option=advanced_option, multi_points_option=multi_points_option)
    # get AOD frequency mapping, hard coding the frequencies since they are unlikely to change
    dac0_freq = np.array([ 80.,  86.,  92.,  98., 104., 110., 116.])
    dac1_freq = np.array([ 76.,  82.,  88.,  94., 100., 106., 112.])
    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                        window=window, thresholds=thresholds, binnings=binnings, 
                        n_cluster_row=7,
                        annotate_title = exp_name, annotate_note=" ")
    data_analysis.getFrequencyCalibration(dac0_freq=dac0_freq, dac1_freq=dac1_freq, expected_grid_shape = gridShape,
                                          neighborhood_size=neighborhood_size, threshold_findLocs=threshold_findLocs, 
                                          advanced_option=advanced_option, multi_points_option=multi_points_option)
    
    return data_analysis.tform_camera_to_AOD


def SLM_frequency_LUT_generation(exp_idx = None, timeout_control = {'use':False, 'timeout':600}):
    config_name = "SLM_frequency_LUT_generation.Config"
    script_name = "SLM_tweezerloading_statistics.mScript"

    config_path = exp.CONFIGURATION_DIR + config_name
    config_file = ConfigurationFile(config_path)

    config_file.modify_parameter("REPETITIONS", "Reps:", str(50))
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_variable("twz_intensity", constant_value = 6.5)

    # Setup experiment details
    YEAR, MONTH, DAY = today()
    name_prefix = "SLM-FREQUENCY-LUT-GENERATION"
    if exp_idx is None:
        data_file_path = f"{exp.DATA_FILE_LOCATION}{YEAR}/{MONTH}/{MONTH} {DAY}/Raw Data/"
        file_count, largest_idx = find_largest_file_number(directory=data_file_path, name_prefix=name_prefix)
        exp_idx = largest_idx + 1
    exp_name = f"{name_prefix}-{exp_idx}"

    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.open_master_script("\\ExperimentAutomation\\" + script_name)
    exp.run_experiment(exp_name)

    # Monitor experiment status
    experiment_monitoring(exp=exp, timeout_control=timeout_control)

    # analysis grid
    window = [0,0,440,30]
    thresholds = 100
    binnings = np.linspace(50, 150, 201)
    gridShape = [1,37]
    neighborhood_size = 50
    threshold_findLocs = 10
    advanced_option = dict({"active":True, "image_threshold":100, "score_threshold":2})
    multi_points_option = dict({"active":True, "search_square":4, "num_points":12})
    
    # Analyze the data
    analysis_locs = da.DataAnalysis(year='2025', month='June', day='2', data_name='data_2', 
                                    window=window, thresholds=thresholds, binnings=binnings, 
                                    n_cluster_row=1, neighborhood_size=neighborhood_size, threshold_findLocs=threshold_findLocs, 
                                    advanced_option=advanced_option, multi_points_option=multi_points_option)
    # get SLM camera trap site camera position
    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                        window=window, thresholds=thresholds, binnings=binnings, 
                        n_cluster_row=1,
                        annotate_title = exp_name, annotate_note=" ")
    pts_Marana = \
        data_analysis.getAveragedAtomLocationOnCamera(expected_grid_shape = gridShape,
                                          neighborhood_size=neighborhood_size, threshold_findLocs=threshold_findLocs, 
                                          advanced_option=advanced_option, multi_points_option=multi_points_option)
    
    row,col,_ = pts_Marana.shape
    num_points=data_analysis.maximaLocs.shape[1]
    grid_file_name = f"atomgrid_{row}x{col}_{num_points}points_{YEAR}-{MONTH}-{DAY}"
    # file_count, largest_idx = find_largest_file_number(directory=data_file_path, name_prefix=grid_file_name, extension='.grid')
    grid_full_name = f"{grid_file_name}-{exp_idx}.grid"
    data_file_path = f"{exp.DATA_FILE_LOCATION}{YEAR}/{MONTH}/{MONTH} {DAY}/Raw Data/"
    grid_full_path = f"{data_file_path}{grid_full_name}"
    data_analysis.saveGridFile(file_name=grid_full_path)
    # move the grid file already in the GRID folder to archived to make space for the new grid file 
    move_files(parent_dir=exp.GRID_FILE_LOCATION, new_parent_dir=exp.ARCHIVED_GRID_FILE_LOCATION, file_extension=".grid")
    shutil.copy(grid_full_path, os.path.join(exp.GRID_FILE_LOCATION, grid_full_name))

    return pts_Marana, grid_full_name

def generate_LUT(tform_camera_to_AOD, pts_Marana_SLM, exp_idx = None):
    freqLUT = tform_camera_to_AOD(pts_Marana_SLM.reshape(-1,2))
    freqLUT = freqLUT.reshape(*pts_Marana_SLM.shape)

    # === Define paths ===
    row,col,_ = freqLUT.shape
    data_dir = os.path.join(exp.DATA_FILE_LOCATION, YEAR, MONTH, f"{MONTH} {DAY}", "Raw Data")
    lut_dir = exp.LUT_FILE_LOCATION
    archived_lut_dir = exp.ARCHIEVD_LUT_FILE_LOCATION

    # === Determine filenames ===
    if exp_idx is None:
        file_count, largest_idx = find_largest_file_number(directory=data_dir, name_prefix="freqLUT", extension=".npy")
        exp_idx = largest_idx+1
    timestamped_filename = f"freqLUT-{YEAR}-{MONTH}-{DAY}-{row}x{col}-{exp_idx}.npy"
    latest_lut_filename = "freqLUT.npy"

    # === Save freqLUT to data directory ===
    timestamped_path = os.path.join(data_dir, timestamped_filename)
    latest_path = os.path.join(data_dir, latest_lut_filename)

    np.save(timestamped_path, freqLUT)
    np.save(latest_path, freqLUT)

    print(f"Saved array to data folder: {timestamped_filename} and {latest_lut_filename}")

    # === Remove existing LUT file to prevent name collision ===
    target_lut_path = os.path.join(lut_dir, latest_lut_filename)
    if os.path.exists(target_lut_path):
        os.remove(target_lut_path)
        print(f"Deleted existing file: {target_lut_path}")

    # === Move files to their final locations ===
    shutil.move(latest_path, target_lut_path)
    shutil.copy(timestamped_path, os.path.join(archived_lut_dir, timestamped_filename))

    print(f"Moved {latest_lut_filename} to {target_lut_path}")
    print(f"Copied {timestamped_filename} to {archived_lut_dir}")


if __name__ == "__main__":
    YEAR, MONTH, DAY = today()
    data_file_path = f"{exp.DATA_FILE_LOCATION}{YEAR}/{MONTH}/{MONTH} {DAY}/Raw Data/"
    file_count, largest_idx = find_largest_file_number(directory=data_file_path, name_prefix="AOD-FREQUENCY-CALIBRATION")
    tform_camera_to_AOD = AOD_frequency_calibration(exp_idx=0)
    pts_Marana_SLM, grid_full_name = SLM_frequency_LUT_generation(exp_idx=0)
    generate_LUT(tform_camera_to_AOD, pts_Marana_SLM, exp_idx=0)
    print(grid_full_name)


    # print(repr(freqLUT))
    # np.save('./freqLUT.npy', freqLUT)