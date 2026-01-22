import numpy as np
from ExperimentProcedure import *
from RydbergBeamMoveProcedure import move_beam_to_target, RYDBERG_BEAM_420_POSITION, RYDBERG_BEAM_1013_POSITION
import time


YEAR, MONTH, DAY = today()
exp = ExperimentProcedure()

# config_name = "Rydberg_Rabi_SLM.Config"
# config_path = exp.CONFIGURATION_DIR + config_name
# config_file = ConfigurationFile(config_path)

config_name = "tweezerloading.Config"
config_path = "C:/Chimera/Chimera-Cryo/Configurations/CryoTweezerLoading/" + config_name
config_file = ConfigurationFile(config_path)


config_file.modify_parameter("REPETITIONS", "Reps:", str(10))
for variable in config_file.config_param.variables:
    config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    
config_file.config_param.update_variable("ryd420_eom_resonance", scan_type="Variable", new_initial_values=[70], new_final_values=[90])
config_file.config_param.update_scan_dimension(0, range_index=0, variations=21)

# analysis grid for 1x7 grid - 202509
window = [0,0,90,20]
thresholds = 100
binnings = np.linspace(0, 240, 241)
analysis_locs = da.DataAnalysis(year='2025', month='December', day='30', data_name='data_2', 
                                window=window, thresholds=thresholds, binnings=binnings)

EOM_CENTER_FREQ = 530 #575+4 # roughly resonant with AOM = 75 MHz
CURRENT_EOM_FREQ = 530.554 #579

def resonace_scan(exp_idx, ryd420_amplitude, timeout_control = {'use':True, 'timeout':1000}):
    global CURRENT_EOM_FREQ
    # script_name = "Calibration_rydberg_420_1013_Rabi_SLM.mScript"
    script_name = "rydberg_420_1013_excitation_SLM.mScript"

    CENTER_FREQ = 79 # MHz
    AOM_CENTER_FREQ = 75 #MHz
    # scan_range_half = np.sqrt(ryd420_amplitude/0.8) * 3 # MHz
    # pulse_time = np.round(1/np.sqrt(ryd420_amplitude/0.8) * 0.30, 2)  #us
    # rabi_freq = np.sqrt((ryd420_amplitude+0.013)/0.063)*134*147/2/1540 # for without solidstate switch
    rabi_freq = np.sqrt((ryd420_amplitude+0.013)/0.063)*4.2
    scan_range_half = min(7, rabi_freq*2)
    pulse_time = np.round(1/rabi_freq/2, 2)  #us

    _raw_move_EOM_resonance(exp=exp, start_freq=CURRENT_EOM_FREQ, end_freq=EOM_CENTER_FREQ-scan_range_half, step=0.25, channel=0)

    # Update configuration
    config_file.modify_parameter("REPETITIONS", "Reps:", str(15))
    config_file.modify_parameter("MAIN_OPTIONS", "Randomize Variations?", str(0))
    config_file.modify_parameter("MAIN_OPTIONS", "Repetition First Over Variation?", str(1))
    config_file.modify_parameter('STATIC_DDS', ' DDS-0 Value:', f'{EOM_CENTER_FREQ:.4f}+ryd420_eom_resonance')
    config_file.modify_parameter('STATIC_DDS', 'Control?', 1)

    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_scan_dimension(0, new_ranges=[ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=21)])
    config_file.config_param.update_variable("ryd420_eom_resonance", scan_type="Variable", new_initial_values=[-scan_range_half], new_final_values=[scan_range_half])
    config_file.config_param.update_variable("ryd420_resonance", constant_value = AOM_CENTER_FREQ)
    config_file.config_param.update_variable("time_scan_us", constant_value = pulse_time)
    config_file.config_param.update_variable("ryd420_amplitude", constant_value = ryd420_amplitude)
    config_file.save()
    
    # Setup experiment details
    YEAR, MONTH, DAY = today()
    exp_name = f"RESONANCE-SCAN-420AMP-{ryd420_amplitude:.3f}-{exp_idx}"
    # exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    # exp.open_master_script("\\ExperimentAutomation\\" + script_name)
    exp.open_configuration("\\CryoTweezerLoading\\" + config_name)
    exp.open_master_script("\\CryoTweezerLoading\\" + script_name)

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

    CURRENT_EOM_FREQ = EOM_CENTER_FREQ+scan_range_half

    # fit_fail = (optimal_field.s > 1) or (analysis_result[0].n > 0) or (analysis_result[0].n < -2)
    # if fit_fail:
    #     raise ValueError(f"Optimal resoance {optimal_field:.3S} has a variance larger than 1 or {analysis_result[0]:.3S} is outside the normal range, this typically means bad data.")

    # Update configuration with the optimal field
    config_file.config_param.update_variable("ryd420_eom_resonance", constant_value = round(optimal_field.n, 3))
    _raw_move_EOM_resonance(exp=exp, start_freq=CURRENT_EOM_FREQ, end_freq=EOM_CENTER_FREQ+round(optimal_field.n, 3), step=0.25, channel=0)
    config_file.save()
    # return fit_fail
    return aborted



