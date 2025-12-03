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

# # analysis grid
# window = [0, 0, 200, 30]
# thresholds = 65
# binnings = np.linspace(0, 240, 241)
# analysis_locs = da.DataAnalysis(year='2024', month='August', day='26', data_name='data_1', 
#                                 window=window, thresholds=70, binnings=binnings)

# analysis grid for 2x7 grid - 20250922
window = [0,0,90,20]
thresholds = 100
binnings = np.linspace(0, 240, 241)
analysis_locs = da.DataAnalysis(year='2025', month='September', day='19', data_name='data_13', 
                                window=window, thresholds=thresholds, binnings=binnings)


def resonace_scan(exp_idx, exp_name_prefix, timeout_control = {'use':False, 'timeout':600}):
    script_name = "Calibration_rydberg_420_1013_excitation.mScript"

    # Update configuration
    config_file.modify_parameter("REPETITIONS", "Reps:", str(4))
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_scan_dimension(0, new_ranges=[ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=22)])
    config_file.config_param.update_variable("resonance_scan", scan_type="Variable", new_initial_values=[73], new_final_values=[87])
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
    # config_file.config_param.update_variable("resonance_scan", constant_value = round(optimal_field.n, 3))
    # config_file.save()
    return fit_fail

def _calibration():
    exp.setZynqOutput()
    analog_in_calibration(exp=exp, name = "prb_pwr")
    exp.save_all()
    config_file.reopen()

def _raw_move_EOM_resonance(exp:ExperimentProcedure, start_freq, end_freq, step = 0.1, channel = 0):
    if start_freq == end_freq:
        return
    # Adjust the step sign so it moves toward end_freq
    if (end_freq - start_freq) * step < 0:
        step = -step
    # Ensure the final frequency is included
    freqs = np.arange(start_freq, end_freq, step)
    if freqs[-1] != end_freq:
        freqs = np.append(freqs, end_freq)

    for f in freqs:
        exp.setStaticDDS(ddsfreq=f, channel=channel)
        sleep(0.25)
    # Final safety set and longer wait
    exp.setStaticDDS(ddsfreq=end_freq, channel=channel)
    sleep(0.5)

def _move_EOM_resonance(start_freq, end_freq, step = 0.1, channel = 0):
    _raw_move_EOM_resonance(exp=exp, start_freq=start_freq, end_freq=end_freq, step=step, channel = channel)
    exp.save_all()
    config_file.reopen()

def high_to_low_direction(exp_idx, channel = 0):
    EOM_center_freqs = np.linspace(225, 255, 4)
    previous_f = 225
    for idx, eom_f in enumerate(EOM_center_freqs):
        _move_EOM_resonance(previous_f, eom_f, step=0.1, channel = channel)
        previous_f = eom_f
        _calibration()
        sleep(1)
        try:
            resonace_scan(exp_idx=exp_idx, exp_name_prefix=f"RESONANCE-SCAN-EOM{eom_f}-DESCEND")
        except:
            sleep(10)
            exp.hardware_controller.restart_zynq_control()
        sleep(10)
        exp.hardware_controller.restart_zynq_control()

def low_to_high_direction(exp_idx, channel = 0):
    # EOM_center_freqs = np.linspace(255, 225, 4)
    # previous_f = 255

    EOM_center_freqs = np.linspace(141.5, 241.5, 11)
    previous_f = EOM_center_freqs[0]

    for idx, eom_f in enumerate(EOM_center_freqs):
        _move_EOM_resonance(previous_f, eom_f, step=0.1, channel = channel)
        previous_f = eom_f
        # _calibration()
        sleep(1)
        try:
            resonace_scan(exp_idx=exp_idx, exp_name_prefix=f"RESONANCE-SCAN-EOM{eom_f}-ASCEND")
        except:
            sleep(2)
            exp.hardware_controller.restart_zynq_control()
        sleep(2)
        exp.hardware_controller.restart_zynq_control()


def efield_tracing_procedure(channel = 0):
    for idx in np.arange(1):
        # if idx<=0: continue
        # print(f"Running experiment sets number {idx}")
        # if idx != 0:
        #     exp.hardware_controller.restart_zynq_control()
        # high_to_low_direction(0)
        low_to_high_direction(0,channel=channel)


