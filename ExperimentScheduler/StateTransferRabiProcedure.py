import numpy as np
from ExperimentProcedure import *
from ExperimentProcedure import experiment_monitoring, analog_in_calibration_monitoring
import time


YEAR, MONTH, DAY = today()
exp = ExperimentProcedure()

config_name = "RamanStateTransfer_Rabi.Config"
config_path = exp.CONFIGURATION_DIR + config_name
config_file = ConfigurationFile(config_path)

config_file.modify_parameter("REPETITIONS", "Reps:", str(7))
for variable in config_file.config_param.variables:
    config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    

config_file.config_param.update_variable("resonance_scan", scan_type="Variable", new_initial_values=[7.1], new_final_values=[7.5])
config_file.config_param.update_scan_dimension(0, range_index=0, variations=17)

exp.open_configuration("\\ExperimentAutomation\\" + config_name)

# analysis grid
window = [0,0,65,40]
thresholds = 100
binnings = np.linspace(80, 120, 161)
analysis_locs = da.DataAnalysis(year='2025', month='September', day='12', data_name='data_11', 
                                window=window, thresholds=100, binnings=binnings)


def resonace_scan(exp_idx, Raman_amplitude, timeout_control = {'use':False, 'timeout':1200}):
    script_name = "tweezerloading_Optical_RabiRamsey.mScript"
    scan_center = 14.7 #14.79 #7.3 # MHz
    scan_range_half = 0.5 * (np.abs(Raman_amplitude)/4.0) #-4V = 0.5MHz # -1.0V setpoint corresponds to 0.1 MHz half range
    scan_time = 0.95 * 4 * (4/np.abs(Raman_amplitude)) #-4V = 4us # -1.0V setpoint corresponds to ~12us pi time

    # Update configuration
    config_file.modify_parameter("REPETITIONS", "Reps:", str(6))
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_scan_dimension(0, new_ranges=[ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=26)])
    config_file.config_param.update_variable("d1_resonance", scan_type="Variable", new_initial_values=[scan_center-scan_range_half], new_final_values=[scan_center+scan_range_half])
    config_file.config_param.update_variable("time_scan_us", constant_value = scan_time)
    config_file.config_param.update_variable("amplitude_scan", constant_value = -np.abs(Raman_amplitude))
    config_file.save()
    
    # Setup experiment details
    YEAR, MONTH, DAY = today()
    exp_name = f"RAMAN-STATETRANSFER-RESONANCE-SCAN-AMP{np.abs(Raman_amplitude):.2f}-{exp_idx}"
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.open_master_script("\\ExperimentAutomation\\" + script_name)
    exp.run_experiment(exp_name)
    
    # Monitor experiment status
    experiment_monitoring(exp=exp, timeout_control=timeout_control)

    # Analyze the data
    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    analysis_result = data_analysis.analyze_data(function=da.sinc_sq)
    optimal_field = analysis_result[1]
    print(f"Optimal resoance for {exp_name} is {optimal_field:.3S} ")

    fit_fail = (optimal_field.s > 1) #or (analysis_result[0].n > 0) or (analysis_result[0].n < -2)
    if fit_fail:
        raise ValueError(f"Optimal resoance {optimal_field:.3S} has a variance larger than 1 or {analysis_result[0]:.3S} is outside the normal range, this typically means bad data.")

    # Update configuration with the optimal field
    config_file.config_param.update_variable("d1_resonance", constant_value = round(optimal_field.n, 4))
    config_file.save()
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    return  fit_fail

def rabi_scan(exp_idx, Raman_amplitude, timeout_control = {'use':False, 'timeout':1200}):
    script_name = "tweezerloading_Optical_RabiRamsey.mScript" 
    scan_time = 0.01 + 120 * (4/np.abs(Raman_amplitude)) # 100us for 4V

    config_file.modify_parameter("REPETITIONS", "Reps:", str(6))
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_scan_dimension(0, new_ranges=[ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=121)])
    config_file.config_param.update_variable("time_scan_us", scan_type="Variable", new_initial_values=[0.01], new_final_values=[scan_time])
    config_file.config_param.update_variable("amplitude_scan", constant_value = -np.abs(Raman_amplitude))

    config_file.save()
    
    YEAR, MONTH, DAY = today()
    exp_name = f"RAMAN-STATETRANSFER-RABI-SCAN-AMP{np.abs(Raman_amplitude):.2f}-{exp_idx}"
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.open_master_script("\\ExperimentAutomation\\" + script_name)
    exp.run_experiment(exp_name)

    # Monitor experiment status
    experiment_monitoring(exp=exp, timeout_control=timeout_control)

    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    return


def calibration(idx, amplitude):
    try:
        resonace_scan(idx, amplitude)

        sleep(3)
        exp.hardware_controller.restart_zynq_control()
        sleep(3)

        rabi_scan(idx, amplitude)

    except Exception as e:
        print(e)
        exp.hardware_controller.restart_zynq_control()
        return


def procedure():
    amplitudes = [0.5, 1, 1.5, 2, 2.5, 3, 3.5, 4, 4.5, 5, 5.5, 6, 6.5]
    # amplitudes = [6, 6.5, 7]
    # amplitudes = [4]
    for _, amp in enumerate(amplitudes):
        if _ != 0:
            exp.hardware_controller.restart_zynq_control()
        calibration(1, amp)


if __name__=='__main__':
    procedure()

    # ryd_1013_mw_lightshift_scan(0)
    # test(0)
