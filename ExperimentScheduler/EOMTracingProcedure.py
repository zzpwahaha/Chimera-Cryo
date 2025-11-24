import numpy as np
from ExperimentProcedure import *
import time
import serial


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
# window = [0,0,90,20]
# thresholds = 100
# binnings = np.linspace(0, 240, 241)
# analysis_locs = da.DataAnalysis(year='2025', month='September', day='30', data_name='data_19', 
#                                 window=window, thresholds=thresholds, binnings=binnings)

    
# analysis grid for 5x7 grid - 20250922
window = [0, 0, 65, 40]
thresholds = 105
binnings = np.linspace(0, 240, 241)
analysis_locs = da.DataAnalysis(year='2025', month='October', day='19', data_name='data_8', 
                                window=window, thresholds=thresholds, binnings=binnings, multi_points_option = dict({"active":True, "search_square":2, "num_points":8}))


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

def avalanche_scan_420_resonance(exp_idx, exp_name_prefix, aom_freq_420, timeout_control = {'use':True, 'timeout':600}):
    # script_name = "Calibration_rydberg_420_1013_excitation.mScript"
    script_name = "rydberg_420_1013_excitation_SLM_dressing.mScript"
  

    # Update configuration
    config_file.modify_parameter("REPETITIONS", "Reps:", str(10))
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_scan_dimension(0, new_ranges=[
            ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=11)])
    config_file.config_param.update_variable("time_scan_us", scan_type="Variable", 
                                             new_initial_values=[0.01], 
                                             new_final_values=[1500.01])
    # config_file.config_param.update_scan_dimension(0, new_ranges=[
    #         ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=5),
    #         ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=9),
    #         ScanRange(index=1,left_inclusive=True, right_inclusive=True, variations=10),])
    # config_file.config_param.update_variable("time_scan_us", scan_type="Variable", 
    #                                          new_initial_values=[0.01,200, 2000.0], 
    #                                          new_final_values=[100.01, 1000,20000.0])
    config_file.config_param.update_variable("ryd420_resonance", constant_value = aom_freq_420)
    config_file.save()
    
    # Setup experiment details
    YEAR, MONTH, DAY = today()
    exp_name = f"{exp_name_prefix}-{exp_idx}" #EFIELD-RESONANCE-SCAN-Z-MINUS
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

def procedure_onlymove420(exp_idx, amplitude_420, aom_start_420, eom_start_420=585, previous_f_420=None, eom_start_1013=120, previous_f_1013=None):
    EOM_START_420 = eom_start_420
    EOM_START_1013 = eom_start_1013
    DETUNING_RANGE = [12,9,5,3,0,3,-5,-9,-10,-11,-12,-13] 
    # DETUNING_RANGE = [10,8,7,6,5,4,3,2,1,-1,-3,-5,-7] 
    # DETUNING_RANGE = [9,-9] 
    # DETUNING_RANGE = [12, 14, 10,8,6,4,2,0,-2,-4,-6,-8,-10, -11, -13] 
    # DETUNING_RANGE = [16,] 
    DETUNING_RANGE.sort()
    AOM_START_420 = aom_start_420

    if previous_f_420 is None:
        previous_f_420 = 596
    if previous_f_1013 is None:
        previous_f_1013 = 120

    for _id, detuning in enumerate(DETUNING_RANGE):
        # --- Compute new target frequencies ---
        target_f_420 = EOM_START_420 + detuning
        target_f_1013 = EOM_START_1013 #+ (2*detuning)

        # --- Move EOMs smoothly to new frequencies ---
        _move_EOM_resonance(previous_f_420, target_f_420, step=0.1, channel=0)
        previous_f_420 = target_f_420

        _move_EOM_resonance(previous_f_1013, target_f_1013, step=0.1, channel=1)
        previous_f_1013 = target_f_1013

        # --- Wait for system to stabilize ---
        sleep(1)

        # exp_name = f"DRESSING-RESONANCE-SCAN-420AMP-{amplitude_420:.3f}-420EOM-{target_f_420:.2f}MHz-EOM1013-{target_f_1013:.2f}MHz-EOM840CENTER-{EOM_START_420:.2f}-EOM1013CENTER-{EOM_START_1013:.2f}"
        exp_name = f"DRESSING-RESONANCE-SCAN-420AMP-{amplitude_420:.3f}-420EOM-{target_f_420:.2f}MHz-EOM1013-{target_f_1013:.2f}MHz-EOM840CENTER-{EOM_START_420:.2f}"
        # exp_name = f"DRESSING-RESONANCE-SCAN-420AMP-{amplitude_420:.3f}-420EOM-{target_f_420:.2f}MHz-EOM1013-{target_f_1013:.2f}MHz-EOM1013CENTER-{EOM_START_1013:.2f}"
        eid = exp_idx
        if _id!=0: exp.hardware_controller.restart_zynq_control()
        while True:
            try:
                set_valon_frequency(AOM_START_420-detuning)
                aborted = avalanche_scan_420_resonance(exp_idx=eid, exp_name_prefix=exp_name, aom_freq_420=AOM_START_420-detuning)
                # if aborted:
                #     exp.hardware_controller.restart_zynq_control()
                #     sleep(5)
                break  # success -> exit retry loop
            except Exception as e:
                print(f"[Warning] Resonance scan failed at detuning {detuning:.2f} MHz: {e}")
                print("Attempting recovery and retrying with incremented exp_idx...")
                break
                # eid += 1
                # sleep(10)
                # exp.hardware_controller.restart_zynq_control()
                # loop will retry same detuning with new exp_idx