# def _calibration():
#     exp.setZynqOutput()
#     analog_in_calibration(exp=exp, name = "prb_pwr")
#     exp.save_all()
#     config_file.reopen()

def rabi_scan(exp_idx, ryd420_amplitude, timeout_control = {'use':True, 'timeout':2000}):
    # script_name = "Calibration_rydberg_420_1013_Rabi_SLM.mScript"
    script_name = "rydberg_420_1013_excitation_SLM.mScript"

    # rabi_freq = np.sqrt(ryd420_amplitude/0.8) * 1.493 # MHz
    rabi_freq = np.sqrt((ryd420_amplitude+0.013)/0.063)*122*147/2/1540
    twopi_time = np.ceil(1/rabi_freq * 10) / 10 # us and is always divisiable by 10

    config_file.modify_parameter("REPETITIONS", "Reps:", str(15))
    config_file.modify_parameter("MAIN_OPTIONS", "Randomize Variations?", str(1))
    config_file.modify_parameter("MAIN_OPTIONS", "Repetition First Over Variation?", str(0))
    config_file.modify_parameter('STATIC_DDS', 'Control?', 0)

    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_variable("ryd420_amplitude", constant_value = ryd420_amplitude)

    if twopi_time <= 0.4: #0.8
        config_file.config_param.update_scan_dimension(0, new_ranges=[
            ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=11),
            ScanRange(index=1,left_inclusive=True, right_inclusive=True, variations=11),
            ScanRange(index=2,left_inclusive=True, right_inclusive=True, variations=11)
            ])
        # config_file.config_param.update_variable("time_scan_us", scan_type="Variable", 
        #                                         new_initial_values=[0.01, 1.33-twopi_time/2, 2.87-twopi_time/2], 
        #                                         new_final_values=[0.01+twopi_time, 1.33+twopi_time/2, 2.87+twopi_time/2])
        config_file.config_param.update_variable("time_scan_us", scan_type="Variable", 
                                                new_initial_values=[0.01, 0.92-twopi_time/2, 1.69-twopi_time/2], 
                                                new_final_values=[0.01+twopi_time, 0.92+twopi_time/2, 1.69+twopi_time/2])
    else:
        config_file.config_param.update_scan_dimension(0, new_ranges=[
            ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=31)])
        config_file.config_param.update_variable("time_scan_us", scan_type="Variable", 
                                                new_initial_values=[0.01], 
                                                new_final_values=[0.01+2.40]) #4.8
    config_file.save()
    
    YEAR, MONTH, DAY = today()
    exp_name = f"RABI-SCAN-420AMP-{ryd420_amplitude:.3f}-{exp_idx}"
    # exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    # exp.open_master_script("\\ExperimentAutomation\\" + script_name)
    exp.open_configuration("\\CryoTweezerLoading\\" + config_name)
    exp.open_master_script("\\CryoTweezerLoading\\" + script_name)
    exp.run_experiment(exp_name)

    # Monitor experiment status
    aborted = experiment_monitoring(exp=exp, timeout_control=timeout_control)

    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    return aborted

