import numpy as np
from ExperimentProcedure import *
from ExperimentProcedure import experiment_monitoring, analog_in_calibration_monitoring
from RydbergBeamMoveProcedure import move_beam_to_target, RYDBERG_BEAM_420_POSITION, RYDBERG_BEAM_1013_POSITION
from EthernetClient import EthernetClient
import time

YEAR, MONTH, DAY = today()
exp = ExperimentProcedure()

config_name = "AODinduced_lightshift_on_SLM.Config"
config_path = exp.CONFIGURATION_DIR + config_name
config_file = ConfigurationFile(config_path)

config_file.modify_parameter("REPETITIONS", "Reps:", str(75))
for variable in config_file.config_param.variables:
    config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    
config_file.config_param.update_variable("resonance_scan", scan_type="Variable", new_initial_values=[20], new_final_values=[50])
config_file.config_param.update_scan_dimension(0, range_index=0, variations=17)

exp.open_configuration("\\ExperimentAutomation\\" + config_name)

# analysis grid
window = [0,0,25,40]
thresholds = 100
binnings = np.linspace(0, 240, 241)
analysis_locs = da.DataAnalysis(year='2025', month='June', day='6', data_name='data_6', 
                                window=window, thresholds=100, binnings=binnings)
# connection to SLM
client = EthernetClient.EthernetClient(host='6.1.1.71', port=8080)


def trap_depth_scan(amplitude, AODon=True, postfix="", timeout_control = {'use':False, 'timeout':1000}):
    script_name_AODon = "tweezerloading_rearrangement_lightshift_from_AOD.mScript"
    script_name_AODoff = "tweezerloading_rearrangement_lightshift_from_AOD_AODoff.mScript"

    if AODon:
        script_name = script_name_AODon
    else:
        script_name = script_name_AODoff

    # Update configuration
    config_file.modify_parameter("REPETITIONS", "Reps:", str(12))
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_scan_dimension(0, new_ranges=[ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=17)])
    config_file.config_param.update_variable("resonance_scan", scan_type="Variable", new_initial_values=[20], new_final_values=[50])
    # config_file.config_param.update_variable("time_scan_us", constant_value = 0.18)
    config_file.save()
    
    # Setup experiment details
    YEAR, MONTH, DAY = today()
    exp_name = f"AODLIGHTSHIFT-SCAN-DEFOCUSING-AMPLITUDE-{amplitude:.3f}"
    if AODon:
        exp_name = f"{exp_name}-AODON"
    else:
        exp_name = f"{exp_name}-AODOFF"
    if postfix!="":
        exp_name = f"{exp_name}-{postfix}"


    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.open_master_script("\\ExperimentAutomation\\" + script_name)
    exp.run_experiment(exp_name)
    
    # Monitor experiment status
    experiment_monitoring(exp=exp, timeout_control=timeout_control)

    # Analyze the data
    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    # try:                   
    #     analysis_result = data_analysis.analyze_data()
    #     optimal_field = analysis_result[1]
    #     print(f"Optimal resoance for {exp_name} is {optimal_field:.3S} ")
    #     fit_fail=False
    # except Exception as e:
    #     print(e)
    #     fit_fail=True
    # fit_fail = (optimal_field.s > 1) or (analysis_result[0].n > 0) or (analysis_result[0].n < -2)
    # if fit_fail:
    #     print(f"Optimal resoance {optimal_field:.3S} has a variance larger than 1 or {analysis_result[0]:.3S} is outside the normal range, this typically means bad data.")
    return  False #fit_fail

def trap_depth_scan_2D(amplitude, postfix="", timeout_control = {'use':False, 'timeout':4800}):
    script_name = "tweezerloading_rearrangement_lightshift_from_AOD.mScript"
    gscript_name = "AOD_rearrangement_2x19.gScript"

    # Update configuration
    config_file.modify_parameter("REPETITIONS", "Reps:", str(6))
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_variable("aod_dac0_freq", scan_type="Variable", scan_dimension=0, new_initial_values=[-0.1], new_final_values=[0.1])
    config_file.config_param.update_variable("aod_dac1_freq", scan_type="Variable", scan_dimension=1, new_initial_values=[-0.1], new_final_values=[0.1])
    config_file.config_param.update_variable("resonance_scan", scan_type="Variable", scan_dimension=2, new_initial_values=[20], new_final_values=[50])

    config_file.config_param.update_scan_dimension(0, new_ranges=[ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=6)])
    config_file.config_param.update_scan_dimension(1, new_ranges=[ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=6)])
    config_file.config_param.update_scan_dimension(2, new_ranges=[ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=13)])
    
    config_file.save()
    
    # Setup experiment details
    YEAR, MONTH, DAY = today()
    exp_name = f"AODLIGHTSHIFT-SCAN-DEFOCUSING-2D-AMPLITUDE-{amplitude:.3f}"
    if postfix!="":
        exp_name = f"{exp_name}-{postfix}"


    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.open_master_script("\\ExperimentAutomation\\" + script_name)
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
    amplitudes = np.linspace(-1.5,1.5, 4)
    ZERNIKE_IDX = 4
    ZERNIKE_CMD = "Zernike"

    for idx, amp in enumerate(amplitudes):
        print(f"Running experiment sets number {idx}, amp {amp}")
        # if idx<6: continue
        client.send(f"{ZERNIKE_CMD} {ZERNIKE_IDX} {amp:.2f}")
        recv = client.receive()
        if recv.lower().startswith("success"):
            trap_depth_scan_2D(amplitude=amp, postfix="")
            sleep(5)
            exp.hardware_controller.restart_zynq_control()
            trap_depth_scan(AODon=False, amplitude=amp, postfix="")
        else:
            print(recv)
            break
        sleep(5)
        exp.hardware_controller.restart_zynq_control()
        
    client.close()

if __name__=='__main__':
    procedure()