if __name__=='__main__':
    # efield_tracing_procedure()
    # _raw_move_EOM_resonance(exp, start_freq=581,end_freq=586,step=0.1)
    # _raw_move_EOM_resonance(exp, start_freq=205,end_freq=225,step=0.25)
    # _raw_move_EOM_resonance(exp, start_freq=324,end_freq=224,step=-0.5)
    # _raw_move_EOM_resonance(exp, start_freq=155,end_freq=239,step=0.5)
    # _raw_move_EOM_resonance(exp, start_freq=239,end_freq=195,step=-0.5)
    # _raw_move_EOM_resonance(exp, start_freq=201,end_freq=227,step=0.5)
    # _raw_move_EOM_resonance(exp, start_freq=227,end_freq=155,step=-0.5)
    # _raw_move_EOM_resonance(exp, start_freq=693,end_freq=643,step=-0.25)
    # _raw_move_EOM_resonance(exp, start_freq=643,end_freq=683,step=0.25)
    # _raw_move_EOM_resonance(exp, start_freq=201,end_freq=251,step=0.25)
    # _raw_move_EOM_resonance(exp, start_freq=251,end_freq=201,step=-0.25)
    # _raw_move_EOM_resonance(exp, start_freq=208,end_freq=215,step=0.25)
    # _raw_move_EOM_resonance(exp, start_freq=215,end_freq=201,step=-0.25)
    # _raw_move_EOM_resonance(exp, start_freq=201,end_freq=190.5,step=-0.25)
    # _raw_move_EOM_resonance(exp, start_freq=190.5,end_freq=185,step=-0.25)
    # _raw_move_EOM_resonance(exp, start_freq=185,end_freq=141.5,step=-0.25)

    # _raw_move_EOM_resonance(exp, start_freq=625,end_freq=645,step=0.25)
    # _raw_move_EOM_resonance(exp, start_freq=625,end_freq=615,step=-0.25)


    # _raw_move_EOM_resonance(exp, start_freq=200,end_freq=160,step=-0.25, channel=1)
    # _raw_move_EOM_resonance(exp, start_freq=160,end_freq=200,step=0.25, channel=1)

    # _raw_move_EOM_resonance(exp, start_freq=185,end_freq=195,step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=175,end_freq=155,step=-0.25, channel=0)

    # _raw_move_EOM_resonance(exp, start_freq=160,end_freq=180,step=0.25, channel=1)
    # _raw_move_EOM_resonance(exp, start_freq=155,end_freq=145,step=-0.25, channel=0)

    # _raw_move_EOM_resonance(exp, start_freq=180,end_freq=170,step=-0.25, channel=1)
    # _raw_move_EOM_resonance(exp, start_freq=145,end_freq=150,step=0.25, channel=0)

    # _raw_move_EOM_resonance(exp, start_freq=165,end_freq=150,step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=140,end_freq=170,step=-0.25, channel=1)


    # _raw_move_EOM_resonance(exp, start_freq=150,end_freq=147.5,step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=147.5,end_freq=155,step=0.25, channel=0)

    # _raw_move_EOM_resonance(exp, start_freq=155,end_freq=155+43.5,step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=155+43.5,end_freq=155+43.5+5,step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=155+43.5+5,end_freq=155+43.5+5-10,step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=155+43.5+5-10,end_freq=155,step=0.25, channel=0)

    # _raw_move_EOM_resonance(exp, start_freq=155,end_freq=203.5,step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=203.5,end_freq=155,step=0.25, channel=0)

    # _raw_move_EOM_resonance(exp, start_freq=159,end_freq=162.5,step=0.25, channel=1)

    # _raw_move_EOM_resonance(exp, start_freq=155,end_freq=203.5,step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=203.5,end_freq=155,step=0.25, channel=0)


    # _raw_move_EOM_resonance(exp, start_freq=155,end_freq=165,step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=162.5,end_freq=142.5,step=0.25, channel=1)

    # _raw_move_EOM_resonance(exp, start_freq=165,end_freq=145,step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=162.5,end_freq=182.5,step=0.25, channel=1)


    # _raw_move_EOM_resonance(exp, start_freq=145,end_freq=203.5,step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=203.5,end_freq=145,step=0.25, channel=0)

    # _raw_move_EOM_resonance(exp, start_freq=155,end_freq=155+4.9/2,step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=162.5+4.9,end_freq=162.5-4.9,step=0.25, channel=1)

    # _raw_move_EOM_resonance(exp, start_freq=155,end_freq=155+4.9/2,step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=162.5,end_freq=162.5-4.9,step=0.25, channel=1)

    # _raw_move_EOM_resonance(exp, start_freq=157.45,end_freq=203.5,step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=203.5,end_freq=157.45,step=0.25, channel=0)


    # _raw_move_EOM_resonance(exp, start_freq=157.45-10,end_freq=157.45-5,step=0.25, channel=0)

    # _raw_move_EOM_resonance(exp, start_freq=152.45,end_freq=203.5,step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=203.5,end_freq=152.45,step=0.25, channel=0)

    # _raw_move_EOM_resonance(exp, start_freq=152.45,end_freq=147.5,step=0.25, channel=0)

    _raw_move_EOM_resonance(exp, start_freq=571+8.05,end_freq=571+8.05+9.45/2, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=114,end_freq=120, step=0.25, channel=1)