def _raw_move_EOM_resonance(exp:ExperimentProcedure, start_freq, end_freq, step = 0.1, channel = 0):
    global CURRENT_EOM_FREQ
    if start_freq is None:
        start_freq = CURRENT_EOM_FREQ
    
    print(f"Move EOM frequency for channel {channel} from {start_freq:10.7f} MHz to {end_freq:10.7f} MHz")
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

    CURRENT_EOM_FREQ = end_freq

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
    # ryd420_amplitudes = [0.8,0.7,0.6,0.5,0.4,0.3,0.2,0.1]
    # ryd420_amplitudes = [0.2,0.1]
    ryd420_amplitudes = [-0.005,-0.004,-0.003,-0.002,-0.001,0,0.005,0.01,0.02,0.03,0.04,0.05,0.06,0.08,0.1]
    # ryd420_amplitudes = [0.01,0.02,0.03,0.04,0.05,0.06,0.08,0.1, -0.005,-0.004,-0.003,-0.002,-0.001,0,0]
    # ryd420_amplitudes = [-0.004,-0.003,-0.002,0.005]
    ryd420_amplitudes = [-0.005,-0.003,-0.001,0,0.005,0.01,0.03,0.05,0.06,0.08,0.1]
    ryd420_amplitudes = [0.06,0.08,0.1]

    for ryd420amp in ryd420_amplitudes:
        try:
            # recenter_beams()
            aborted = resonace_scan(exp_idx=exp_idx, ryd420_amplitude=ryd420amp, timeout_control = {'use':True, 'timeout':1200})
            # if aborted:
            #     exp.hardware_controller.restart_zynq_control()
            #     return
        except Exception as e:
            print(e)
            exp.hardware_controller.restart_zynq_control()
            # calibration(exp_idx)
            continue
        
        exp.hardware_controller.restart_zynq_control()

        try:
            # exp.hardware_controller.restart_zynq_control()
            # _calibration()
            # recenter_beams()
            aborted = rabi_scan(exp_idx=exp_idx, ryd420_amplitude=ryd420amp, timeout_control = {'use':True, 'timeout':2000}) #1500
            # if aborted:
            #     exp.hardware_controller.restart_zynq_control()
            #     return
            # sleep(3)
        except Exception as e:
            print(e)
            exp.hardware_controller.restart_zynq_control()
            continue

        exp.hardware_controller.restart_zynq_control()

        try:
            recenter_beams()
        except Exception as e:
            print(e)
            exp.hardware_controller.restart_zynq_control()
            continue


        # try:
        #     # exp.hardware_controller.restart_zynq_control()
        #     # _calibration()
        #     # recenter_beams()
        #     aborted = rabi_scan_twopi_time(exp_idx=exp_idx, ryd420_amplitude=ryd420amp, timeout_control = {'use':True, 'timeout':2000}) #1500
        #     # if aborted:
        #     #     exp.hardware_controller.restart_zynq_control()
        #     #     return
        #     # sleep(3)

        # except Exception as e:
        #     print(e)
        #     exp.hardware_controller.restart_zynq_control()
        #     continue

        # exp.hardware_controller.restart_zynq_control()


if __name__=='__main__':
    for idx in np.arange(1):
        # if idx<12: continue
        # if idx<12: continue
        print(f"Running experiment sets number {idx}")
        # if idx != 0:
        #     exp.hardware_controller.restart_zynq_control()
        calibration(1)

    # _raw_move_EOM_resonance(exp, start_freq=144,end_freq=185,step=0.25)
