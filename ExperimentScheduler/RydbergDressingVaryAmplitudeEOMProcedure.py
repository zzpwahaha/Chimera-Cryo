import numpy as np
from ExperimentProcedure import *
from RydbergBeamMoveProcedure import move_beam_to_target, RYDBERG_BEAM_420_POSITION, RYDBERG_BEAM_1013_POSITION
from fitters import linear
import uncertainties.core as unc
import uncertainties.unumpy as unp
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
CURRENT_EOM_FREQ = 516 #525 #530
# RYD420_CONVERSION = unc.ufloat_fromstr('7.19(14)e+02'), unc.ufloat_fromstr('9.402(87)e+00')
# RYD420_CONVERSION = unc.ufloat_fromstr('4.77(12)e+02'), unc.ufloat_fromstr('6.271(75)e+00')
RYD420_CONVERSION = unc.ufloat_fromstr('3.04(12)e+02'), unc.ufloat_fromstr('3.58(17)e+00')

BETA = 0.15
# HALF_TIME = {'-0.005': np.float64(97.07375381376578),
#  '-0.003': np.float64(118.32516985774157),
#  '-0.001': np.float64(183.7555163431352),
#  '0.010': np.float64(435.3844516453014),
#  '0.030': np.float64(96.09729725894383),
#  '0.050': np.float64(369.78184872123387),
#  '0.080': np.float64(279.7777702610356),
#  '0.100': np.float64(241.65286525982086),
#  '0.120': np.float64(496.34959547275815),
#  '0.150': np.float64(469.68165191644954)}

# HALF_TIME = {'-0.005': np.float64(1474.753305016479),
#  '-0.001': np.float64(935.1431468480947),
#  '0.010': np.float64(349.2696500449695),
#  '0.030': np.float64(180.5009161932799),
#  '0.050': np.float64(120.64769907626498),
#  '0.080': np.float64(39.07568245572416),
#  '0.100': np.float64(49.210618602599645),
#  '0.120': np.float64(46.24624702058691),
#  '0.150': np.float64(38.85005808368469),
#  '0.200': np.float64(37.091387317043264)}

HALF_TIME = {'-0.005': np.float64(18000),
 '-0.001': np.float64(6196.581896353601),
 '0.010': np.float64(3120.131163804183),
 '0.030': np.float64(1668.8413254652428),
 '0.050': np.float64(1251.593466233498),
 '0.080': np.float64(684.0562720454367),
 '0.100': np.float64(248.99326117303394),
 '0.120': np.float64(258.45258260455017),
 '0.150': np.float64(258.1690619155114),
 '0.200': np.float64(288.53771719787636)}

def round_for_divisor(x, base, n_digits):
    """
    Round x so that (x / base) has n_digits after the decimal point.
    """
    step = base * 10**(-n_digits)
    return round(x / step) * step

