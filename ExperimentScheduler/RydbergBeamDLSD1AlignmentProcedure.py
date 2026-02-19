import numpy as np
from ExperimentProcedure import *
from RydbergBeamMoveProcedure import move_beam_to_target
import UtilityFunctions as uf
import time


YEAR, MONTH, DAY = today()
exp = ExperimentProcedure()

# analysis grid for 5x7 grid - 20250922
window = [0, 0, 65, 40]
thresholds = 105
binnings = np.linspace(0, 240, 241)
analysis_locs = da.DataAnalysis(year='2025', month='November', day='17', data_name='data_2', 
                                window=window, thresholds=thresholds, binnings=binnings, multi_points_option = dict({"active":True, "search_square":2, "num_points":5}))
grid_file_name = 'atomgrid_5x7_5points_2025-11-17'
camera_image_dim = {'Left:':1026, 'Right:':1090, 'H-Bin:':1, 'Bottom:': 928, 'Top:': 967, 'V-Bin:': 1}
tweezer_intensity_setpoint = 2.9 #V
repetitions = 6


# analysis grid for 5x20 grid - 20251223
window = [0,0,170,54]
thresholds = 103
binnings = np.linspace(0, 240, 241)
analysis_locs = da.DataAnalysis(year='2026', month='February', day='2', data_name='data_4', 
                                window=window, thresholds=thresholds, binnings=binnings, multi_points_option = dict({"active":True, "search_square":4, "num_points":6}))
grid_file_name = 'atomgrid_5x20_6points_20260129_SLM'
camera_image_dim = {'Left:':971, 'Right:':1140, 'H-Bin:':1, 'Bottom:': 921, 'Top:': 974, 'V-Bin:': 1}
tweezer_intensity_setpoint = 8.7 #6.5 #V
repetitions = 3


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
# grid_file_name = 'atomgrid_1x7_4points_2025-9-30'
# camera_image_dim = {'Left:':971, 'Right:':1150, 'H-Bin:':2, 'Bottom:': 923, 'Top:': 962, 'V-Bin:': 2}
# tweezer_intensity_setpoint = 0.61 #V
# repetitions = 8



def update_camera_image_dimension(config_file: ConfigurationFile, camera_image_dim: dict):
    config_file.modify_parameter("CAMERA_IMAGE_DIMENSIONS", "Left:", str(camera_image_dim['Left:']))
    config_file.modify_parameter("CAMERA_IMAGE_DIMENSIONS", "Right:", str(camera_image_dim['Right:']))
    config_file.modify_parameter("CAMERA_IMAGE_DIMENSIONS", "H-Bin:", str(camera_image_dim['H-Bin:']))
    config_file.modify_parameter("CAMERA_IMAGE_DIMENSIONS", "Bottom:", str(camera_image_dim['Bottom:']))
    config_file.modify_parameter("CAMERA_IMAGE_DIMENSIONS", "Top:", str(camera_image_dim['Top:']))
    config_file.modify_parameter("CAMERA_IMAGE_DIMENSIONS", "V-Bin:", str(camera_image_dim['V-Bin:']))

def shuttle_grid_files(grid_file_name: str):
    # move the grid file already in the GRID folder to archived to make space for the new grid file 
    uf.move_files(parent_dir=exp.GRID_FILE_LOCATION, new_parent_dir=exp.ARCHIVED_GRID_FILE_LOCATION, file_extension=".grid")
    # move the required grid file in the archived folder to GRID folder 
    uf.move_files(parent_dir=exp.ARCHIVED_GRID_FILE_LOCATION, new_parent_dir=exp.GRID_FILE_LOCATION, file_extension=".grid", file_name=grid_file_name, throw=True)

