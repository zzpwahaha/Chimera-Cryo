from enum import Enum
from ExperimentProcedure import *
from RydbergBeamMoveProcedure import (
    move_beam_to_target,
    RYDBERG_BEAM_420_POSITION,
    RYDBERG_BEAM_1013_POSITION,
)


YEAR, MONTH, DAY = today()
exp = ExperimentProcedure()

config_name = "tweezerloading_singleBodyRydbergLifetime.Config"
config_path = "C:/Chimera/Chimera-Cryo/Configurations/CryoTweezerLoading/" + config_name
config_file = ConfigurationFile(config_path)

# analysis grid for 1x4 grid - 20260224
window = [0,0,90,20]
thresholds = 100
binnings = np.linspace(0, 240, 241)
analysis_locs = da.DataAnalysis(year='2026', month='February', day='24', data_name='data_13', 
                                window=window, thresholds=thresholds, binnings=binnings, 
                                multi_points_option = dict({"active":True, "search_square":2, "num_points":4}))


class LifetimeInterleaveRabi:
    def __init__(self, lifetime_repetitions_arr):
        self.lifetime_repetitions_arr = lifetime_repetitions_arr
        pass
    # def __init__(self, ryd_420_amplitude, AVALANCHE_420_FREQ, RABI_420_FREQ, CURRENT_EOM_FREQ, avalanche_repetitions_arr):
        # self.ryd_420_amplitude = ryd_420_amplitude
        # self.AVALANCHE_420_FREQ = AVALANCHE_420_FREQ
        # self.RABI_420_FREQ = RABI_420_FREQ
        # self.CURRENT_EOM_FREQ = CURRENT_EOM_FREQ
        # self.avalanche_repetitions_arr = avalanche_repetitions_arr

    def lifetime(self, exp_idx, exp_name_prefix, exp_name_postfix = '', timeout_control={"use": True, "timeout": 6000}):
        script_name = "rydberg_420_1013_excitation_SLM_forLifetime.mScript"
        # script_name = "rydberg_420_1013_excitation_SLM_forLifetime_static1013.mScript"

        # set up the rest of the config file
        config_file.modify_parameter("REPETITIONS", "Reps:", str(self.lifetime_repetitions_arr[exp_idx])) #
        config_file.modify_parameter("MAIN_OPTIONS", "Randomize Variations?", str(1))
        config_file.modify_parameter("MAIN_OPTIONS", "Repetition First Over Variation?", str(0))
        config_file.modify_parameter("STATIC_DDS", "Control?", 0)
        config_file.modify_parameter("MW1", "Control?", 1)
        config_file.modify_parameter("GMOOG", "Experiment Active:", 0)

        for variable in config_file.config_param.variables:
            config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)
        config_file.config_param.update_variable("ryd420_amplitude", constant_value=0.02) #0.1 #0.02 #0.04
        config_file.config_param.update_variable("ryd1013_amplitude", constant_value=2) #4 #0.25
        config_file.config_param.update_scan_dimension(0, new_ranges=[
            ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=16),])
        config_file.config_param.update_variable("time_scan_us", scan_type="Variable", 
                                                new_initial_values=[5], new_final_values=[35])

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
        script_name = "rydberg_420_1013_excitation_SLM_forLifetime_Rabi.mScript"

        # set up the config file
        config_file.modify_parameter("REPETITIONS", "Reps:", str(15))
        config_file.modify_parameter("MAIN_OPTIONS", "Randomize Variations?", str(0))
        config_file.modify_parameter("MAIN_OPTIONS", "Repetition First Over Variation?", str(0))
        config_file.modify_parameter("STATIC_DDS", "Control?", 0)
        config_file.modify_parameter("MW1", "Control?", 1)
        config_file.modify_parameter("GMOOG", "Experiment Active:", 0)

        for variable in config_file.config_param.variables:
            config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)
        config_file.config_param.update_variable("ryd420_amplitude", constant_value=0.04)
        config_file.config_param.update_variable("ryd1013_amplitude", constant_value=0.25)
        config_file.config_param.update_scan_dimension(0, new_ranges=[
            ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=21), #61
            ])
        config_file.config_param.update_variable("time_scan_us", scan_type="Variable", 
                                                new_initial_values=[0.01], 
                                                new_final_values=[0.01+1.0])
        
        config_file.modify_parameter("MAKO3_CAM", "Mako System Active:", str(1))
        config_file.modify_parameter("MAKO3_CAM", "Exposure Time:", str(1035))
        config_file.modify_parameter("MAKO4_CAM", "Mako System Active:", str(1))
        config_file.modify_parameter("MAKO4_CAM", "Exposure Time:", str(1035))

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

    def recenter_beams(self, exp: ExperimentProcedure):
        exp.setDAC()
        sleep(1)
        exp.setDDS()
        move_beam_to_target(exp=exp, mako_idx=3, pico_idx=(1,2), target_position=RYDBERG_BEAM_420_POSITION, tolerance=0.1)
        sleep(1)
        move_beam_to_target(exp=exp, mako_idx=4, pico_idx=(3,4), target_position=RYDBERG_BEAM_1013_POSITION, tolerance=(0.1,0.01))
        sleep(1)


if __name__ == "__main__":

    EXP_NAME_PREFIX = "N-55-RIGHTALIGNED-CRYO-SHIELDANDOR"


    NUM_LIFETIME_SEGMENTS = 16*2
    LIFETIME_REPETITIONS = 60*NUM_LIFETIME_SEGMENTS


    quotient, remainder = divmod(LIFETIME_REPETITIONS, NUM_LIFETIME_SEGMENTS)
    lifetime_repetitions_arr = [
        quotient + (1 if i < remainder else 0) 
        for i in range(NUM_LIFETIME_SEGMENTS)
    ]

    experiment = LifetimeInterleaveRabi(
        lifetime_repetitions_arr=lifetime_repetitions_arr,
    )

    for exp_idx, _ in enumerate(lifetime_repetitions_arr):
        # if exp_idx in [0,1,]: continue
        eid = 0
        exp_postfix = ''
        while True:
            try:
                aborted = experiment.lifetime(exp_idx=exp_idx, exp_name_prefix=EXP_NAME_PREFIX, exp_name_postfix=exp_postfix,
                                               timeout_control={"use": True, "timeout": 6000/2})
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

        # eid = 0
        # exp_postfix = ''
        # while True:
        #     try:
        #         aborted = experiment.rabi(exp_idx=exp_idx, exp_name_prefix=EXP_NAME_PREFIX, exp_name_postfix=exp_postfix,
        #                                 timeout_control={"use": True, "timeout": 1800})
        #         if aborted:
        #             # exp.hardware_controller.restart_zynq_control()
        #             sleep(5)
        #         break
        #     except Exception as e:
        #         print(e)
        #         eid += 1
        #         exp_postfix = f'-{eid}'
        #         print(f"Rabi scan failed at experiment run number {exp_idx}")
        #         print("Attempting recovery and retrying with incremented exp_idx...")
        #         sleep(10)
        #         # exp.hardware_controller.restart_zynq_control()

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

