import numpy as np
from ExperimentProcedure import *
import time


YEAR, MONTH, DAY = today()
exp = ExperimentProcedure()

config_name = "Rydberg_Rabi_Ramsey.Config"
config_path = exp.CONFIGURATION_DIR + config_name
config_file = ConfigurationFile(config_path)

config_file.modify_parameter("REPETITIONS", "Reps:", str(7))
for variable in config_file.config_param.variables:
    config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    
config_file.config_param.update_variable("resonance_scan", scan_type="Variable", new_initial_values=[76], new_final_values=[84])
config_file.config_param.update_scan_dimension(0, range_index=0, variations=33)

# analysis grid
window = [0, 0, 200, 30]
thresholds = 65
binnings = np.linspace(0, 240, 241)
analysis_locs = da.DataAnalysis(year='2024', month='August', day='26', data_name='data_1', 
                                window=window, thresholds=70, binnings=binnings)

def experiment_monitoring(timeout_control = {'use':False, 'timeout':600}):
    # Monitor experiment status
    exp_start_time = time.time()
    timeout = timeout_control.get('timeout', 600)
    sleep(1)
    while exp.is_experiment_running():
        elapsed_time = time.time() - exp_start_time
        if timeout_control.get('use', False) and elapsed_time > timeout:
            exp.abort_experiment(save=True)
            print(f"Experiment aborted after {timeout} seconds due to timeout.")
            time.sleep(10)
            break
        print("Waiting for experiment to finish...")
        time.sleep(10) 
    return



def resonace_scan(exp_idx, timeout_control = {'use':False, 'timeout':600}):
    script_name = "Calibration_rydberg_420_1013_excitation.mScript"

    # Update configuration
    config_file.modify_parameter("REPETITIONS", "Reps:", str(4))
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_scan_dimension(0, new_ranges=[ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=33)])
    config_file.config_param.update_variable("resonance_scan", scan_type="Variable", new_initial_values=[73], new_final_values=[87])
    config_file.save()
    
    # Setup experiment details
    YEAR, MONTH, DAY = today()
    exp_name = f"RESONANCE-SCAN-{exp_idx}"
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.open_master_script("\\ExperimentAutomation\\" + script_name)
    exp.run_experiment(exp_name)
    
    # Monitor experiment status
    experiment_monitoring(timeout_control=timeout_control)

    # Analyze the data
    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    analysis_result = data_analysis.analyze_data()
    optimal_field = analysis_result[1]
    print(f"Optimal resoance for {exp_name} is {optimal_field:.3S} ")

    # Update configuration with the optimal field
    config_file.config_param.update_variable("resonance_scan", constant_value = round(optimal_field.n, 3))
    config_file.save()
    return 

def rabi_scan(exp_idx, timeout_control = {'use':False, 'timeout':600}):
    script_name = "Calibration_rydberg_420_1013_excitation.mScript"
    config_file.modify_parameter("REPETITIONS", "Reps:", str(7))
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_scan_dimension(0, new_ranges=[
        ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=21),
        ScanRange(index=1,left_inclusive=True, right_inclusive=True, variations=11),
        ScanRange(index=2,left_inclusive=True, right_inclusive=True, variations=11)])
    config_file.config_param.update_variable("time_scan_us", scan_type="Variable", new_initial_values=[0.01,1.5,3.0], new_final_values=[0.41,1.8,3.3])
    config_file.save()
    
    YEAR, MONTH, DAY = today()
    exp_name = f"RABI-SCAN-{exp_idx}"
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.open_master_script("\\ExperimentAutomation\\" + script_name)
    exp.run_experiment(exp_name)

    # Monitor experiment status
    experiment_monitoring(timeout_control=timeout_control)

    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    return

def ramsey_scan(exp_idx, timeout_control = {'use':False, 'timeout':600}):
    script_name = "Calibration_rydberg_420_1013_Ramsey.mScript"
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_scan_dimension(0, new_ranges=[
        ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=11),
        ScanRange(index=1,left_inclusive=True, right_inclusive=True, variations=11),
        ScanRange(index=2,left_inclusive=True, right_inclusive=True, variations=11)])
    config_file.config_param.update_variable("time_scan_us", scan_type="Variable", new_initial_values=[0.5,1.5,2.5], new_final_values=[1,2,3])
    config_file.save()
    
    YEAR, MONTH, DAY = today()
    exp_name = f"RAMSEY-SCAN-420-BEAM-OFF-{exp_idx}"
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.open_master_script("\\ExperimentAutomation\\" + script_name)
    exp.run_experiment(exp_name)

    # Monitor experiment status
    experiment_monitoring(timeout_control=timeout_control)

    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    return
    
def ramsey_scan_bothOff(exp_idx, timeout_control = {'use':False, 'timeout':600}):
    script_name = "Calibration_rydberg_420_1013_Ramsey_bothOff.mScript"
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_variable("time_scan_us", scan_type="Variable", new_initial_values=[0.5,1.5,2.5], new_final_values=[1,2,3])
    config_file.config_param.update_scan_dimension(0, new_ranges=[
        ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=11),
        ScanRange(index=1,left_inclusive=True, right_inclusive=True, variations=11),
        ScanRange(index=2,left_inclusive=True, right_inclusive=True, variations=11)])
    config_file.save()
    
    YEAR, MONTH, DAY = today()
    exp_name = f"RAMSEY-SCAN-BOTH-BEAM-OFF-{exp_idx}"
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.open_master_script("\\ExperimentAutomation\\" + script_name)
    exp.run_experiment(exp_name)
    
    # Monitor experiment status
    experiment_monitoring(timeout_control=timeout_control)

    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    return


def calibration(exp_idx):
    try:
        resonace_scan(exp_idx=exp_idx, timeout_control = {'use':True, 'timeout':900})
    except Exception as e:
        print(e)
        exp.hardware_controller.restart_zynq_control()
        # calibration(exp_idx)
        return
    try:
        # exp.hardware_controller.restart_zynq_control()
        rabi_scan(exp_idx=exp_idx, timeout_control = {'use':True, 'timeout':1200})
        sleep(1)
        # ramsey_scan(exp_idx=exp_idx, timeout_control = {'use':True, 'timeout':400})
        # sleep(1)
        # ramsey_scan_bothOff(exp_idx=exp_idx, timeout_control = {'use':True, 'timeout':400})
    except Exception as e:
        print(e)
        exp.hardware_controller.restart_zynq_control()
        return

def procedure():
    for idx in np.arange(100):
        # if idx<=48: continue
        print(f"Running experiment sets number {idx}")
        if idx != 0:
            exp.hardware_controller.restart_zynq_control()
        calibration(idx)

def test(exp_idx):
    resonace_scan(exp_idx=exp_idx)
    rabi_scan(exp_idx=exp_idx)
    ramsey_scan(exp_idx=exp_idx)
    ramsey_scan_bothOff(exp_idx=exp_idx)
    

if __name__=='__main__':
    procedure()

    # test(0)
