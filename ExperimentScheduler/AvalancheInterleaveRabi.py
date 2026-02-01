from enum import Enum
from ExperimentProcedure import *
from RydbergBeamMoveProcedure import (
    move_beam_to_target,
    RYDBERG_BEAM_420_POSITION,
    RYDBERG_BEAM_1013_POSITION,
)
from UtilityFunctions import (
    set_AWG_avalanche,
    shuttle_grid_files,
    update_camera_image_dimension,
    set_configuration_2pic,
    set_configuration_3pic,
    set_AWG_rabi,
    set_AWG_avalanche,
    _raw_move_EOM_resonance,
)

from EthernetClient import EthernetClient

YEAR, MONTH, DAY = today()
exp = ExperimentProcedure()
# connection to SLM
client = EthernetClient.EthernetClient(host='10.10.0.14', port=8080)

config_name = "tweezerloading.Config"
config_path = "C:/Chimera/Chimera-Cryo/Configurations/CryoTweezerLoading/" + config_name
config_file = ConfigurationFile(config_path)

class AvalancheTimeScanConfig(Enum):
    FIVE_BY_ELEVEN_N60_RED = "5x11_n60_red"
    FIVE_BY_ELEVEN_N60_BLUE = "5x11_n60_blue"

class AvalancheInterleaveRabi:
    def __init__(self, ryd_420_amplitude, AVALANCHE_420_FREQ, RABI_420_FREQ, CURRENT_EOM_FREQ, avalanche_repetitions_arr):
        self.ryd_420_amplitude = ryd_420_amplitude
        self.AVALANCHE_420_FREQ = AVALANCHE_420_FREQ
        self.RABI_420_FREQ = RABI_420_FREQ
        self.CURRENT_EOM_FREQ = CURRENT_EOM_FREQ
        self.avalanche_repetitions_arr = avalanche_repetitions_arr

    def _update_avalanche_time_scan_parameter(self, config_file: ConfigurationFile, config_mode: AvalancheTimeScanConfig):
        if config_mode == AvalancheTimeScanConfig.FIVE_BY_ELEVEN_N60_BLUE:
            # for 5x11, n=60, blue
            config_file.config_param.update_scan_dimension(0, new_ranges=[
                ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=16),
                ScanRange(index=1,left_inclusive=True, right_inclusive=True, variations=5),
                ScanRange(index=2,left_inclusive=True, right_inclusive=True, variations=4)
                ])
            config_file.config_param.update_variable("time_scan_us", scan_type="Variable", 
                                                    new_initial_values=[0.01, 170, 270], 
                                                    new_final_values=[0.01+150, 250, 570])
        
        elif config_mode == AvalancheTimeScanConfig.FIVE_BY_ELEVEN_N60_RED:
            # for 5x11, n=60, red
            config_file.config_param.update_scan_dimension(0, new_ranges=[
                ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=16),
                ScanRange(index=1,left_inclusive=True, right_inclusive=True, variations=5),
                ScanRange(index=2,left_inclusive=True, right_inclusive=True, variations=3)
                ])
            config_file.config_param.update_variable("time_scan_us", scan_type="Variable", 
                                                    new_initial_values=[0.01, 1000, 4000], 
                                                    new_final_values=[0.01+900.01, 3000, 8000])
            
        else:
            raise ValueError(f"Unsupported avalanche time scan config: {config_mode}")


    def avalanche(self, exp_idx, exp_name_prefix, exp_name_postfix = '', timeout_control={"use": True, "timeout": 6000}):
        script_name = "rydberg_420_1013_excitation_SLM_dressing_rearrangement.mScript"

        # analysis grid for 5x20 grid - 20251223
        window = [0,0,170,54]
        thresholds = 105
        binnings = np.linspace(0, 240, 241)
        analysis_locs = da.DataAnalysis(year='2026', month='January', day='23', data_name='data_3', 
                                        window=window, thresholds=thresholds, binnings=binnings, multi_points_option = dict({"active":True, "search_square":4, "num_points":6}))
        
        grid_file_name = 'atomgrid_5x20_6points_20260129_SLM'
        camera_image_dim = {'Left:':971, 'Right:':1140, 'H-Bin:':1, 'Bottom:': 921, 'Top:': 974, 'V-Bin:': 1}
        tweezer_intensity_setpoint = 8.7 #V

        # move 420 frequency, config is saved, must run this first
        self.move_EOM_resonance(end_freq=self.AVALANCHE_420_FREQ, step=0.25, channel=0)

        # change ROI for 5 x 20 array, SLM tweezer intensity and grid file
        shuttle_grid_files(grid_file_name=grid_file_name)
        config_file.modify_parameter("DATA_ANALYSIS", "Grid File Name:", grid_file_name)
        update_camera_image_dimension(config_file=config_file, camera_image_dim=camera_image_dim)
        config_file.config_param.update_variable("slm_twz_intensity", constant_value=tweezer_intensity_setpoint)

        # set 3 pictures per repetition
        set_configuration_3pic(config_file)

        # set up the rest of the config file
        config_file.modify_parameter("REPETITIONS", "Reps:", str(self.avalanche_repetitions_arr[exp_idx]))
        config_file.modify_parameter("MAIN_OPTIONS", "Randomize Variations?", str(0))
        config_file.modify_parameter("MAIN_OPTIONS", "Repetition First Over Variation?", str(0))
        config_file.modify_parameter("STATIC_DDS", "Control?", 0)
        config_file.modify_parameter("MW1", "Control?", 0)
        config_file.modify_parameter("GMOOG", "Experiment Active:", 1)

        for variable in config_file.config_param.variables:
            config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)
        config_file.config_param.update_variable("ryd420_amplitude", constant_value=self.ryd_420_amplitude)
        config_file.config_param.update_variable("ryd1013_amplitude", constant_value=4)

        self._update_avalanche_time_scan_parameter(config_file=config_file, config_mode=AvalancheTimeScanConfig.FIVE_BY_ELEVEN_N60_RED)

        config_file.modify_parameter("MAKO3_CAM", "Mako System Active:", str(0))
        config_file.modify_parameter("MAKO3_CAM", "Exposure Time:", str(1035))
        config_file.modify_parameter("MAKO4_CAM", "Mako System Active:", str(0))
        config_file.modify_parameter("MAKO4_CAM", "Exposure Time:", str(1035))

        # set new phase pattern on SLM
        self.set_SLM_for_avalanche()

        # set AWGs
        set_AWG_avalanche(config_file, exp)
        config_file.save()

        # run experiment
        exp_name = f"AVALANCHE-{exp_name_prefix}-{exp_idx}{exp_name_postfix}"
        exp.open_configuration("\\CryoTweezerLoading\\" + config_name)
        exp.open_master_script("\\CryoTweezerLoading\\" + script_name)
        exp.run_experiment(exp_name)

        # Monitor experiment status
        aborted = experiment_monitoring(exp=exp, timeout_control=timeout_control)

        data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                                window=window, thresholds=thresholds, binnings=binnings, 
                                annotate_title = exp_name, annotate_note=" ")
        return aborted

    def rabi(self, exp_idx, exp_name_prefix, exp_name_postfix = '', timeout_control={"use": True, "timeout": 2000}):
        script_name = "rydberg_420_1013_excitation_SLM.mScript"

        # analysis grid for 1x7 grid - 20250930
        window = [0,0,90,20]
        thresholds = 100
        binnings = np.linspace(0, 240, 241)
        analysis_locs = da.DataAnalysis(year='2025', month='September', day='30', data_name='data_19', 
                                        window=window, thresholds=thresholds, binnings=binnings)
        
        grid_file_name = 'atomgrid_1x7_4points_2025-9-30'
        camera_image_dim = {'Left:':971, 'Right:':1150, 'H-Bin:':2, 'Bottom:': 929, 'Top:': 968, 'V-Bin:': 2}
        tweezer_intensity_setpoint = 0.61 #V

        # move 420 frequency, config is saved, must run this first
        self.move_EOM_resonance(end_freq=self.RABI_420_FREQ, step=0.25, channel=0)

        # change ROI for 1 x 7 array, SLM tweezer intensity and grid file
        shuttle_grid_files(grid_file_name=grid_file_name)
        config_file.modify_parameter("DATA_ANALYSIS", "Grid File Name:", grid_file_name)
        update_camera_image_dimension(config_file=config_file, camera_image_dim=camera_image_dim)
        config_file.config_param.update_variable("slm_twz_intensity", constant_value=tweezer_intensity_setpoint)

        # set 2 pictures per repetition
        set_configuration_2pic(config_file)

        # set up the config file
        config_file.modify_parameter("REPETITIONS", "Reps:", str(10))
        config_file.modify_parameter("MAIN_OPTIONS", "Randomize Variations?", str(0))
        config_file.modify_parameter("MAIN_OPTIONS", "Repetition First Over Variation?", str(1))
        config_file.modify_parameter("STATIC_DDS", "Control?", 0)
        config_file.modify_parameter("MW1", "Control?", 1)
        config_file.modify_parameter("GMOOG", "Experiment Active:", 0)

        for variable in config_file.config_param.variables:
            config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)
        config_file.config_param.update_variable("ryd420_amplitude", constant_value=self.ryd_420_amplitude)
        config_file.config_param.update_variable("ryd1013_amplitude", constant_value=4)
        config_file.config_param.update_scan_dimension(0, new_ranges=[
            ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=61),
            ])
        config_file.config_param.update_variable("time_scan_us", scan_type="Variable", 
                                                new_initial_values=[0.01], 
                                                new_final_values=[0.01+1.2])
        
        config_file.modify_parameter("MAKO3_CAM", "Mako System Active:", str(1))
        config_file.modify_parameter("MAKO3_CAM", "Exposure Time:", str(1035))
        config_file.modify_parameter("MAKO4_CAM", "Mako System Active:", str(1))
        config_file.modify_parameter("MAKO4_CAM", "Exposure Time:", str(1035))

        # set new phase pattern on SLM
        self.set_SLM_for_Rabi()

        # set AWGs
        set_AWG_rabi(config_file, exp)
        config_file.save()

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

    def move_EOM_resonance(self, end_freq, step=0.1, channel=0):
        if True: #start_freq is None:
            start_freq = self.CURRENT_EOM_FREQ
        _raw_move_EOM_resonance(exp=exp, start_freq=start_freq, end_freq=end_freq, step=step, channel=channel)
        self.CURRENT_EOM_FREQ = end_freq
        exp.save_all()
        config_file.reopen()

    def recenter_beams(self, exp: ExperimentProcedure):
        exp.setDAC()
        sleep(1)
        exp.setDDS()
        move_beam_to_target(exp=exp, mako_idx=3, pico_idx=(1,2), target_position=RYDBERG_BEAM_420_POSITION, tolerance=0.2)
        sleep(1)
        move_beam_to_target(exp=exp, mako_idx=4, pico_idx=(3,4), target_position=RYDBERG_BEAM_1013_POSITION, tolerance=0.2)
        sleep(1)

    def set_SLM_for_avalanche(self):
        client.connect()
        client.send(f"Phase-Pattern 5x20_20umx95um_trapBalanceCamera_cameraBalanced_balanced4")
        sleep(2)
        recv = client.receive()
        client.close()

        if recv.lower().startswith("success"):
            pass
        else:
            raise RuntimeError("Error in programming SLM for avalanche: " + recv)

    def set_SLM_for_Rabi(self):
        client.connect()
        client.send(f"Phase-Pattern 1x7_latticeconstant16.5um_trapBalanceCamera_cameraBalanced_balanced2")
        sleep(2)
        recv = client.receive()
        client.close()

        if recv.lower().startswith("success"):
            pass
        else:
            raise RuntimeError("Error in programming SLM for avalanche: " + recv)


