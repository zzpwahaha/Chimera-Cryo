from enum import Enum
from ExperimentProcedure import *
from RydbergBeamMoveProcedure import (
    move_beam_to_target,
    RYDBERG_BEAM_420_POSITION,
    RYDBERG_BEAM_1013_POSITION,
)
from UtilityFunctions import _raw_move_EOM_resonance
import SLMAODTransverseOffsetAlignment as aod_align

YEAR, MONTH, DAY = today()
exp = ExperimentProcedure()

config_name = "MWdressing.Config"
config_path = "C:/Chimera/Chimera-Cryo/Configurations/MWDressing/" + config_name
config_file = ConfigurationFile(config_path)

# analysis grid for 5x20 grid - 20251223
window = [0,0,170,54]
thresholds = 105
binnings = np.linspace(0, 240, 4*240+1)
binnings = np.linspace(80, 120, 161)
analysis_locs = da.DataAnalysis(year='2026', month='July', day='17', data_name='data_2', n_cluster_row=5,
                                window=window, thresholds=thresholds, binnings=binnings, 
                                multi_points_option = dict({"active":True, "search_square":4, "num_points":6}))


class ResonanceWithMWDressing:
    def __init__(self, RABI_420_FREQ, CURRENT_EOM_FREQ):
        self.RABI_420_FREQ = RABI_420_FREQ
        self.CURRENT_EOM_FREQ = CURRENT_EOM_FREQ
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

    # def rabi(self, exp_idx, exp_name_prefix, exp_name_postfix = '', timeout_control={"use": True, "timeout": 1800}):
    #     script_name = "rydberg_420_1013_excitation_SLM_rearrangement.mScript"
    #     gscript_name = "C:/Chimera/Chimera-Cryo/Configurations/MWDressing/rearrangement_1x7_Rabi.gScript"
    #     exp.open_configuration("\\MWDressing\\" + config_name)

    #     self.move_EOM_resonance(end_freq=self.RABI_420_FREQ, step=0.25, channel=0)

    #     # set up the config file
    #     config_file.modify_parameter("REPETITIONS", "Reps:", str(20))
    #     config_file.modify_parameter("MAIN_OPTIONS", "Randomize Variations?", str(1))
    #     config_file.modify_parameter("MAIN_OPTIONS", "Repetition First Over Variation?", str(1))
    #     config_file.modify_parameter("STATIC_DDS", "Control?", 0)
    #     config_file.modify_parameter("MW1", "Control?", 1)
    #     config_file.modify_parameter("GMOOG", "Experiment Active:", 1)
    #     config_file.modify_parameter("GMOOG", "Scripted Arb Address:", gscript_name)

    #     for variable in config_file.config_param.variables:
    #         config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)
    #     config_file.config_param.update_variable("ryd420_amplitude", constant_value=0.02) #-0.0072 #-0.0096
    #     config_file.config_param.update_variable("ryd1013_amplitude", constant_value=4)
    #     config_file.config_param.update_scan_dimension(0, new_ranges=[
    #         ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=31), #61
    #         ])
    #     config_file.config_param.update_variable("time_scan_us", scan_type="Variable", 
    #                                             new_initial_values=[0.01], 
    #                                             new_final_values=[0.01+1.2])
        
    #     config_file.modify_parameter("MAKO3_CAM", "Mako System Active:", str(1))
    #     config_file.modify_parameter("MAKO3_CAM", "Exposure Time:", str(1035))
    #     config_file.modify_parameter("MAKO3_CAM", "Trigger Mode:", "Line1")
    #     config_file.modify_parameter("MAKO4_CAM", "Mako System Active:", str(1))
    #     config_file.modify_parameter("MAKO4_CAM", "Exposure Time:", str(1035))
    #     config_file.modify_parameter("MAKO4_CAM", "Trigger Mode:", "Line1")

    #     config_file.save()

    #     YEAR, MONTH, DAY = today()
    #     # run experiment
    #     exp_name = f"RABI-{exp_name_prefix}-{exp_idx}{exp_name_postfix}"
    #     exp.open_configuration("\\MWDressing\\" + config_name)
    #     exp.open_master_script("\\MWDressing\\" + script_name)
                
    #     exp.run_experiment(exp_name)

    #     # Monitor experiment status
    #     aborted = experiment_monitoring(exp=exp, timeout_control=timeout_control)

    #     data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
    #                             window=window, thresholds=thresholds, binnings=binnings, 
    #                             annotate_title = exp_name, annotate_note=" ")
    #     return aborted

    def resonance(self, exp_idx, amplitude, frequency, exp_name_prefix, exp_name_postfix = '', timeout_control={"use": True, "timeout": 1800}):
        script_name = "rydberg_420_1013_excitation_SLM_rearrangement.mScript"
        gscript_name = "C:/Chimera/Chimera-Cryo/Configurations/MWDressing/rearrangement_1x7_Rabi.gScript"

        SCAN_RANGE = 4 # [-SCAN_RANGE, SCAN_RANGE]

        self.move_EOM_resonance(end_freq=self.RABI_420_FREQ-SCAN_RANGE, step=0.25, channel=0)
        config_file.modify_parameter("STATIC_DDS", "Control?", 1)
        config_file.modify_parameter("STATIC_DDS", " DDS-0 Value:", f'{self.RABI_420_FREQ:.3f}+ryd420_eom_resonance')

        # set up the config file
        config_file.modify_parameter("REPETITIONS", "Reps:", str(3))
        config_file.modify_parameter("MAIN_OPTIONS", "Randomize Variations?", str(0))
        config_file.modify_parameter("MAIN_OPTIONS", "Repetition First Over Variation?", str(1))
        config_file.modify_parameter("MW1", "Control?", 1)
        config_file.modify_parameter("MWAnt", "Control?", 1)
        config_file.sections['MWAnt'].mw_lists[0].set_parameter('Freq:', 'ryd60s60p12_resonance+resonance_scan')
        config_file.sections['MWAnt'].mw_lists[0].set_parameter('Power:', '-25')

        config_file.modify_parameter("GMOOG", "Experiment Active:", 1)
        config_file.modify_parameter("GMOOG", "Scripted Arb Address:", gscript_name)

        for variable in config_file.config_param.variables:
            config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)
        config_file.config_param.update_variable("ryd420_amplitude", constant_value=0.024) #0.02 # -0.0072 #-0.0096
        config_file.config_param.update_variable("ryd1013_amplitude", constant_value=4)
        config_file.config_param.update_variable("time_scan_us", constant_value=0.41)
        config_file.config_param.update_variable("resonance_scan", constant_value=frequency)
        # config_file.config_param.update_variable("ryd_ionization_resonance", constant_value=frequency)
        config_file.config_param.update_scan_dimension(0, new_ranges=[
            ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=32), #61
            ])
        config_file.config_param.update_variable("ryd420_eom_resonance", scan_type="Variable", 
                                                new_initial_values=[-SCAN_RANGE], 
                                                new_final_values=[SCAN_RANGE])
        
        # config_file.modify_parameter("MAKO3_CAM", "Mako System Active:", str(0))
        # config_file.modify_parameter("MAKO3_CAM", "Exposure Time:", str(1035))
        # config_file.modify_parameter("MAKO3_CAM", "Trigger Mode:", "Line1")
        # config_file.modify_parameter("MAKO4_CAM", "Mako System Active:", str(0))
        # config_file.modify_parameter("MAKO4_CAM", "Exposure Time:", str(1035))
        # config_file.modify_parameter("MAKO4_CAM", "Trigger Mode:", "Line1")

        config_file.save()

        YEAR, MONTH, DAY = today()
        # run experiment
        exp_name = f"RESONANCE-{exp_name_prefix}-{exp_name_postfix}"
        exp.open_configuration("\\MWDressing\\" + config_name)
        exp.open_master_script("\\MWDressing\\" + script_name)

        exp.run_experiment(exp_name)
        # Monitor experiment status
        aborted = experiment_monitoring(exp=exp, timeout_control=timeout_control)

        self.CURRENT_EOM_FREQ = self.RABI_420_FREQ+SCAN_RANGE
        self.move_EOM_resonance(end_freq=self.RABI_420_FREQ, step=0.25, channel=0)
        data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                                window=window, thresholds=thresholds, binnings=binnings, n_cluster_row=analysis_locs.n_cluster_row,
                                annotate_title = exp_name, annotate_note=" ")
        
        # try:                   
        #     analysis_result = data_analysis.analyze_data(function=da.sinc_sq, locs_selection=self.locs_selection)
        #     optimal_field = analysis_result[1]
        #     print(f"Optimal resoance for {exp_name} is {optimal_field:.3S} ")
        #     fit_fail=False
        # except Exception as e:
        #     print(e)
        #     fit_fail=True
        # fit_fail = (optimal_field.s > 1) or (analysis_result[0].n > 1.5) or (analysis_result[0].n < -1.5)
        # if fit_fail:
        #     print(f"Optimal resoance {optimal_field:.3S} has a variance larger than 1 or {analysis_result[0]:.3S} is outside the normal range, this typically means bad data.")
        # else:
        #     RABI_420_FREQ = round(self.RABI_420_FREQ-1.5+optimal_field.n, 3)
        #     self.move_EOM_resonance(end_freq=RABI_420_FREQ, step=0.25, channel=0)
        #     self.RABI_420_FREQ = RABI_420_FREQ
        #     self.AVALANCHE_420_FREQ = self.RABI_420_FREQ - 1
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

    EXP_NAME_PREFIX = "N-60-MWDRESSING-7G-60P12--25dBm"

    RABI_420_FREQ = 577 
    CURRENT_EOM_FREQ = 577  #RABI_420_FREQ

    experiment = ResonanceWithMWDressing(
        CURRENT_EOM_FREQ = CURRENT_EOM_FREQ,
        RABI_420_FREQ = RABI_420_FREQ
    )
    # amplitudes = [15,12,9,6,3,0,-3,-6]
    frequencies = np.linspace(-30,30,31)
    # frequencies = np.concatenate([np.linspace(36,50,8), np.linspace(-50,-42,5)])

    for exp_idx, freq in enumerate(frequencies):        
        eid = 0
        exp_postfix = ''
        while True:
            if (exp_idx%15 != 0)  : break #or (exp_idx==0)
            # if True: break
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
            try:
                aborted = experiment.resonance(exp_idx=exp_idx, amplitude = 0, frequency=freq,
                                            exp_name_prefix=EXP_NAME_PREFIX + f'-MW-FREQ-{freq:.1f}MHz', exp_name_postfix=exp_postfix,
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

        for _ in range(1):
            trial_num = 0
            if (exp_idx%10 != 0) or (exp_idx==0) : break #
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