def rydberg_420_lightshift_DLS_D1(exp_postfix: str, amplitude: float = 0.2, timeout_control = {'use':True, 'timeout':900}):
    exp_name_prefix = f"RYDBERG-LIGHTSHIFT-DLS-D1-420-{amplitude:.3f}"
    script_name = "rydberg_420_D1Ramanlightshift.mScript"

    config_name = "420alignment_with_d1.Config"
    config_path = exp.CONFIGURATION_DIR + config_name
    config_file = ConfigurationFile(config_path)

    # Update configuration
    config_file.modify_parameter("REPETITIONS", "Reps:", str(repetitions))
    config_file.modify_parameter("MAIN_OPTIONS", "Randomize Variations?", str(1))
    config_file.modify_parameter("MAIN_OPTIONS", "Repetition First Over Variation?", str(0))
    config_file.modify_parameter("MW1", "Control?", str(0))
    config_file.modify_parameter("MW2", "Control?", str(1))
    config_file.modify_parameter("MAKO3_CAM", "Mako System Active:", str(1))
    config_file.modify_parameter("MAKO4_CAM", "Mako System Active:", str(0))
    config_file.modify_parameter("DATA_ANALYSIS", "Grid File Name:", grid_file_name)
    update_camera_image_dimension(config_file=config_file, camera_image_dim=camera_image_dim)
    config_file.config_param.update_variable("slm_twz_intensity", constant_value = tweezer_intensity_setpoint)
    config_file.modify_parameter("PICOSCREW", "Control?", str(0))
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    # config_file.config_param.update_variable("d1_resonance", scan_type="Variable", new_initial_values=[14.5], new_final_values=[14.8])
    config_file.config_param.update_variable("d1_resonance", scan_type="Variable", new_initial_values=[7.10], new_final_values=[7.40])
    config_file.config_param.update_variable("amplitude_scan", constant_value = amplitude)
    config_file.config_param.update_variable("time_scan_us", constant_value = 17)
    config_file.config_param.update_scan_dimension(0, range_index=0, variations=31)

    config_file.save()

    # Setup experiment details
    YEAR, MONTH, DAY = today()
    exp_name = f"{exp_name_prefix}-{exp_postfix}" 
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.open_master_script("\\ExperimentAutomation\\" + script_name)
    
    start_and_stop_camera(exp)
    zeroScrews(exp)
    # _calibration(exp=exp, config_file=config_file)
    
    exp.run_experiment(exp_name)
    
    # Monitor experiment status
    experiment_monitoring(exp=exp, timeout_control=timeout_control)

    # Analyze the data
    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    analysis_result = data_analysis.analyze_data()
    optimal_field = analysis_result[1]
    vertical_pos, horizontal_pos = data_analysis.analyze_mako_data(mako_idx=3, function = da.gaussian)
    print(f"420 lightshift for {exp_name} is {optimal_field:.3S}  with"
          f" position ({vertical_pos.mean():.3S}, {horizontal_pos.mean():.3S})")

def rydberg_1013_lightshift_DLS_D1(exp_postfix: str, amplitude: float = 1.0, timeout_control = {'use':True, 'timeout':900}):
    exp_name_prefix = f"RYDBERG-LIGHTSHIFT-DLS-D1-1013-{amplitude:.3f}"
    script_name = "rydberg_1013_D1Ramanlightshift.mScript"

    config_name = "1013alignment_with_d1.Config"
    config_path = exp.CONFIGURATION_DIR + config_name
    config_file = ConfigurationFile(config_path)

    # Update configuration
    config_file.modify_parameter("REPETITIONS", "Reps:", str(repetitions))
    config_file.modify_parameter("MAIN_OPTIONS", "Randomize Variations?", str(1))
    config_file.modify_parameter("MAIN_OPTIONS", "Repetition First Over Variation?", str(0))
    config_file.modify_parameter("MW1", "Control?", str(0))
    config_file.modify_parameter("MW2", "Control?", str(1))
    config_file.modify_parameter("MAKO3_CAM", "Mako System Active:", str(0))
    config_file.modify_parameter("MAKO4_CAM", "Mako System Active:", str(1))
    config_file.modify_parameter("DATA_ANALYSIS", "Grid File Name:", grid_file_name)
    update_camera_image_dimension(config_file=config_file, camera_image_dim=camera_image_dim)
    config_file.config_param.update_variable("slm_twz_intensity", constant_value = tweezer_intensity_setpoint)
    config_file.modify_parameter("PICOSCREW", "Control?", str(0))
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    # config_file.config_param.update_variable("d1_resonance", scan_type="Variable", new_initial_values=[14.5], new_final_values=[14.8])
    config_file.config_param.update_variable("d1_resonance", scan_type="Variable", new_initial_values=[7.10], new_final_values=[7.40])
    config_file.config_param.update_variable("amplitude_scan", constant_value = amplitude)
    config_file.config_param.update_variable("time_scan_us", constant_value = 17)
    config_file.config_param.update_scan_dimension(0, range_index=0, variations=31)

    config_file.save()

    # Setup experiment details
    YEAR, MONTH, DAY = today()
    exp_name = f"{exp_name_prefix}-{exp_postfix}" 
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.open_master_script("\\ExperimentAutomation\\" + script_name)
    
    start_and_stop_camera(exp)
    zeroScrews(exp)

    # _calibration(exp=exp, config_file=config_file)
    
    exp.run_experiment(exp_name)
    
    # Monitor experiment status
    experiment_monitoring(exp=exp, timeout_control=timeout_control)

    # Analyze the data
    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    # analysis_result = data_analysis.analyze_data()
    # optimal_field = analysis_result[1]
    # vertical_pos, horizontal_pos = data_analysis.analyze_mako_data(mako_idx=4, function = da.gaussian)
    # print(f"1013 lightshift for {exp_name} is {optimal_field:.3S}  with"
    #       f" position ({vertical_pos.mean():.3S}, {horizontal_pos.mean():.3S})")

