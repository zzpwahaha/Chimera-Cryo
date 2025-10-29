import numpy as np
from ExperimentProcedure import *
from RydbergBeamMoveProcedure import move_beam_to_target, RYDBERG_BEAM_420_POSITION, RYDBERG_BEAM_1013_POSITION
import time


YEAR, MONTH, DAY = today()
exp = ExperimentProcedure()

config_name = "tweezerloading.Config"
config_path = "C:/Chimera/Chimera-Cryo/Configurations/CryoTweezerLoading/" + config_name
config_file = ConfigurationFile(config_path)

config_file.modify_parameter("REPETITIONS", "Reps:", str(10))
for variable in config_file.config_param.variables:
    config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    
config_file.config_param.update_variable("ryd420_resonance", scan_type="Variable", new_initial_values=[70], new_final_values=[90])
config_file.config_param.update_scan_dimension(0, range_index=0, variations=21)


# # analysis grid for 2x7 grid - 20250922
# window = [0,0,90,20]
# thresholds = 100
# binnings = np.linspace(0, 240, 241)
# analysis_locs = da.DataAnalysis(year='2025', month='September', day='19', data_name='data_13', 
#                                 window=window, thresholds=thresholds, binnings=binnings)

# analysis grid for 1x7 grid - 202509
window = [0,0,90,20]
thresholds = 100
binnings = np.linspace(0, 240, 241)
analysis_locs = da.DataAnalysis(year='2025', month='September', day='30', data_name='data_19', 
                                window=window, thresholds=thresholds, binnings=binnings)


def resonace_scan(exp_idx, timeout_control = {'use':True, 'timeout':1000}):
    # script_name = "Calibration_rydberg_420_1013_excitation.mScript"
    script_name = "rydberg_420_1013_excitation_SLM.mScript"
  

    # Update configuration
    config_file.modify_parameter("REPETITIONS", "Reps:", str(12))
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_scan_dimension(0, new_ranges=[ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=21)])
    config_file.config_param.update_variable("ryd420_resonance", scan_type="Variable", new_initial_values=[70], new_final_values=[90])
    config_file.config_param.update_variable("time_scan_us", constant_value = 0.1)
    config_file.save()
    
    # Setup experiment details
    YEAR, MONTH, DAY = today()
    exp_name = f"RESONANCE-SCAN-{exp_idx}"
    exp.open_configuration("\\CryoTweezerLoading\\" + config_name)
    exp.open_master_script("\\CryoTweezerLoading\\" + script_name)
    # exp.open_master_script("\\ExperimentAutomation\\" + script_name)
    exp.run_experiment(exp_name)
    
    # Monitor experiment status
    aborted = experiment_monitoring(exp=exp, timeout_control=timeout_control)

    # Analyze the data
    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    x,y,yerr = data_analysis.get_loading_result_1D()
    if np.any(np.isnan(y)) or (y.mean()<0.2):
        raise ValueError(f"No atom is loaded. Should restart Zynq")

    analysis_result = data_analysis.analyze_data()
    optimal_field = analysis_result[1]
    print(f"Optimal resoance for {exp_name} is {optimal_field:.3S} ")

    # fit_fail = (optimal_field.s > 1) or (analysis_result[0].n > 0) or (analysis_result[0].n < -2)
    # if fit_fail:
    #     raise ValueError(f"Optimal resoance {optimal_field:.3S} has a variance larger than 1 or {analysis_result[0]:.3S} is outside the normal range, this typically means bad data.")

    # Update configuration with the optimal field
    config_file.config_param.update_variable("ryd420_resonance", constant_value = round(optimal_field.n, 3))
    config_file.save()
    # return fit_fail
    return aborted



# def _calibration():
#     exp.setZynqOutput()
#     analog_in_calibration(exp=exp, name = "prb_pwr")
#     exp.save_all()
#     config_file.reopen()

def rabi_scan(exp_idx, timeout_control = {'use':True, 'timeout':2000}):
    script_name = "rydberg_420_1013_excitation_SLM.mScript"
    config_file.modify_parameter("REPETITIONS", "Reps:", str(10))
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    

    config_file.config_param.update_scan_dimension(0, new_ranges=[
        ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=21),
        ScanRange(index=1,left_inclusive=True, right_inclusive=True, variations=11),
        ScanRange(index=2,left_inclusive=True, right_inclusive=True, variations=11)
        ])
    config_file.config_param.update_variable("time_scan_us", scan_type="Variable", 
                                             new_initial_values=[0.01,2.4,4.9], 
                                             new_final_values=[0.41,2.6,5.1])

    config_file.save()
    
    YEAR, MONTH, DAY = today()
    exp_name = f"RABI-SCAN-{exp_idx}"
    exp.open_configuration("\\CryoTweezerLoading\\" + config_name)
    exp.open_master_script("\\CryoTweezerLoading\\" + script_name)
    exp.run_experiment(exp_name)

    # Monitor experiment status
    aborted = experiment_monitoring(exp=exp, timeout_control=timeout_control)

    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    return aborted

def recenter_beams():
    exp.setDAC()
    sleep(1)
    exp.setDDS()
    move_beam_to_target(exp=exp, mako_idx=3, pico_idx=(1,2), target_position=RYDBERG_BEAM_420_POSITION, tolerance=0.2)
    sleep(1)
    move_beam_to_target(exp=exp, mako_idx=4, pico_idx=(3,4), target_position=RYDBERG_BEAM_1013_POSITION, tolerance=0.2)
    sleep(1)
    exp.save_all()
    config_file.reopen()
    sleep(1)

def calibration(exp_idx):
    try:
        recenter_beams()
        aborted = resonace_scan(exp_idx=exp_idx, timeout_control = {'use':True, 'timeout':1200})
        if aborted:
            exp.hardware_controller.restart_zynq_control()
            return
    except Exception as e:
        print(e)
        exp.hardware_controller.restart_zynq_control()
        # calibration(exp_idx)
        return
    try:
        # exp.hardware_controller.restart_zynq_control()
        # _calibration()
        recenter_beams()
        rabi_scan(exp_idx=exp_idx, timeout_control = {'use':True, 'timeout':1800}) #1500
        if aborted:
            exp.hardware_controller.restart_zynq_control()
            return
        sleep(3)

    except Exception as e:
        print(e)
        exp.hardware_controller.restart_zynq_control()
        return

if __name__=='__main__':
    for idx in np.arange(100):
        # if idx<12: continue
        # if idx<12: continue
        print(f"Running experiment sets number {idx}")
        if idx != 0:
            exp.hardware_controller.restart_zynq_control()
        calibration(idx)

    # _raw_move_EOM_resonance(exp, start_freq=144,end_freq=185,step=0.25)