def efield_tracing_procedure():
    # for idx in np.arange(2):
    # procedure_onlymove1013(1)
    
    amp_detuning_dict_420 = {np.float64(-0.005): np.float64(78.8856118764176),
                            np.float64(-0.004): np.float64(78.90840974308706),
                            np.float64(-0.003): np.float64(78.89802843346514),
                            np.float64(-0.002): np.float64(78.9613031745272),
                            np.float64(-0.001): np.float64(78.94560348669144),
                            np.float64(0.0): np.float64(78.92825412345351),
                            np.float64(0.005): np.float64(79.03291166278343),
                            np.float64(0.01): np.float64(79.00093194335177),
                            np.float64(0.02): np.float64(79.10714751648125),
                            np.float64(0.03): np.float64(79.14581911921817),
                            np.float64(0.04): np.float64(79.49824583271692),
                            np.float64(0.05): np.float64(79.34627165354743),
                            np.float64(0.06): np.float64(79.33544009726145),
                            np.float64(0.08): np.float64(79.37829546111794),
                            np.float64(0.1): np.float64(79.5817719884493)}


    # ryd420_amplitudes = [-0.005,-0.004,-0.003,-0.002,-0.001,0,0.005,0.01,0.02,0.03,0.04,0.05,0.06,0.08,0.1]
    # ryd420_amplitudes = [-0.005,-0.003,-0.001,0.005,0.01,0.02,0.04,0.06,0.08,0.1]
    # ryd420_amplitudes = [0.02,0.04,0.06,0.08,0.1]
    # ryd420_amplitudes = np.round(ryd420_amplitudes, 3)
    # ryd420_amplitudes = [-0.001]
    ryd420_amplitudes = [0.04]
    # ryd420_amplitudes = [-0.005]

    for a in ryd420_amplitudes:
        # procedure_onlymove420(exp_idx=0, amplitude_420=a, aom_start_420=amp_detuning_dict_420[a]+0, eom_start_420=575+10-0, previous_f_420=597, eom_start_1013=120, previous_f_1013=120)
        procedure_onlymove420(exp_idx=0, amplitude_420=a, aom_start_420=79.17+0, eom_start_420=575+10-0, previous_f_420=572, eom_start_1013=120, previous_f_1013=120)
        # if idx<=0: continue
        # print(f"Running experiment sets number {idx}")
        # if idx != 0:
        #     exp.hardware_controller.restart_zynq_control()
        # high_to_low_direction(0)
        # low_to_high_direction(0,channel=channel)

    # eom_starts = [585,580]
    # previous_f_420s = [596, 599]
    # for idx, e in enumerate(eom_starts):
    #     procedure_onlymove420(exp_idx=1, amplitude_420=-0.001, aom_start_420=amp_detuning_dict_420[-0.001], eom_start_420=e, previous_f_420=previous_f_420s[idx])

def set_valon_frequency(freq):
    with serial.Serial(port="COM19", baudrate=115200, timeout=10) as ser:
        print('asd')
        ser.write(f"Source 2; Frequency {freq:.6f} MHz;\r".encode("utf-8"))
        sleep(0.5)
        recv = ser.read_all().decode('utf-8')
        print(recv)

        ser.write(f"Frequency?\r".encode("utf-8"))
        sleep(0.5)
        recv = ser.read_all().decode('utf-8')
        print(recv)

if __name__=='__main__':
    # set_valon_frequency(81)
    efield_tracing_procedure()
    # _raw_move_EOM_resonance(exp, start_freq=144,end_freq=185,step=0.25)