def rydberg_420_alignment_DLS(exp_postfix: str, pico_idx: int, amplitude: float = 0.2, timeout_control = {'use':True, 'timeout':3000}):
    NUM_POSITION_VAR = 11
    NUM_RESONANCE_VAR = 21
    if pico_idx not in [1,2]:
        raise ValueError("pico_idx out of the range. Ranges are " + str([1,2,3,4]))
    VERTICAL = 1; HORIZONTAL = 2;
    align_axis = -1
    if pico_idx==1:
        align_axis = VERTICAL
    else:
        align_axis = HORIZONTAL
    exp_name_prefix = "RYDBERG-ALIGNMENT-DLS-D1-420"
    script_name = "rydberg_420_D1Ramanlightshift.mScript"

    config_name = "420alignment_with_d1.Config"
    config_path = exp.CONFIGURATION_DIR + config_name
    config_file = ConfigurationFile(config_path)

    # # Update configuration
    config_file.modify_parameter("REPETITIONS", "Reps:", str(repetitions))
    config_file.modify_parameter("MAIN_OPTIONS", "Randomize Variations?", str(0))
    config_file.modify_parameter("MAIN_OPTIONS", "Repetition First Over Variation?", str(1))
    config_file.modify_parameter("MW1", "Control?", str(0))
    config_file.modify_parameter("MW2", "Control?", str(1))
    config_file.modify_parameter("MAKO3_CAM", "Mako System Active:", str(1))
    config_file.modify_parameter("MAKO4_CAM", "Mako System Active:", str(0))
    config_file.modify_parameter("DATA_ANALYSIS", "Grid File Name:", grid_file_name)
    update_camera_image_dimension(config_file=config_file, camera_image_dim=camera_image_dim)
    config_file.config_param.update_variable("slm_twz_intensity", constant_value = tweezer_intensity_setpoint)
    config_file.modify_parameter("PICOSCREW", "Control?", str(1))
    for idx in range(4):
        config_file.modify_parameter("PICOSCREW", f" Screw-{idx+1} Value:", "0")
    config_file.modify_parameter("PICOSCREW", f" Screw-{pico_idx} Value:", "integer_scan")
    
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_variable("integer_scan", scan_type="Variable", scan_dimension=0, new_initial_values=[-250], new_final_values=[250])
    # config_file.config_param.update_variable("d1_resonance", scan_type="Variable", scan_dimension=1, new_initial_values=[14.5], new_final_values=[14.8])
    config_file.config_param.update_variable("d1_resonance", scan_type="Variable", scan_dimension=1, new_initial_values=[7.10], new_final_values=[7.40])
    config_file.config_param.update_variable("amplitude_scan", constant_value = amplitude)
    config_file.config_param.update_variable("time_scan_us", constant_value = 17)
    config_file.config_param.update_scan_dimension(0, range_index=0, variations=NUM_POSITION_VAR)
    config_file.config_param.update_scan_dimension(1, range_index=0, variations=NUM_RESONANCE_VAR)
    config_file.save()

    exp.setPicoScrewHomes()

    # # Setup experiment details
    YEAR, MONTH, DAY = today()
    if align_axis==VERTICAL: axis_str = "VERTICAL"
    else: axis_str = "HORIZONTAL"
    exp_name = f"{exp_name_prefix}-{axis_str}-{exp_postfix}" 
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.open_master_script("\\ExperimentAutomation\\" + script_name)

    # _calibration(exp=exp, config_file=config_file)

    exp.run_experiment(exp_name)
    
    # # Monitor experiment status
    experiment_monitoring(exp=exp, timeout_control=timeout_control)

    # Analyze the data
    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    
    vertical_pos, horizontal_pos = data_analysis.analyze_mako_data(mako_idx=3, function = da.gaussian)
    if align_axis==VERTICAL: xkey = da.ah.nominal(vertical_pos).reshape(NUM_POSITION_VAR,NUM_RESONANCE_VAR).mean(axis=1)
    else: xkey = da.ah.nominal(horizontal_pos).reshape(NUM_POSITION_VAR,NUM_RESONANCE_VAR).mean(axis=1)
    analysis_result = data_analysis.analyze_data_2D(xkey0=xkey, function_d0=da.gaussian, function_d1=da.gaussian)
    optimal_field = analysis_result[1]
    
    if align_axis == VERTICAL:
        print_str = f"position ({optimal_field:.3S}, {horizontal_pos.mean():.3S})"
        target_position = (optimal_field.n, horizontal_pos.mean().n)
    else:
        print_str = f"position (vertical,horizontal) ({vertical_pos.mean():.3S}, {optimal_field:.3S})"
        target_position = (vertical_pos.mean().n, optimal_field.n)

    print(f"420 lightshift for {exp_name} is {optimal_field:.3S}  with " + print_str)
    screws_position = exp.getPicoScrewPositions()
    exp.setPicoScrewPosition(1,screws_position[0], update=False)
    exp.setPicoScrewPosition(2,screws_position[1], update=False)
    move_beam_to_target(exp, mako_idx=3, pico_idx=(1,2), target_position=target_position, tolerance=0.1)
    sleep(1)
    exp.setPicoScrewHomes()
    exp.setPicoScrewPosition(1,position=0, update=False)
    exp.setPicoScrewPosition(2,position=0, update=False)

