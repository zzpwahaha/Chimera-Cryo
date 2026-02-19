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

config_file.modify_parameter("REPETITIONS", "Reps:", str(50))
config_file.modify_parameter("MAIN_OPTIONS", "Randomize Variations?", str(0))
config_file.modify_parameter("MAIN_OPTIONS", "Repetition First Over Variation?", str(0))


for variable in config_file.config_param.variables:
    config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
# config_file.config_param.update_variable("ryd1013_amplitude", constant_value=4)

exp.open_configuration("\\CryoTweezerLoading\\" + config_name)

# analysis grid
window = [0,0,90,20]
thresholds = 100
binnings = np.linspace(0, 240, 241)
analysis_locs = da.DataAnalysis(year='2025', month='September', day='30', data_name='data_19', 
                                window=window, thresholds=thresholds, binnings=binnings)

def defocusing_scan(amplitude, AODon=True, postfix="", timeout_control = {'use':True, 'timeout':1000}):
    script_name = "rydberg_420_1013_excitation_AOD_withBoBSLM.mScript"

    # Update configuration    
    config_file.modify_parameter("REPETITIONS", "Reps:", str(50))
    config_file.modify_parameter("MAIN_OPTIONS", "Randomize Variations?", str(0))
    config_file.modify_parameter("MAIN_OPTIONS", "Repetition First Over Variation?", str(0))
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    # config_file.config_param.update_scan_dimension(0, new_ranges=[ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=17)])
    # config_file.config_param.update_variable("resonance_scan", scan_type="Variable", new_initial_values=[20], new_final_values=[50])
    # config_file.config_param.update_variable("time_scan_us", constant_value = 0.18)
    config_file.save()
    
    # Setup experiment details
    YEAR, MONTH, DAY = today()
    exp_name = f"SLM-DEFOCUSING-AMPLITUDE-{amplitude:.3f}" + (f'-{postfix}' if postfix else "")
    exp.open_configuration("\\CryoTweezerLoading\\" + config_name)
    exp.open_master_script("\\CryoTweezerLoading\\" + script_name)
    exp.run_experiment(exp_name)
    
    # Monitor experiment status
    experiment_monitoring(exp=exp, timeout_control=timeout_control)

    # Analyze the data
    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")

    return  False #fit_fail


def procedure():

    client.connect()
    # amplitudes = np.linspace(-0.75,1.5, 10)
    # amplitudes = np.linspace(-1.75,-1, 4)
    # amplitudes = np.linspace(-1.5,1.5, 4)
    # amplitudes = np.array([-2,-1, 0, 1, 2, 2.5])
    # amplitudes = np.array([0, -1.5, -1, -0.5, 0.5, 1, 1.5, 2, 2.5])
    amplitudes = np.array([-5,-4,-3,-2,-1,0,1,2,3,4,5])
    # amplitudes = np.array([3,4,5])
    # amplitudes = np.array([0.5,1.5,2.5,3.5])
    # amplitudes = np.array([3.5])
    ZERNIKE_IDX = 4
    ZERNIKE_CMD = "Zernike"

    for idx, amp in enumerate(amplitudes):
        print(f"Running experiment sets number {idx}, amp {amp}")
        # if idx<6: continue
        client.send(f"{ZERNIKE_CMD} {ZERNIKE_IDX} {amp:.2f}")
        recv = client.receive()
        if recv.lower().startswith("success"):
            defocusing_scan(amplitude=amp, postfix="")
            sleep(5)
            # exp.hardware_controller.restart_zynq_control()
            # trap_depth_scan(AODon=False, amplitude=amp, postfix="")
        else:
            print(recv)
            break
        sleep(5)
        # exp.hardware_controller.restart_zynq_control()
        
    client.close()

if __name__=='__main__':
    procedure()