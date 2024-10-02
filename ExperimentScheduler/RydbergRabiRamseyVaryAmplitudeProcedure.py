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

def resonace_scan(exp_idx, exp_name_prefix, pulse_time=0.15, timeout_control = {'use':False, 'timeout':600}):
    script_name = "Calibration_rydberg_420_1013_excitation.mScript"

    # Update configuration
    config_file.modify_parameter("REPETITIONS", "Reps:", str(5))
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_scan_dimension(0, new_ranges=[ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=32)])
    config_file.config_param.update_variable("resonance_scan", scan_type="Variable", new_initial_values=[73], new_final_values=[87])
    config_file.config_param.update_variable("time_scan_us", constant_value = round(pulse_time, 2))
    config_file.save()
    
    # Setup experiment details
    YEAR, MONTH, DAY = today()
    exp_name = f"{exp_name_prefix}-{exp_idx}" #EFIELD-RESONANCE-SCAN-Z-MINUS
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.open_master_script("\\ExperimentAutomation\\" + script_name)
    exp.run_experiment(exp_name)
    
    # Monitor experiment status
    experiment_monitoring(exp=exp, timeout_control=timeout_control)

    # Analyze the data
    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    analysis_result = data_analysis.analyze_data()
    optimal_field = analysis_result[1]
    print(f"Optimal resoance for {exp_name} is {optimal_field:.3S} ")

    fit_fail = (optimal_field.s > 1) or (analysis_result[0].n > 0) or (analysis_result[0].n < -2)
    if fit_fail:
        raise ValueError(f"Optimal resoance {optimal_field:.3S} has a variance larger than 1 or {analysis_result[0]:.3S} is outside the normal range, this typically means bad data.")

    # # Update configuration with the optimal field
    config_file.config_param.update_variable("resonance_scan", constant_value = round(optimal_field.n, 3))
    config_file.save()
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    return fit_fail

def rabi_scan(exp_idx, exp_name_prefix, range_values, variations, timeout_control = {'use':False, 'timeout':600}):
    script_name = "Calibration_rydberg_420_1013_excitation.mScript"
    config_file.modify_parameter("REPETITIONS", "Reps:", str(6))
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
 
    new_ranges = []
    for idx, var in enumerate(variations):
        new_ranges.append(ScanRange(index=idx,left_inclusive=True, right_inclusive=True, variations=var))
    initial_vals, final_vals = np.array(range_values).T

    config_file.config_param.update_scan_dimension(0, new_ranges=new_ranges)
    config_file.config_param.update_variable("time_scan_us", scan_type="Variable", new_initial_values=initial_vals, new_final_values=final_vals)

    config_file.save()
    
    YEAR, MONTH, DAY = today()
    exp_name = f"{exp_name_prefix}-{exp_idx}"
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.open_master_script("\\ExperimentAutomation\\" + script_name)
    exp.run_experiment(exp_name)

    # Monitor experiment status
    experiment_monitoring(exp=exp, timeout_control=timeout_control)

    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    return


def _calibration():
    exp.setZynqOutput()
    analog_in_calibration(exp=exp, name = "prb_pwr")
    exp.save_all()
    config_file.reopen()

def calibration(exp_idx, blue_amp, red_amp):
    pulse_time = (0.15-0.10)*((0.45*0.18)/((blue_amp)*red_amp))**1/2+0.10
    time_range = [[0.01,0.33], [1.5,1.8], [3,3.3]]
    num_points = [17,11,11]
    prefix_str = f"BLUEAMP_{blue_amp:.2f}-REDAMP_{red_amp:.2f}"
    try:
        _calibration()
        config_file.config_param.update_variable("ryd_420_amplitude", constant_value = blue_amp)
        config_file.config_param.update_variable("ryd_1013_amplitude", constant_value = red_amp)        
        config_file.save()
        resonace_scan(exp_idx=exp_idx, exp_name_prefix="RESONANCE-SCAN-"+prefix_str, pulse_time=pulse_time, 
                      timeout_control = {'use':True, 'timeout':900})
    except Exception as e:
        print(e)
        exp.hardware_controller.restart_zynq_control()
        return False
    try:
        _calibration()
        config_file.config_param.update_variable("ryd_420_amplitude", constant_value = blue_amp)
        config_file.config_param.update_variable("ryd_1013_amplitude", constant_value = red_amp)        
        config_file.save()
        exp.hardware_controller.restart_zynq_control()
        rabi_scan(exp_idx=exp_idx, exp_name_prefix="RABI-SCAN-"+prefix_str, range_values=time_range, variations=num_points,
                  timeout_control = {'use':True, 'timeout':1200}) #1500
        sleep(1)
    except Exception as e:
        print(e)
        exp.hardware_controller.restart_zynq_control()
        return False
    
    return True

def procedure():
    ryd_420_amplitude = [0.1, 0.15,0.2]
    ryd_1013_amplitude = [0.15,0.3,0.45]
    RYD_420, RYD_1013 = np.meshgrid(ryd_420_amplitude, ryd_1013_amplitude)

    for idx_exp in np.arange(10):
        if idx_exp<=1:
            continue
        for id, (blue_amp, red_amp) in enumerate(zip(RYD_420.flatten(), RYD_1013.flatten())):
            print(f"Running experiment sets number {idx_exp}")
            if idx_exp != 2 and id != 0:
                exp.hardware_controller.restart_zynq_control()
            success = calibration(idx_exp, blue_amp=blue_amp,red_amp=red_amp)


if __name__=='__main__':
    procedure()