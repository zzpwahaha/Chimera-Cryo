from enum import Enum
from ExperimentProcedure import *
from RydbergBeamMoveProcedure import (
    move_beam_to_target,
    RYDBERG_BEAM_420_POSITION,
    RYDBERG_BEAM_1013_POSITION,
)
from UtilityFunctions import _raw_move_EOM_resonance
import SLMAODTransverseOffsetAlignment as aod_align
from Random import SDG2042X_NOISEmode

YEAR, MONTH, DAY = today()
exp = ExperimentProcedure()

config_name = "tweezerloading_manyBodyRydbergLifetimeDressingNoStrobo.Config"
config_path = "C:/Chimera/Chimera-Cryo/Configurations/CryoTweezerLoading/" + config_name
config_file = ConfigurationFile(config_path)

# analysis grid for 5x20 grid - 20251223
window = [0,0,170,54]
thresholds = 105
binnings = np.linspace(0, 240, 4*240+1)
binnings = np.linspace(80, 120, 161)
analysis_locs = da.DataAnalysis(year='2026', month='May', day='19', data_name='data_1', n_cluster_row=5,
                                window=window, thresholds=thresholds, binnings=binnings, 
                                multi_points_option = dict({"active":True, "search_square":4, "num_points":6}))


class LifetimeInterleaveRabi:
    def __init__(self, lifetime_repetitions_arr, AVALANCHE_420_FREQ, RABI_420_FREQ, 
                 CURRENT_EOM_FREQ, RYDBERG_420_SETPOINT, RYDBERG_1013_SETPOINT, 
                 RYDBERG_RABI_PI_TIME, RYDBERG_RABI_SCAN_TIME):
        self.lifetime_repetitions_arr = lifetime_repetitions_arr
        self.AVALANCHE_420_FREQ = AVALANCHE_420_FREQ
        self.RABI_420_FREQ = RABI_420_FREQ
        self.CURRENT_EOM_FREQ = CURRENT_EOM_FREQ
        self.RYDBERG_420_SETPOINT = RYDBERG_420_SETPOINT
        self.RYDBERG_1013_SETPOINT = RYDBERG_1013_SETPOINT
        self.RYDBERG_RABI_PI_TIME = RYDBERG_RABI_PI_TIME
        self.RYDBERG_RABI_SCAN_TIME = RYDBERG_RABI_SCAN_TIME
        aod_align.setup(aod_align.config_file)

        s = """00000000000000000000
00000000000000000000
10010010010010010010
00000000000000000000
00000000000000000000"""

        # locs_selection = np.array([char == '1' for char in s], dtype=bool)
        locs_selection = np.array([[c == '1' for c in line] for line in s.splitlines()])
        self.locs_selection = locs_selection.flatten()


        pass
    # def __init__(self, ryd_420_amplitude, AVALANCHE_420_FREQ, RABI_420_FREQ, CURRENT_EOM_FREQ, avalanche_repetitions_arr):
        # self.ryd_420_amplitude = ryd_420_amplitude
        # self.AVALANCHE_420_FREQ = AVALANCHE_420_FREQ
        # self.RABI_420_FREQ = RABI_420_FREQ
        # self.CURRENT_EOM_FREQ = CURRENT_EOM_FREQ
        # self.avalanche_repetitions_arr = avalanche_repetitions_arr

    def lifetime(self, exp_idx, exp_name_prefix, exp_name_postfix = '', timeout_control={"use": True, "timeout": 6000}):
        script_name = "rydberg_420_1013_excitation_SLM_dressing_rearrangement_nostrobing.mScript"
        gscript_name = "C:/Chimera/Chimera-Cryo/Configurations/CryoTweezerLoading/rearrangement_5x7.gScript"

        exp.open_configuration("\\CryoTweezerLoading\\" + config_name)
        self.move_EOM_resonance(end_freq=self.AVALANCHE_420_FREQ, step=0.25, channel=0)

        # set up the rest of the config file
        config_file.modify_parameter("REPETITIONS", "Reps:", str(self.lifetime_repetitions_arr[exp_idx])) #
        config_file.modify_parameter("MAIN_OPTIONS", "Randomize Variations?", str(0))
        config_file.modify_parameter("MAIN_OPTIONS", "Repetition First Over Variation?", str(0))
        config_file.modify_parameter("STATIC_DDS", "Control?", 0)
        config_file.modify_parameter("MW1", "Control?", 1)
        config_file.modify_parameter("GMOOG", "Experiment Active:", 1)
        config_file.modify_parameter("GMOOG", "Scripted Arb Address:", gscript_name)


        for variable in config_file.config_param.variables:
            config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)
        config_file.config_param.update_variable("ryd420_amplitude", constant_value=self.RYDBERG_420_SETPOINT) #-0.0093 #-0.0069 # -0.0072 #-0.0096
        config_file.config_param.update_variable("ryd1013_amplitude", constant_value=self.RYDBERG_1013_SETPOINT) #4, 2.25
        config_file.config_param.update_scan_dimension(0, new_ranges=[
            ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=11),
            ScanRange(index=1,left_inclusive=True, right_inclusive=True, variations=16),
            ScanRange(index=2,left_inclusive=True, right_inclusive=True, variations=4),])
        config_file.config_param.update_variable("time_scan_us", scan_type="Variable", 
                                                # new_initial_values=[0.01,220,500], new_final_values=[200.01,420,2000])
                                                # new_initial_values=[0.01,110,400], new_final_values=[100.01,310,1400])
                                                # new_initial_values=[0.01,55,170], new_final_values=[50.01,155,420])
                                                new_initial_values=[0.01,29.5,122], new_final_values=[25.01,97,197]) # 50, 5x14
                                                # new_initial_values=[0.01,45,230], new_final_values=[35.01,195,335]) # 50, 5x5
                                                # new_initial_values=[0.01,31,154], new_final_values=[25.01,121,253]) # 50, 5x10
                                                # new_initial_values=[0.01,36,146], new_final_values=[30.01,126,266]) # 70, 5x14
                                                # new_initial_values=[0.01,47,172], new_final_values=[40.01,152,292]) # 70, 5x10
                                                # new_initial_values=[0.01,72,292], new_final_values=[60.01,252,412]) # 70, 5x5


        config_file.modify_parameter("MAKO3_CAM", "Mako System Active:", str(0))
        config_file.modify_parameter("MAKO3_CAM", "Exposure Time:", str(1035))
        config_file.modify_parameter("MAKO4_CAM", "Mako System Active:", str(0))
        config_file.modify_parameter("MAKO4_CAM", "Exposure Time:", str(1035))

        config_file.save()

        YEAR, MONTH, DAY = today()
        # run experiment
        exp_name = f"LIFETIME-{exp_name_prefix}-{exp_idx}{exp_name_postfix}"
        exp.open_configuration("\\CryoTweezerLoading\\" + config_name)
        exp.open_master_script("\\CryoTweezerLoading\\" + script_name)

        exp.run_experiment(exp_name)

        # Monitor experiment status
        aborted = experiment_monitoring(exp=exp, timeout_control=timeout_control)

        data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                                window=window, thresholds=thresholds, binnings=binnings, 
                                annotate_title = exp_name, annotate_note=" ")
        return aborted

    def rabi(self, exp_idx, exp_name_prefix, exp_name_postfix = '', timeout_control={"use": True, "timeout": 1800}):
        script_name = "rydberg_420_1013_excitation_SLM_dressing_rearrangement_nostrobing_Rabi.mScript"
        gscript_name = "C:/Chimera/Chimera-Cryo/Configurations/CryoTweezerLoading/rearrangement_1x7_Rabi.gScript"
        exp.open_configuration("\\CryoTweezerLoading\\" + config_name)

        self.move_EOM_resonance(end_freq=self.RABI_420_FREQ, step=0.25, channel=0)

        # set up the config file
        config_file.modify_parameter("REPETITIONS", "Reps:", str(10))
        config_file.modify_parameter("MAIN_OPTIONS", "Randomize Variations?", str(1))
        config_file.modify_parameter("MAIN_OPTIONS", "Repetition First Over Variation?", str(1))
        config_file.modify_parameter("STATIC_DDS", "Control?", 0)
        config_file.modify_parameter("MW1", "Control?", 1)
        config_file.modify_parameter("GMOOG", "Experiment Active:", 1)
        config_file.modify_parameter("GMOOG", "Scripted Arb Address:", gscript_name)

        for variable in config_file.config_param.variables:
            config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)
        config_file.config_param.update_variable("ryd420_amplitude", constant_value=self.RYDBERG_420_SETPOINT) #-0.0093 #-0.0069 # -0.0072 #-0.0096
        config_file.config_param.update_variable("ryd1013_amplitude", constant_value=self.RYDBERG_1013_SETPOINT) #4, 2.25
        config_file.config_param.update_scan_dimension(0, new_ranges=[
            ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=31), #61
            ])
        config_file.config_param.update_variable("time_scan_us", scan_type="Variable", 
                                                new_initial_values=[0.01], 
                                                new_final_values=[0.01+self.RYDBERG_RABI_SCAN_TIME])
        
        config_file.modify_parameter("MAKO3_CAM", "Mako System Active:", str(1))
        config_file.modify_parameter("MAKO3_CAM", "Exposure Time:", str(1035))
        config_file.modify_parameter("MAKO3_CAM", "Trigger Mode:", "Line1")
        config_file.modify_parameter("MAKO4_CAM", "Mako System Active:", str(1))
        config_file.modify_parameter("MAKO4_CAM", "Exposure Time:", str(1035))
        config_file.modify_parameter("MAKO4_CAM", "Trigger Mode:", "Line1")

        config_file.save()

        YEAR, MONTH, DAY = today()
        # run experiment
        exp_name = f"RABI-{exp_name_prefix}-{exp_idx}{exp_name_postfix}"
        exp.open_configuration("\\CryoTweezerLoading\\" + config_name)
        exp.open_master_script("\\CryoTweezerLoading\\" + script_name)
                
        exp.run_experiment(exp_name)

        # Monitor experiment status
        aborted = experiment_monitoring(exp=exp, timeout_control=timeout_control)

        data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                                window=window, thresholds=thresholds, binnings=binnings, 
                                annotate_title = exp_name, annotate_note=" ")
        return aborted

    def resonance(self, exp_idx, exp_name_prefix, exp_name_postfix = '', timeout_control={"use": True, "timeout": 1800}):
        script_name = "rydberg_420_1013_excitation_SLM_dressing_rearrangement_nostrobing_Rabi.mScript"
        gscript_name = "C:/Chimera/Chimera-Cryo/Configurations/CryoTweezerLoading/rearrangement_1x7_Rabi.gScript"

        self.move_EOM_resonance(end_freq=self.RABI_420_FREQ-1.5, step=0.25, channel=0)
        config_file.modify_parameter("STATIC_DDS", "Control?", 1)
        config_file.modify_parameter("STATIC_DDS", " DDS-0 Value:", f'{self.RABI_420_FREQ:.3f}-1.5+ryd420_eom_resonance')


        # set up the config file
        config_file.modify_parameter("REPETITIONS", "Reps:", str(12))
        config_file.modify_parameter("MAIN_OPTIONS", "Randomize Variations?", str(0))
        config_file.modify_parameter("MAIN_OPTIONS", "Repetition First Over Variation?", str(1))
        config_file.modify_parameter("MW1", "Control?", 1)
        config_file.modify_parameter("GMOOG", "Experiment Active:", 1)
        config_file.modify_parameter("GMOOG", "Scripted Arb Address:", gscript_name)

        for variable in config_file.config_param.variables:
            config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)
        config_file.config_param.update_variable("ryd420_amplitude", constant_value=self.RYDBERG_420_SETPOINT) #-0.0093 #-0.0069 # -0.0072 #-0.0096
        config_file.config_param.update_variable("ryd1013_amplitude", constant_value=self.RYDBERG_1013_SETPOINT) #4, 2.25
        config_file.config_param.update_variable("time_scan_us", constant_value=self.RYDBERG_RABI_PI_TIME)
        config_file.config_param.update_scan_dimension(0, new_ranges=[
            ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=31), #61
            ])
        config_file.config_param.update_variable("ryd420_eom_resonance", scan_type="Variable", 
                                                new_initial_values=[0], 
                                                new_final_values=[3])
        
        config_file.modify_parameter("MAKO3_CAM", "Mako System Active:", str(1))
        config_file.modify_parameter("MAKO3_CAM", "Exposure Time:", str(1035))
        config_file.modify_parameter("MAKO3_CAM", "Trigger Mode:", "Line1")
        config_file.modify_parameter("MAKO4_CAM", "Mako System Active:", str(1))
        config_file.modify_parameter("MAKO4_CAM", "Exposure Time:", str(1035))
        config_file.modify_parameter("MAKO4_CAM", "Trigger Mode:", "Line1")

        config_file.save()

        YEAR, MONTH, DAY = today()
        # run experiment
        exp_name = f"RESONANCE-{exp_name_prefix}-{exp_idx}{exp_name_postfix}"
        exp.open_configuration("\\CryoTweezerLoading\\" + config_name)
        exp.open_master_script("\\CryoTweezerLoading\\" + script_name)

        exp.run_experiment(exp_name)
        # Monitor experiment status
        aborted = experiment_monitoring(exp=exp, timeout_control=timeout_control)

        self.CURRENT_EOM_FREQ = self.RABI_420_FREQ-1.5+3
        self.move_EOM_resonance(end_freq=self.RABI_420_FREQ, step=0.25, channel=0)
        data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                                window=window, thresholds=thresholds, binnings=binnings, n_cluster_row=analysis_locs.n_cluster_row,
                                annotate_title = exp_name, annotate_note=" ")
        try:                   
            analysis_result = data_analysis.analyze_data(function=da.sinc_sq, locs_selection=self.locs_selection)
            optimal_field = analysis_result[1]
            print(f"Optimal resoance for {exp_name} is {optimal_field:.3S} ")
            fit_fail=False
        except Exception as e:
            print(e)
            fit_fail=True
        fit_fail = (optimal_field.s > 1) or (analysis_result[0].n > 1.5) or (analysis_result[0].n < -1.5)
        if fit_fail:
            print(f"Optimal resoance {optimal_field:.3S} has a variance larger than 1 or {analysis_result[0]:.3S} is outside the normal range, this typically means bad data.")
        else:
            RABI_420_FREQ = round(self.RABI_420_FREQ-1.5+optimal_field.n, 3)
            self.move_EOM_resonance(end_freq=RABI_420_FREQ, step=0.25, channel=0)
            self.RABI_420_FREQ = RABI_420_FREQ
            self.AVALANCHE_420_FREQ = self.RABI_420_FREQ - 1
        return aborted

    def AOD_alignment(self, exp_idx, exp_name_prefix, exp_name_postfix = '', timeout_control={"use": True, "timeout": 1800}):

        fit_fail = \
        aod_align.alignment(exp=exp, config_file=aod_align.config_file, master_file=aod_align.master_file,
                            exp_idx=exp_idx, x_or_y='y', postfix=exp_name_prefix+'AVALANCHE'+exp_name_postfix,
                            timeout_control=timeout_control)
        if fit_fail:
            print('FAILED IN ALIGNING AOD WITH SLM FOR Y')
        sleep(5)

        fit_fail = \
        aod_align.alignment(exp=exp, config_file=aod_align.config_file, master_file=aod_align.master_file,
                            exp_idx=exp_idx, x_or_y='x', postfix=exp_name_prefix+'AVALANCHE'+exp_name_postfix,
                            timeout_control=timeout_control)
        if fit_fail:
            print('FAILED IN ALIGNING AOD WITH SLM FOR X')

    def recenter_beams(self, exp: ExperimentProcedure):
        exp.setDAC()
        sleep(1)
        exp.setDDS()
        move_beam_to_target(exp=exp, mako_idx=3, pico_idx=(1,2), target_position=RYDBERG_BEAM_420_POSITION, tolerance=0.1)
        sleep(1)
        move_beam_to_target(exp=exp, mako_idx=4, pico_idx=(3,4), target_position=RYDBERG_BEAM_1013_POSITION, tolerance=(0.1,0.01))
        sleep(1)

    def move_EOM_resonance(self, end_freq, step=0.1, channel=0):
        if True: #start_freq is None:
            start_freq = self.CURRENT_EOM_FREQ
        _raw_move_EOM_resonance(exp=exp, start_freq=start_freq, end_freq=end_freq, step=step, channel=channel)
        self.CURRENT_EOM_FREQ = end_freq
        exp.save_all()
        sleep(1)
        config_file.reopen()


