import numpy as np
from ExperimentProcedure import *


YEAR, MONTH, DAY = today()
exp = ExperimentProcedure()

config_name = "Rydberg_Rabi_Ramsey.Config"
config_path = exp.CONFIGURATION_DIR + config_name
config_file = ConfigurationFile(config_path)

config_file.modify_parameter("REPETITIONS", "Reps:", str(5))
for variable in config_file.config_param.variables:
    config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    
config_file.config_param.update_variable("resonance_scan", scan_type="Variable", new_initial_values=[76], new_final_values=[84])
config_file.config_param.update_scan_dimension(0, range_index=0, variations=33)

# analysis grid
window = [0, 0, 200, 30]
thresholds = 65
binnings = np.linspace(0, 240, 241)
analysis_locs = da.DataAnalysis(year='2024', month='May', day='15', data_name='data_1', 
                                window=window, thresholds=104, binnings=binnings)

# for bias E x range = [-0.5, 1] 16, E y range = [-1.2, 0.0] 16, E z range = [-1., 1.0] 16

def resonace_scan(exp_idx):
    script_name = "Calibration_rydberg_420_1013_excitation.mScript"
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_scan_dimension(0, new_ranges=[ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=33)])
    config_file.config_param.update_variable("resonance_scan", scan_type="Variable", new_initial_values=[76], new_final_values=[84])
    config_file.save()
    
    YEAR, MONTH, DAY = today()
    exp_name = f"RESONANCE-SCAN-{exp_idx}"
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.open_master_script("\\ExperimentAutomation\\" + script_name)
    exp.run_experiment(exp_name)
    sleep(1)
    while exp.is_experiment_running():
        sleep(10)
        print("Waiting for experiment to finish")

    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    analysis_result = data_analysis.analyze_data()
    optimal_field = analysis_result[1]
    print(f"Optimal resoance for {exp_name} is {optimal_field:.3S} ")

    config_file.config_param.update_variable("resonance_scan", constant_value = round(optimal_field.n, 3))
    config_file.save()
    return 

def rabi_scan(exp_idx):
    script_name = "Calibration_rydberg_420_1013_excitation.mScript"
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_scan_dimension(0, new_ranges=[
        ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=17),
        ScanRange(index=1,left_inclusive=True, right_inclusive=True, variations=11),
        ScanRange(index=2,left_inclusive=True, right_inclusive=True, variations=11)])
    config_file.config_param.update_variable("time_scan_us", scan_type="Variable", new_initial_values=[0.01,1.5,2.5], new_final_values=[0.49,1.8,2.8])
    config_file.save()
    
    YEAR, MONTH, DAY = today()
    exp_name = f"RABI-SCAN-{exp_idx}"
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.open_master_script("\\ExperimentAutomation\\" + script_name)
    exp.run_experiment(exp_name)
    sleep(1)
    while exp.is_experiment_running():
        sleep(10)
        print("Waiting for experiment to finish")

    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    return

def ramsey_scan(exp_idx):
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
    sleep(1)
    while exp.is_experiment_running():
        sleep(10)
        print("Waiting for experiment to finish")

    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    return
    
def ramsey_scan_bothOff(exp_idx):
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
    sleep(1)
    while exp.is_experiment_running():
        sleep(10)
        print("Waiting for experiment to finish")

    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    return


def calibration(exp_idx):
    try:
        resonace_scan(exp_idx=exp_idx)
    except Exception as e:
        print(e)
        exp.hardware_controller.restart_zynq_control()
        calibration(exp_idx)
        return
    try:
        rabi_scan(exp_idx=exp_idx)
        sleep(1)
        ramsey_scan(exp_idx=exp_idx)
        sleep(1)
        ramsey_scan_bothOff(exp_idx=exp_idx)
    except Exception as e:
        print(e)
        exp.hardware_controller.restart_zynq_control()
        return

def procedure():
    for idx in np.arange(100):
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