def dressing_time_scan(exp_idx, ryd420_amplitude, detuning = None, timeout_control = {'use':True, 'timeout':12000}):
    global CURRENT_EOM_FREQ
    script_name = "rydberg_420_1013_excitation_SLM_dressing_rearrangement.mScript"
    half_time = HALF_TIME[f'{ryd420_amplitude:.3f}']
    scan_time = round(round_for_divisor(half_time*4, 20,-2),2)+0.01

    AOM_CENTER_FREQ = 75 #MHz
    if detuning is None:
        ryd420_rabi = unp.sqrt(linear.f(ryd420_amplitude, *RYD420_CONVERSION)).item().nominal_value
        detuning = ryd420_rabi/(2*BETA)  # further divided by 2 to account for 840-420
    
    # _raw_move_EOM_resonance(exp=exp, start_freq=CURRENT_EOM_FREQ, end_freq=EOM_CENTER_FREQ-detuning/2, step=0.25, channel=0)

    # Update configuration
    config_file.modify_parameter("REPETITIONS", "Reps:", str(100))
    config_file.modify_parameter("MAIN_OPTIONS", "Randomize Variations?", str(0))
    config_file.modify_parameter("MAIN_OPTIONS", "Repetition First Over Variation?", str(1))

    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_scan_dimension(0, new_ranges=[ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=21)])
    # config_file.config_param.update_variable("time_scan_us", scan_type="Variable", new_initial_values=[round(0.8*half_time,1)], new_final_values=[round(1.2*half_time,1)])
    # config_file.config_param.update_variable("time_scan_us", scan_type="Variable", new_initial_values=[2.01], new_final_values=[12.01])
    # config_file.config_param.update_variable("time_scan_us", scan_type="Variable", new_initial_values=[5.01], new_final_values=[45.01])
    # config_file.config_param.update_variable("time_scan_us", scan_type="Variable", new_initial_values=[0.01], new_final_values=[240.01])
    # config_file.config_param.update_variable("time_scan_us", scan_type="Variable", new_initial_values=[0.01], new_final_values=[750.01])
    config_file.config_param.update_variable("time_scan_us", scan_type="Variable", new_initial_values=[0.01], new_final_values=[scan_time])
    config_file.config_param.update_variable("ryd420_amplitude", constant_value = ryd420_amplitude)
    config_file.config_param.update_variable("ryd420_resonance", constant_value = AOM_CENTER_FREQ)
    config_file.save()
    
    # Setup experiment details
    YEAR, MONTH, DAY = today()
    exp_name = f"DRESSING-TIME-SCAN-420AMP-FIXED-RED-DETUNED-{ryd420_amplitude:.3f}-{exp_idx}"
    exp.open_configuration("\\CryoTweezerLoading\\" + config_name)
    exp.open_master_script("\\CryoTweezerLoading\\" + script_name)

    exp.run_experiment(exp_name)
    
    # Monitor experiment status
    aborted = experiment_monitoring(exp=exp, timeout_control=timeout_control)

    # Analyze the data
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
    # ryd420_amplitudes = [-0.005,-0.004,-0.003,-0.002,-0.001,0,0.005,0.01,0.02,0.03,0.04,0.05,0.06,0.08,0.1]
    # ryd420_amplitudes = [0.01,0.02,0.03,0.04,0.05, -0.005,-0.004,-0.003,-0.002,-0.001,0]
    # ryd420_amplitudes = [-0.005,0.05,-0.003,-0.001,0]

    # ryd420_amplitudes = [-0.005]
    # ryd420_amplitudes = [-0.005,-0.003,-0.001, 0.01,0.03,0.05,0.08,0.1, 0.12, 0.15]
    # # ryd420_amplitudes = [0.01,0.03,0.05,0.06,0.08,0.1,0.12,0.15]
    # ryd420_amplitudes = [0.08,0.1,0.12,0.15]
    # ryd420_amplitudes = [0.18,0.20,0.23,0.25]
    # ryd420_amplitudes = [-0.005,-0.001, 0.01,0.03,0.05,0.08,0.1, 0.12, 0.15, 0.18,0.20,0.23,0.25]
    # ryd420_amplitudes = [0.20,0.25]
    ryd420_amplitudes = np.array([-0.005,-0.001, 0.01,0.03,0.05,0.08,0.1, 0.12, 0.15, 0.2])
    ryd420_amplitudes = np.array([0.01,0.03,0.05,0.08,0.1, 0.12, 0.15, 0.2])
    ryd420_amplitudes = np.array([0.03,0.05,0.08,0.1, 0.12, 0.15, 0.2])
    ryd420_amplitudes = np.array([-0.005,-0.001,])

    
    ryd420_rabis = unp.nominal_values(unp.sqrt(linear.f(np.array(ryd420_amplitudes), *RYD420_CONVERSION)))
    detunings = ryd420_rabis/(2*BETA)  # further divided by 2 to account for 840-420

    # detunings = [10,15,20,25,30]
    # detunings = [20,25,30]

    for ryd420amp in ryd420_amplitudes:
    # for detuning in detunings:
        try:
            # aborted = dressing_time_scan(exp_idx=exp_idx, ryd420_amplitude=ryd420amp, timeout_control = {'use':True, 'timeout':10000})
            aborted = dressing_time_scan(exp_idx=exp_idx, ryd420_amplitude=ryd420amp, timeout_control = {'use':True, 'timeout':6000})
            # aborted = dressing_time_scan(exp_idx=exp_idx, ryd420_amplitude=ryd420amp, timeout_control = {'use':True, 'timeout':1800})
            # aborted = dressing_time_scan(exp_idx=exp_idx, ryd420_amplitude=0.1, detuning=detuning, timeout_control = {'use':True, 'timeout':6000})
            # if aborted:
            #     exp.hardware_controller.restart_zynq_control()
            #     return
        except Exception as e:
            print(e)
            exp.hardware_controller.restart_zynq_control()
            # calibration(exp_idx)
            continue
        try:
            recenter_beams()
        except Exception as e:
            print(e)
            exp.hardware_controller.restart_zynq_control()
            continue

        exp.hardware_controller.restart_zynq_control()



if __name__=='__main__':
    for idx in np.arange(1):
        # if idx<12: continue
        # if idx<12: continue
        print(f"Running experiment sets number {idx}")
        # if idx != 0:
        #     exp.hardware_controller.restart_zynq_control()
        calibration(1)

    # _raw_move_EOM_resonance(exp, start_freq=144,end_freq=185,step=0.25)