if __name__ == "__main__":
    # n=60, blue, 1/30
    # RABI_420_FREQ = 578.55 
    # AVALANCHE_420_FREQ = RABI_420_FREQ + 16/2
    # CURRENT_EOM_FREQ = 578.55
    # RYD_420_AMPLITUDE = 0.02
    # EXP_NAME_PREFIX = "N-60-BLUE"

    # n=60, red, 1/30
    RABI_420_FREQ = 578.72 
    AVALANCHE_420_FREQ = RABI_420_FREQ - 36/2
    CURRENT_EOM_FREQ = 578.72
    RYD_420_AMPLITUDE = 0.081
    EXP_NAME_PREFIX = "N-60-RED"




    AVALANCHE_REPETITIONS = 600
    NUM_AVALANCHE_SEGMENTS = 6


    quotient, remainder = divmod(AVALANCHE_REPETITIONS, NUM_AVALANCHE_SEGMENTS)
    avalanche_repetitions_arr = [
        quotient + (1 if i < remainder else 0) 
        for i in range(NUM_AVALANCHE_SEGMENTS)
    ]

    experiment = AvalancheInterleaveRabi(
        ryd_420_amplitude=RYD_420_AMPLITUDE,
        AVALANCHE_420_FREQ=AVALANCHE_420_FREQ,
        RABI_420_FREQ=RABI_420_FREQ,
        CURRENT_EOM_FREQ = CURRENT_EOM_FREQ,
        avalanche_repetitions_arr=avalanche_repetitions_arr,
    )

    for exp_idx, _ in enumerate(avalanche_repetitions_arr):
        eid = 0
        exp_postfix = ''
        while True:
            try:
                aborted = experiment.avalanche(exp_idx=exp_idx, exp_name_prefix=EXP_NAME_PREFIX, exp_name_postfix=exp_postfix,
                                               timeout_control={"use": True, "timeout": 6000})
                if aborted:
                    # exp.hardware_controller.restart_zynq_control()
                    sleep(5)
                break
            except Exception as e:
                print(e)
                eid += 1
                exp_postfix = f'-{eid}'
                print(f"Avalanche failed at experiment run number {exp_idx}")
                print("Attempting recovery and retrying with incremented exp_idx...")
                sleep(10)
                exp.hardware_controller.restart_zynq_control()
        exp.hardware_controller.restart_zynq_control()

        eid = 0
        exp_postfix = ''
        while True:
            try:
                aborted = experiment.rabi(exp_idx=exp_idx, exp_name_prefix=EXP_NAME_PREFIX, exp_name_postfix=exp_postfix,
                                        timeout_control={"use": True, "timeout": 2000})
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
                exp.hardware_controller.restart_zynq_control()
        exp.hardware_controller.restart_zynq_control()

        trial_num = 0
        while True:
            try:
                experiment.recenter_beams(exp)
                break
            except Exception as e:
                print(e)
                exp.hardware_controller.restart_zynq_control()
                trial_num += 1
                if trial_num>=3:
                    print("Tried to recetner the beam in Avalanche experiment for 3 times but it failed for all 3 !!!!")
                    break
        exp.hardware_controller.restart_zynq_control()