def rydberg_1013_alignment_DLS(exp_postfix: str, pico_idx: int, amplitude: float = 1.0, timeout_control = {'use':True, 'timeout':3000}):
    NUM_POSITION_VAR = 11
    NUM_RESONANCE_VAR = 21
    if pico_idx not in [3,4]:
        raise ValueError("pico_idx out of the range. Ranges are " + str([1,2,3,4]))
    VERTICAL = 1; HORIZONTAL = 2;
    align_axis = -1
    if pico_idx==3:
        align_axis = VERTICAL
    else:
        align_axis = HORIZONTAL
    exp_name_prefix = "RYDBERG-ALIGNMENT-DLS-D1-1013"
    script_name = "rydberg_1013_D1Ramanlightshift.mScript"

    config_name = "1013alignment_with_d1.Config"
    config_path = exp.CONFIGURATION_DIR + config_name
    config_file = ConfigurationFile(config_path)

    # # Update configuration
    config_file.modify_parameter("REPETITIONS", "Reps:", str(repetitions))
    config_file.modify_parameter("MAIN_OPTIONS", "Randomize Variations?", str(0))
    config_file.modify_parameter("MAIN_OPTIONS", "Repetition First Over Variation?", str(1))
    config_file.modify_parameter("MW1", "Control?", str(0))
    config_file.modify_parameter("MW2", "Control?", str(1))
    config_file.modify_parameter("MAKO3_CAM", "Mako System Active:", str(0))
    config_file.modify_parameter("MAKO4_CAM", "Mako System Active:", str(1))
    config_file.modify_parameter("DATA_ANALYSIS", "Grid File Name:", grid_file_name)
    update_camera_image_dimension(config_file=config_file, camera_image_dim=camera_image_dim)
    config_file.config_param.update_variable("slm_twz_intensity", constant_value = tweezer_intensity_setpoint)
    config_file.modify_parameter("PICOSCREW", "Control?", str(1))
    for idx in range(4):
        config_file.modify_parameter("PICOSCREW", f" Screw-{idx+1} Value:", "0")
    config_file.modify_parameter("PICOSCREW", f" Screw-{pico_idx} Value:", "integer_scan")
    
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)
    if pico_idx==3: # vertical
        config_file.config_param.update_variable("integer_scan", scan_type="Variable", scan_dimension=0, new_initial_values=[750], new_final_values=[-750])
    else: # horizontal
        config_file.config_param.update_variable("integer_scan", scan_type="Variable", scan_dimension=0, new_initial_values=[-50], new_final_values=[50])
    # config_file.config_param.update_variable("d1_resonance", sssssssscan_type="Variable", scan_dimension=1, new_initial_values=[14.5], new_final_values=[14.8])
    config_file.config_param.update_variable("d1_resonance", scan_type="Variable", scan_dimension=1, new_initial_values=[7.10], new_final_values=[7.40])
    config_file.config_param.update_variable("amplitude_scan", constant_value = amplitude)
    config_file.config_param.update_variable("time_scan_us", constant_value = 17)
    config_file.config_param.update_scan_dimension(0, range_index=0, variations=NUM_POSITION_VAR)
    config_file.config_param.update_scan_dimension(1, range_index=0, variations=NUM_RESONANCE_VAR)
    config_file.save()

    exp.setPicoScrewHomes()

    # # Setup experiment details
    YEAR, MONTH, DAY = today()
    if align_axis==VERTICAL: axis_str = "VERTICAL"
    else: axis_str = "HORIZONTAL"
    exp_name = f"{exp_name_prefix}-{axis_str}-{exp_postfix}" 
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.open_master_script("\\ExperimentAutomation\\" + script_name)

    # _calibration(exp=exp, config_file=config_file)

    exp.run_experiment(exp_name)
    
    # Monitor experiment status
    experiment_monitoring(exp=exp, timeout_control=timeout_control)

    # Analyze the data
    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    
    vertical_pos, horizontal_pos = data_analysis.analyze_mako_data(mako_idx=4, function = da.gaussian)
    if align_axis==VERTICAL: xkey = da.ah.nominal(vertical_pos).reshape(NUM_POSITION_VAR,NUM_RESONANCE_VAR).mean(axis=1)
    else: xkey = da.ah.nominal(horizontal_pos).reshape(NUM_POSITION_VAR,NUM_RESONANCE_VAR).mean(axis=1)
    analysis_result = data_analysis.analyze_data_2D(xkey0=xkey, function_d0=da.gaussian, function_d1=da.gaussian)
    optimal_field = analysis_result[1]
    
    if align_axis == VERTICAL:
        print_str = f"position ({optimal_field:.3S}, {horizontal_pos.mean():.3S})"
        target_position = (optimal_field.n, horizontal_pos.mean().n)
    else:
        print_str = f"position (vertical,horizontal) ({vertical_pos.mean():.3S}, {optimal_field:.3S})"
        target_position = (vertical_pos.mean().n, optimal_field.n)

    print(f"1013 lightshift for {exp_name} is {optimal_field:.3S}  with " + print_str)
    screws_position = exp.getPicoScrewPositions()
    exp.setPicoScrewPosition(3,screws_position[2], update=False)
    exp.setPicoScrewPosition(4,screws_position[3], update=False)
    move_beam_to_target(exp, mako_idx=4, pico_idx=(3,4), target_position=target_position, tolerance=0.1)
    sleep(1)
    exp.setPicoScrewHomes()
    exp.setPicoScrewPosition(3,position=0, update=False)
    exp.setPicoScrewPosition(4,position=0, update=False)