if __name__ == "__main__":
    RYDBERG_420_SETPOINT = 0.024 #0.012 #0.002 #-0.005 #-0.0099 #-0.0093 #-0.0069 # -0.0072 #-0.0096
    RYDBERG_1013_SETPOINT = 2.25 #4, 2.25
    RYDBERG_RABI_PI_TIME = 0.41 #0.57 #0.73 #0.91 #0.41
    RYDBERG_RABI_SCAN_TIME = 3.6 #4.2 #5.4 #7.2 #3.6

    SDG2042X_IP = "TCPIP0::10.10.0.50::INSTR"
    EXP_NAME_PREFIX_BASE = "N-50-0.01-5x14-0.0V"

    NUM_LIFETIME_SEGMENTS = 3
    LIFETIME_REPETITIONS = 60*NUM_LIFETIME_SEGMENTS
    RABI_420_FREQ = 583.5 #577.624 #587. #583.8 #582.795 #587.9 #585.037 #583.868 #529.08 #578.048   #578.091 #577.977 #578.096 #576.334 #578.271 #578.656 #578.56 # MHz
    AVALANCHE_420_FREQ = RABI_420_FREQ-1 # MHz

    CURRENT_EOM_FREQ = 583.5+1.5  #RABI_420_FREQ

    quotient, remainder = divmod(LIFETIME_REPETITIONS, NUM_LIFETIME_SEGMENTS)
    lifetime_repetitions_arr = [
        quotient + (1 if i < remainder else 0) 
        for i in range(NUM_LIFETIME_SEGMENTS)
    ]

    experiment = LifetimeInterleaveRabi(
        lifetime_repetitions_arr=lifetime_repetitions_arr,
        CURRENT_EOM_FREQ = CURRENT_EOM_FREQ,
        AVALANCHE_420_FREQ = AVALANCHE_420_FREQ,
        RABI_420_FREQ = RABI_420_FREQ,
        RYDBERG_420_SETPOINT = RYDBERG_420_SETPOINT,
        RYDBERG_1013_SETPOINT = RYDBERG_1013_SETPOINT,
        RYDBERG_RABI_PI_TIME = RYDBERG_RABI_PI_TIME,
        RYDBERG_RABI_SCAN_TIME = RYDBERG_RABI_SCAN_TIME,
    )

    # INJECTED_NOISE = [0,2,50,5,10,25,] #mV
    # INJECTED_NOISE = [15,20,30] #mV
    # INJECTED_NOISE = [400, 0, 192, 50, 300, 96] #mV
    # INJECTED_NOISE = [25,75,] #mV
    INJECTED_NOISE = [125,300,75] #mV

    sdg2042 = SDG2042X_NOISEmode.SDG2042XNoise(resource=SDG2042X_IP, channel=1, output_load="HZ")
    sdg2042.enable_noise(std_rms=0, mean=0)

    for idn, injected_noise in enumerate(INJECTED_NOISE):
        EXP_NAME_PREFIX = EXP_NAME_PREFIX_BASE + f'-NOISE{injected_noise:.1f}mV'
        sdg2042.set_std(injected_noise*1e-3)
        sdg2042.query_settings()

        for exp_idx, _ in enumerate(lifetime_repetitions_arr):
            # if exp_idx in np.arange(2): continue
            
            eid = 0
            exp_postfix = ''
            while True:
                if exp_idx in [0,]: break
                try:
                    aborted = experiment.AOD_alignment(exp_idx=exp_idx, exp_name_prefix=EXP_NAME_PREFIX, exp_name_postfix=exp_postfix,
                                                timeout_control={"use": True, "timeout": 1800})
                    if aborted:
                        # exp.hardware_controller.restart_zynq_control()
                        sleep(5)
                    break
                except Exception as e:
                    print(e)
                    eid += 1
                    exp_postfix = f'-{eid}'
                    print(f"Lifetime failed at experiment run number {exp_idx}")
                    print("Attempting recovery and retrying with incremented exp_idx...")
                    sleep(10)
                    # exp.hardware_controller.restart_zynq_control()

            eid = 0
            exp_postfix = ''
            while True:
                if exp_idx in [0,]: break
                try:
                    aborted = experiment.lifetime(exp_idx=exp_idx, exp_name_prefix=EXP_NAME_PREFIX, exp_name_postfix=exp_postfix,
                                                timeout_control={"use": True, "timeout": 7000})
                    if aborted:
                        # exp.hardware_controller.restart_zynq_control()
                        sleep(5)
                    break
                except Exception as e:
                    print(e)
                    eid += 1
                    exp_postfix = f'-{eid}'
                    print(f"Lifetime failed at experiment run number {exp_idx}")
                    print("Attempting recovery and retrying with incremented exp_idx...")
                    sleep(10)
                    # exp.hardware_controller.restart_zynq_control()

            eid = 0
            exp_postfix = ''
            while True:
                # if exp_idx==0: break
                try:
                    aborted = experiment.resonance(exp_idx=exp_idx, exp_name_prefix=EXP_NAME_PREFIX, exp_name_postfix=exp_postfix,
                                            timeout_control={"use": True, "timeout": 1800})
                    if aborted:
                        # exp.hardware_controller.restart_zynq_control()
                        sleep(5)
                    break
                except Exception as e:
                    print(e)
                    eid += 1
                    exp_postfix = f'-{eid}'
                    print(f"Rabi scan failed at experiment run number {exp_idx}")
                    print("Attempting recovery and retrying with incremented exp_idx...")
                    sleep(10)
                    # exp.hardware_controller.restart_zynq_control()


            eid = 0
            exp_postfix = ''
            while True:
                try:
                    aborted = experiment.rabi(exp_idx=exp_idx, exp_name_prefix=EXP_NAME_PREFIX, exp_name_postfix=exp_postfix,
                                            timeout_control={"use": True, "timeout": 1800})
                    if aborted:
                        # exp.hardware_controller.restart_zynq_control()
                        sleep(5)
                    break
                except Exception as e:
                    print(e)
                    eid += 1
                    exp_postfix = f'-{eid}'
                    print(f"Rabi scan failed at experiment run number {exp_idx}")
                    print("Attempting recovery and retrying with incremented exp_idx...")
                    sleep(10)
                    # exp.hardware_controller.restart_zynq_control()

            for _ in range(3):
                trial_num = 0
                while True:
                    try:
                        experiment.recenter_beams(exp)
                        break
                    except Exception as e:
                        print(e)
                        # exp.hardware_controller.restart_zynq_control()
                        trial_num += 1
                        if trial_num>=3:
                            print("Tried to recetner the beam in Avalanche experiment for 3 times but it failed for all 3 !!!!")
                            break
                sleep(30)

