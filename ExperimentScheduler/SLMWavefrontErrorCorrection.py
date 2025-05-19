import numpy as np
from ExperimentProcedure import *
from ExperimentProcedure import experiment_monitoring, analog_in_calibration_monitoring
from RydbergBeamMoveProcedure import move_beam_to_target, RYDBERG_BEAM_420_POSITION, RYDBERG_BEAM_1013_POSITION
from EthernetClient import EthernetClient
import time

YEAR, MONTH, DAY = today()
exp = ExperimentProcedure()

config_name = "SLMTrapFrequency.Config"
config_path = exp.CONFIGURATION_DIR + config_name
config_file = ConfigurationFile(config_path)

config_file.modify_parameter("REPETITIONS", "Reps:", str(15))
for variable in config_file.config_param.variables:
    config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    
config_file.config_param.update_variable("twz_mod_freq", scan_type="Variable", new_initial_values=[150], new_final_values=[300])
config_file.config_param.update_scan_dimension(0, range_index=0, variations=21)

exp.open_configuration("\\ExperimentAutomation\\" + config_name)

# analysis grid
window = [0, 0, 210, 210]
thresholds = 60
binnings = np.linspace(0, 240, 241)
analysis_locs = da.DataAnalysis(year='2025', month='May', day='14', data_name='data_8', 
                                window=window, thresholds=70, binnings=binnings)
# connection to SLM
client = EthernetClient.EthernetClient(host='6.1.1.71', port=8080)


def trapFrequency_scan(zernike_idx, amplitude, postfix="", timeout_control = {'use':False, 'timeout':1200}):
    script_name = "tweezerloading_trapFreq.mScript"

    # Update configuration
    config_file.modify_parameter("REPETITIONS", "Reps:", str(13))
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_scan_dimension(0, new_ranges=[ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=21)])
    config_file.config_param.update_variable("twz_mod_freq", scan_type="Variable", new_initial_values=[150], new_final_values=[300])
    # config_file.config_param.update_variable("time_scan_us", constant_value = 0.18)
    config_file.save()
    
    # Setup experiment details
    YEAR, MONTH, DAY = today()
    if postfix=="":
        exp_name = f"TRAPFREQUENCY-SCAN-ZERNIKE-{zernike_idx}-AMPLITUDE-{amplitude:.3f}"
    else:
        exp_name = f"TRAPFREQUENCY-SCAN-ZERNIKE-{zernike_idx}-AMPLITUDE-{amplitude:.3f}-{postfix}"

    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.open_master_script("\\ExperimentAutomation\\" + script_name)
    exp.run_experiment(exp_name)
    
    # Monitor experiment status
    experiment_monitoring(exp=exp, timeout_control=timeout_control)

    # Analyze the data
    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    try:                   
        analysis_result = data_analysis.analyze_data()
        optimal_field = analysis_result[1]
        print(f"Optimal resoance for {exp_name} is {optimal_field:.3S} ")
        fit_fail=False
    except Exception as e:
        print(e)
        fit_fail=True
    # fit_fail = (optimal_field.s > 1) or (analysis_result[0].n > 0) or (analysis_result[0].n < -2)
    # if fit_fail:
    #     print(f"Optimal resoance {optimal_field:.3S} has a variance larger than 1 or {analysis_result[0]:.3S} is outside the normal range, this typically means bad data.")
    return  fit_fail


def procedure():
    client.connect()
    amplitudes = np.linspace(-1.5,1.5,6)
    # ZERNIKE_IDX = 12
    ZERNIKE_CMD = "Zernike_pure"
    postfix="2"
    # zernike_indices = [
    #     3,5, # astigmatism
    #     7,8, # comma
    #     6,9, # trefoil
    #     11,13, # 2nd astigmatism
    #     10,14, # quadrafoil
    #     17,18, # 2nd coma ?
    #     24, # rho^6 
    #     23,25, # 4th order astigmatism
    #     ]
    zernike_indices = [25]
    for ZERNIKE_IDX in zernike_indices:
        for idx, amp in enumerate(amplitudes):
            print(f"Running experiment sets number {idx}, amp {amp}")
            client.send(f"{ZERNIKE_CMD} {ZERNIKE_IDX} {amp:.2f}")
            recv = client.receive()
            if recv.lower().startswith("success"):
                max_retries = 4
                for attempt in range(max_retries):
                    fit_fail = trapFrequency_scan(zernike_idx=ZERNIKE_IDX, amplitude=amp, postfix=postfix)
                    if not fit_fail:
                        break  # Exit loop if scan was successful
                    else:
                        exp.hardware_controller.restart_zynq_control()
            else:
                print(recv)
                break
            sleep(5)
            exp.hardware_controller.restart_zynq_control()
        
    client.close()

if __name__=='__main__':
    procedure()