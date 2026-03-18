import numpy as np
from ExperimentProcedure import *
from ExperimentProcedure import experiment_monitoring, analog_in_calibration_monitoring
from RydbergBeamMoveProcedure import move_beam_to_target, RYDBERG_BEAM_420_POSITION, RYDBERG_BEAM_1013_POSITION
from EthernetClient import EthernetClient
import time

exp = ExperimentProcedure()

config_name = "AOD_grid_alignment_on_SLM.Config"
config_path = exp.CONFIGURATION_DIR + config_name
config_file = ConfigurationFile(config_path)
master_file = MasterConfiguration(ExperimentProcedure.MASTER_CONFIGURATION_DIR+'Master-Configuration.txt')

script_name = 'tweezerloading_rearrangement_5x20_AODalignment.mScript'
gscript_name_x = 'rearrangement_5x20_AODalignment_X.gScript'
gscript_name_y = 'rearrangement_5x20_AODalignment_Y.gScript'
constant_name = {'x': 'aod_offset_x', 'y': 'aod_offset_y'}

# analysis grid
# analysis grid for 5x20 grid - 20251223
window = [0,0,170,54]
thresholds = 105
binnings = np.linspace(0, 240, 241)
analysis_locs = da.DataAnalysis(year='2026', month='March', day='16', data_name='data_17', 
                                window=window, thresholds=thresholds, binnings=binnings, 
                                multi_points_option = dict({"active":True, "search_square":4, "num_points":6}))

def setup(config_file: ConfigurationFile):
    config_file.modify_parameter("REPETITIONS", "Reps:", str(5))
    config_file.modify_parameter("MAIN_OPTIONS", "Randomize Variations?", str(0))
    config_file.modify_parameter("MAIN_OPTIONS", "Repetition First Over Variation?", str(1))
    config_file.modify_parameter("GMOOG", "Experiment Active:", 1)
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
        
    config_file.config_param.update_variable("frequency_scan", scan_type="Variable", new_initial_values=[-0.4], new_final_values=[0.4])
    config_file.config_param.update_scan_dimension(0, range_index=0, variations=41)

def alignment(exp:ExperimentProcedure, config_file: ConfigurationFile, master_file: MasterConfiguration,
              exp_idx, x_or_y = 'x', postfix="", timeout_control = {'use':True, 'timeout':1000}):
    master_file.reopen()
    
    if x_or_y=='x':
        gscript = gscript_name_x
    elif x_or_y=='y':
        gscript = gscript_name_y
    else:
        raise ValueError(f'Unrecognized alignment direction: {x_or_y}')
    config_file.modify_parameter("GMOOG", "Scripted Arb Address:", gscript)
    config_file.save()
    
    # Setup experiment details
    YEAR, MONTH, DAY = today()
    exp_name = f"AODSLM-ALIGNMENT-{x_or_y}-{exp_idx}{postfix}"

    # exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.open_master_script("\\ExperimentAutomation\\" + script_name)
    exp.run_experiment(exp_name)
    
    # Monitor experiment status
    experiment_monitoring(exp=exp, timeout_control=timeout_control)

    # Analyze the data
    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    try:                   
        analysis_result = data_analysis.analyze_data_AOD_alignment()
        optimal_field = analysis_result[1]
        print(f"Optimal resoance for {exp_name} is {optimal_field:.3S} ")
        fit_fail=False
    except Exception as e:
        print(e)
        fit_fail=True
    fit_fail = (optimal_field.s > 1) or (optimal_field.n > 0.5) or (optimal_field.n < -0.5)
    if fit_fail:
        print(f"Optimal resoance {optimal_field:.3S} has a variance larger than 1 or {analysis_result[0]:.3S} is outside the normal range, this typically means bad data.")
    else:
        current_val = float(master_file.get_constant(constant_name[x_or_y]))
        master_file.set_constant(constant_name[x_or_y], f"{current_val+optimal_field.n:.3f}")
        print(f"Set the parameter {constant_name[x_or_y]} in master configuration from {current_val:.3f} to {current_val+optimal_field.n:.3f}")
        master_file.save()
    return  False #fit_fail


def procedure():
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)

    alignment(exp, config_file, master_file, exp_idx=0, x_or_y='y')
    sleep(5)
    alignment(exp, config_file, master_file, exp_idx=0, x_or_y='x')

if __name__=='__main__':
    procedure()