def _calibration(exp:ExperimentProcedure ,config_file: ConfigurationFile):
    exp.setZynqOutput()
    analog_in_calibration(exp=exp, name = "prb_pwr")
    exp.save_all()
    sleep(2)
    analog_in_calibration(exp=exp, name = "op_pwr")
    exp.save_all()
    config_file.reopen()

def start_and_stop_camera(exp:ExperimentProcedure):
    exp.startMako(mako_idx=3)
    sleep(1)
    exp.startMako(mako_idx=4)
    sleep(1)
    exp.stopMako(mako_idx=3)
    sleep(1)
    exp.stopMako(mako_idx=4)
    sleep(1)

def zeroScrews(exp:ExperimentProcedure):
    exp.setPicoScrewHomes()
    exp.setPicoScrewPosition(1,0, update=False)
    exp.setPicoScrewPosition(2,0, update=False)
    exp.setPicoScrewPosition(3,0, update=False)
    exp.setPicoScrewPosition(4,0, update=False)

if __name__ == '__main__':
    zeroScrews(exp)

    shuttle_grid_files(grid_file_name=grid_file_name)

    # rydberg_420_lightshift_DLS_D1(exp_postfix="2D-preAlignment", amplitude=0.2)
    # rydberg_420_lightshift_DLS_D1(exp_postfix="2D-preAlignment", amplitude=0.0)


    # rydberg_1013_lightshift_DLS_D1(exp_postfix="2D-preAlignment", amplitude=1.0)
    # rydberg_1013_lightshift_DLS_D1(exp_postfix="2D-preAlignment", amplitude=0.0)

    # for idx in range(9,100):
    #     rydberg_1013_lightshift_DLS_D1(exp_postfix=f"2D-preAlignment-{idx}", amplitude=1.0, timeout_control = {'use':True, 'timeout':420})
    #     rydberg_1013_lightshift_DLS_D1(exp_postfix=f"2D-preAlignment-{idx}", amplitude=0.0, timeout_control = {'use':True, 'timeout':420})
    #     # sleep(600)
    #     exp.hardware_controller.restart_zynq_control()

    # exp.hardware_controller.restart_zynq_control()
    # rydberg_1013_alignment_DLS(exp_postfix="2D", pico_idx=4)
    # exp.hardware_controller.restart_zynq_control()
    # rydberg_1013_alignment_DLS(exp_postfix="2D", pico_idx=3)
    # exp.hardware_controller.restart_zynq_control()
    # rydberg_1013_lightshift_DLS_D1(exp_postfix="2D-postAlignment", amplitude=1.0)
    # rydberg_1013_lightshift_DLS_D1(exp_postfix="2D-postAlignment", amplitude=0.0)
    # # sleep(10)
    # exp.hardware_controller.restart_zynq_control()

    # rydberg_420_lightshift_DLS_D1(exp_postfix="2D-preAlignment", amplitude=0.2)
    # rydberg_420_lightshift_DLS_D1(exp_postfix="2D-preAlignment", amplitude=0.0)
    # # # exp.hardware_controller.restart_zynq_control()
    # rydberg_420_alignment_DLS(exp_postfix="2D", pico_idx=1)
    # # # exp.hardware_controller.restart_zynq_control()
    rydberg_420_alignment_DLS(exp_postfix="2D", pico_idx=2)
    # # # exp.hardware_controller.restart_zynq_control()
    rydberg_420_lightshift_DLS_D1(exp_postfix="2D-postAlignment", amplitude=0.2)
    rydberg_420_lightshift_DLS_D1(exp_postfix="2D-postAlignment", amplitude=0.0)
    # sleep(10)
    # # exp.hardware_controller.restart_zynq_control()

    # rydberg_420_lightshift_DLS(exp_postfix="2D-preAlignment-2")
    # exp.hardware_controller.restart_zynq_control()
    # rydberg_420_alignment_DLS(exp_postfix="2D-2", pico_idx=1)
    # exp.hardware_controller.restart_zynq_control()
    # rydberg_420_alignment_DLS(exp_postfix="2D-2", pico_idx=2)
    # exp.hardware_controller.restart_zynq_control()
    # rydberg_420_lightshift_DLS(exp_postfix="2D-postAlignment-2")
    # sleep(10)

    # rydberg_1013_lightshift_DLS(exp_postfix="2D-preAlignment")
    # exp.hardware_controller.restart_zynq_control()
    # rydberg_1013_alignment_DLS(exp_postfix="2D", pico_idx=3)
    # exp.hardware_controller.restart_zynq_control()
    # rydberg_1013_alignment_DLS(exp_postfix="2D", pico_idx=4)
    # exp.hardware_controller.restart_zynq_control()
    # rydberg_1013_lightshift_DLS(exp_postfix="2D-postAlignment")
    # sleep(10)