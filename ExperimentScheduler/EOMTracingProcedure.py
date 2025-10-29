import numpy as np
from ExperimentProcedure import *
import time


YEAR, MONTH, DAY = today()
exp = ExperimentProcedure()

config_name = "tweezerloading.Config"
config_path = "C:/Chimera/Chimera-Cryo/Configurations/CryoTweezerLoading/" + config_name
config_file = ConfigurationFile(config_path)

config_file.modify_parameter("REPETITIONS", "Reps:", str(7))
for variable in config_file.config_param.variables:
    config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    
config_file.config_param.update_variable("ryd420_resonance", scan_type="Variable", new_initial_values=[70], new_final_values=[90])
config_file.config_param.update_scan_dimension(0, range_index=0, variations=21)


# analysis grid for 2x7 grid - 20250922
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


def resonance_scan_420_resonance(exp_idx, exp_name_prefix, timeout_control = {'use':True, 'timeout':1000}):
    # script_name = "Calibration_rydberg_420_1013_excitation.mScript"
    script_name = "rydberg_420_1013_EIT_SLM.mScript"
  

    # Update configuration
    config_file.modify_parameter("REPETITIONS", "Reps:", str(15))
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_scan_dimension(0, new_ranges=[ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=31)])
    config_file.config_param.update_variable("ryd420_resonance", scan_type="Variable", new_initial_values=[65], new_final_values=[95])
    config_file.config_param.update_variable("time_scan_us", constant_value = 3)
    config_file.config_param.update_variable("fraction_scan", constant_value = 0.1)
    config_file.save()
    
    # Setup experiment details
    YEAR, MONTH, DAY = today()
    exp_name = f"{exp_name_prefix}-{exp_idx}" #EFIELD-RESONANCE-SCAN-Z-MINUS
    exp.open_configuration("\\CryoTweezerLoading\\" + config_name)
    exp.open_master_script("\\CryoTweezerLoading\\" + config_name)
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

    # analysis_result = data_analysis.analyze_data()
    # optimal_field = analysis_result[1]
    # print(f"Optimal resoance for {exp_name} is {optimal_field:.3S} ")

    # fit_fail = (optimal_field.s > 1) or (analysis_result[0].n > 0) or (analysis_result[0].n < -2)
    # if fit_fail:
    #     raise ValueError(f"Optimal resoance {optimal_field:.3S} has a variance larger than 1 or {analysis_result[0]:.3S} is outside the normal range, this typically means bad data.")

    # # Update configuration with the optimal field
    # config_file.config_param.update_variable("resonance_scan", constant_value = round(optimal_field.n, 3))
    # config_file.save()
    # return fit_fail
    return aborted



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

def procedure(exp_idx):
    EOM_START_420 = 155.0
    EOM_START_1013 = 160.0
    DETUNING_RANGE = np.linspace(-20, 20, 41) # for 1013 to its transition, >0 -> 1013 blue, 420 red; <0 -> 1013 red, 420 blue

    previous_f_420 = 145.0
    previous_f_1013 = 180.0

    for detuning in DETUNING_RANGE:
        # --- Compute new target frequencies ---
        target_f_420 = EOM_START_420 + detuning / 2
        target_f_1013 = EOM_START_1013 - detuning

        # --- Move EOMs smoothly to new frequencies ---
        _move_EOM_resonance(previous_f_420, target_f_420, step=0.1, channel=0)
        previous_f_420 = target_f_420

        _move_EOM_resonance(previous_f_1013, target_f_1013, step=0.1, channel=1)
        previous_f_1013 = target_f_1013

        # --- Wait for system to stabilize ---
        sleep(1)

        exp_name = f"EIT-RESONANCE-SCAN-EOM420-{target_f_420:.2f}MHz-EOM1013-{target_f_1013:.2f}MHz"
        eid = exp_idx
        while True:
            try:
                aborted = resonance_scan_420_resonance(exp_idx=eid, exp_name_prefix=exp_name)
                if aborted:
                    exp.hardware_controller.restart_zynq_control()
                    sleep(5)
                break  # success -> exit retry loop
            except Exception as e:
                print(f"[Warning] Resonance scan failed at detuning {detuning:.2f} MHz: {e}")
                print("Attempting recovery and retrying with incremented exp_idx...")
                eid += 1
                sleep(10)
                exp.hardware_controller.restart_zynq_control()
                # loop will retry same detuning with new exp_idx

def procedure_onlymove1013(exp_idx):
    EOM_START_420 = 155.0
    EOM_START_1013 = 160.0
    # DETUNING_RANGE = np.linspace(-3, 3, 7) # for 1013 to its transition, >0 -> 1013 blue, 420 red; <0 -> 1013 red, 420 blue
    DETUNING_RANGE = np.concatenate((np.linspace(-15,-4,12),np.linspace(4,15,12))) # for 1013 to its transition, >0 -> 1013 blue, 420 red; <0 -> 1013 red, 420 blue
    DETUNING_RANGE = [15]

    previous_f_420 = 155.0
    previous_f_1013 = 149

    for detuning in DETUNING_RANGE:
        # --- Compute new target frequencies ---
        target_f_420 = EOM_START_420
        target_f_1013 = EOM_START_1013 - detuning

        # --- Move EOMs smoothly to new frequencies ---
        _move_EOM_resonance(previous_f_420, target_f_420, step=0.1, channel=0)
        previous_f_420 = target_f_420

        _move_EOM_resonance(previous_f_1013, target_f_1013, step=0.1, channel=1)
        previous_f_1013 = target_f_1013

        # --- Wait for system to stabilize ---
        sleep(1)

        exp_name = f"EIT-RESONANCE-SCAN-EOM420-{target_f_420:.2f}MHz-EOM1013-{target_f_1013:.2f}MHz"
        eid = exp_idx
        exp.hardware_controller.restart_zynq_control()
        while True:
            try:
                aborted = resonance_scan_420_resonance(exp_idx=eid, exp_name_prefix=exp_name)
                if aborted:
                    exp.hardware_controller.restart_zynq_control()
                    sleep(5)
                break  # success -> exit retry loop
            except Exception as e:
                print(f"[Warning] Resonance scan failed at detuning {detuning:.2f} MHz: {e}")
                print("Attempting recovery and retrying with incremented exp_idx...")
                eid += 1
                sleep(10)
                exp.hardware_controller.restart_zynq_control()
                # loop will retry same detuning with new exp_idx


def efield_tracing_procedure():
    # for idx in np.arange(2):
    procedure_onlymove1013(1)
        # if idx<=0: continue
        # print(f"Running experiment sets number {idx}")
        # if idx != 0:
        #     exp.hardware_controller.restart_zynq_control()
        # high_to_low_direction(0)
        # low_to_high_direction(0,channel=channel)


if __name__=='__main__':
    efield_tracing_procedure()
    # _raw_move_EOM_resonance(exp, start_freq=144,end_freq=185,step=0.25)
