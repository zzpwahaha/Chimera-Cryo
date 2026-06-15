import numpy as np
from ExperimentProcedure import *
import time


YEAR, MONTH, DAY = today()
exp = ExperimentProcedure()

config_name = "Rydberg_Rabi_Ramsey.Config"
config_path = exp.CONFIGURATION_DIR + config_name
config_file = ConfigurationFile(config_path)

config_file.modify_parameter("REPETITIONS", "Reps:", str(7))
for variable in config_file.config_param.variables:
    config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    
config_file.config_param.update_variable("resonance_scan", scan_type="Variable", new_initial_values=[76], new_final_values=[84])
config_file.config_param.update_scan_dimension(0, range_index=0, variations=33)

# # analysis grid
# window = [0, 0, 200, 30]
# thresholds = 65
# binnings = np.linspace(0, 240, 241)
# analysis_locs = da.DataAnalysis(year='2024', month='August', day='26', data_name='data_1', 
#                                 window=window, thresholds=70, binnings=binnings)

# analysis grid for 2x7 grid - 20250922
# window = [0,0,90,20]
# thresholds = 100
# binnings = np.linspace(0, 240, 241)
# analysis_locs = da.DataAnalysis(year='2025', month='September', day='19', data_name='data_13', 
#                                 window=window, thresholds=thresholds, binnings=binnings)


def resonace_scan(exp_idx, exp_name_prefix, timeout_control = {'use':False, 'timeout':600}):
    script_name = "Calibration_rydberg_420_1013_excitation.mScript"

    # Update configuration
    config_file.modify_parameter("REPETITIONS", "Reps:", str(4))
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_scan_dimension(0, new_ranges=[ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=22)])
    config_file.config_param.update_variable("resonance_scan", scan_type="Variable", new_initial_values=[73], new_final_values=[87])
    config_file.save()
    
    # Setup experiment details
    YEAR, MONTH, DAY = today()
    exp_name = f"{exp_name_prefix}-{exp_idx}" #EFIELD-RESONANCE-SCAN-Z-MINUS
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.open_master_script("\\ExperimentAutomation\\" + script_name)
    exp.run_experiment(exp_name)
    
    # Monitor experiment status
    experiment_monitoring(exp=exp, timeout_control=timeout_control)

    # Analyze the data
    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    analysis_result = data_analysis.analyze_data()
    optimal_field = analysis_result[1]
    print(f"Optimal resoance for {exp_name} is {optimal_field:.3S} ")

    fit_fail = (optimal_field.s > 1) or (analysis_result[0].n > 0) or (analysis_result[0].n < -2)
    if fit_fail:
        raise ValueError(f"Optimal resoance {optimal_field:.3S} has a variance larger than 1 or {analysis_result[0]:.3S} is outside the normal range, this typically means bad data.")

    # # Update configuration with the optimal field
    # config_file.config_param.update_variable("resonance_scan", constant_value = round(optimal_field.n, 3))
    # config_file.save()
    return fit_fail

def _calibration():
    exp.setZynqOutput()
    analog_in_calibration(exp=exp, name = "prb_pwr")
    exp.save_all()
    config_file.reopen()

def _raw_move_EOM_resonance(exp:ExperimentProcedure, start_freq, end_freq, step = 0.1, channel = 0):
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

def _move_EOM_resonance(start_freq, end_freq, step = 0.1, channel = 0):
    _raw_move_EOM_resonance(exp=exp, start_freq=start_freq, end_freq=end_freq, step=step, channel = channel)
    exp.save_all()
    config_file.reopen()

def high_to_low_direction(exp_idx, channel = 0):
    EOM_center_freqs = np.linspace(225, 255, 4)
    previous_f = 225
    for idx, eom_f in enumerate(EOM_center_freqs):
        _move_EOM_resonance(previous_f, eom_f, step=0.1, channel = channel)
        previous_f = eom_f
        _calibration()
        sleep(1)
        try:
            resonace_scan(exp_idx=exp_idx, exp_name_prefix=f"RESONANCE-SCAN-EOM{eom_f}-DESCEND")
        except:
            sleep(10)
            exp.hardware_controller.restart_zynq_control()
        sleep(10)
        exp.hardware_controller.restart_zynq_control()

def low_to_high_direction(exp_idx, channel = 0):
    # EOM_center_freqs = np.linspace(255, 225, 4)
    # previous_f = 255

    EOM_center_freqs = np.linspace(141.5, 241.5, 11)
    previous_f = EOM_center_freqs[0]

    for idx, eom_f in enumerate(EOM_center_freqs):
        _move_EOM_resonance(previous_f, eom_f, step=0.1, channel = channel)
        previous_f = eom_f
        # _calibration()
        sleep(1)
        try:
            resonace_scan(exp_idx=exp_idx, exp_name_prefix=f"RESONANCE-SCAN-EOM{eom_f}-ASCEND")
        except:
            sleep(2)
            exp.hardware_controller.restart_zynq_control()
        sleep(2)
        exp.hardware_controller.restart_zynq_control()


def efield_tracing_procedure(channel = 0):
    for idx in np.arange(1):
        # if idx<=0: continue
        # print(f"Running experiment sets number {idx}")
        # if idx != 0:
        #     exp.hardware_controller.restart_zynq_control()
        # high_to_low_direction(0)
        low_to_high_direction(0,channel=channel)


if __name__=='__main__':
    # efield_tracing_procedure()
    # _raw_move_EOM_resonance(exp, start_freq=581,end_freq=586,step=0.1)
    # _raw_move_EOM_resonance(exp, start_freq=205,end_freq=225,step=0.25)
    # _raw_move_EOM_resonance(exp, start_freq=324,end_freq=224,step=-0.5)
    # _raw_move_EOM_resonance(exp, start_freq=155,end_freq=239,step=0.5)
    # _raw_move_EOM_resonance(exp, start_freq=239,end_freq=195,step=-0.5)
    # _raw_move_EOM_resonance(exp, start_freq=201,end_freq=227,step=0.5)
    # _raw_move_EOM_resonance(exp, start_freq=227,end_freq=155,step=-0.5)
    # _raw_move_EOM_resonance(exp, start_freq=693,end_freq=643,step=-0.25)
    # _raw_move_EOM_resonance(exp, start_freq=643,end_freq=683,step=0.25)
    # _raw_move_EOM_resonance(exp, start_freq=201,end_freq=251,step=0.25)
    # _raw_move_EOM_resonance(exp, start_freq=251,end_freq=201,step=-0.25)
    # _raw_move_EOM_resonance(exp, start_freq=208,end_freq=215,step=0.25)
    # _raw_move_EOM_resonance(exp, start_freq=215,end_freq=201,step=-0.25)
    # _raw_move_EOM_resonance(exp, start_freq=201,end_freq=190.5,step=-0.25)
    # _raw_move_EOM_resonance(exp, start_freq=190.5,end_freq=185,step=-0.25)
    # _raw_move_EOM_resonance(exp, start_freq=185,end_freq=141.5,step=-0.25)

    # _raw_move_EOM_resonance(exp, start_freq=625,end_freq=645,step=0.25)
    # _raw_move_EOM_resonance(exp, start_freq=625,end_freq=615,step=-0.25)


    # _raw_move_EOM_resonance(exp, start_freq=200,end_freq=160,step=-0.25, channel=1)
    # _raw_move_EOM_resonance(exp, start_freq=160,end_freq=200,step=0.25, channel=1)

    # _raw_move_EOM_resonance(exp, start_freq=185,end_freq=195,step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=175,end_freq=155,step=-0.25, channel=0)

    # _raw_move_EOM_resonance(exp, start_freq=160,end_freq=180,step=0.25, channel=1)
    # _raw_move_EOM_resonance(exp, start_freq=155,end_freq=145,step=-0.25, channel=0)

    # _raw_move_EOM_resonance(exp, start_freq=180,end_freq=170,step=-0.25, channel=1)
    # _raw_move_EOM_resonance(exp, start_freq=145,end_freq=150,step=0.25, channel=0)

    # _raw_move_EOM_resonance(exp, start_freq=165,end_freq=150,step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=140,end_freq=170,step=-0.25, channel=1)


    # _raw_move_EOM_resonance(exp, start_freq=150,end_freq=147.5,step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=147.5,end_freq=155,step=0.25, channel=0)

    # _raw_move_EOM_resonance(exp, start_freq=155,end_freq=155+43.5,step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=155+43.5,end_freq=155+43.5+5,step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=155+43.5+5,end_freq=155+43.5+5-10,step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=155+43.5+5-10,end_freq=155,step=0.25, channel=0)

    # _raw_move_EOM_resonance(exp, start_freq=155,end_freq=203.5,step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=203.5,end_freq=155,step=0.25, channel=0)

    # _raw_move_EOM_resonance(exp, start_freq=159,end_freq=162.5,step=0.25, channel=1)

    # _raw_move_EOM_resonance(exp, start_freq=155,end_freq=203.5,step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=203.5,end_freq=155,step=0.25, channel=0)


    # _raw_move_EOM_resonance(exp, start_freq=155,end_freq=165,step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=162.5,end_freq=142.5,step=0.25, channel=1)

    # _raw_move_EOM_resonance(exp, start_freq=165,end_freq=145,step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=162.5,end_freq=182.5,step=0.25, channel=1)


    # _raw_move_EOM_resonance(exp, start_freq=145,end_freq=203.5,step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=203.5,end_freq=145,step=0.25, channel=0)

    # _raw_move_EOM_resonance(exp, start_freq=155,end_freq=155+4.9/2,step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=162.5+4.9,end_freq=162.5-4.9,step=0.25, channel=1)

    # _raw_move_EOM_resonance(exp, start_freq=155,end_freq=155+4.9/2,step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=162.5,end_freq=162.5-4.9,step=0.25, channel=1)

    # _raw_move_EOM_resonance(exp, start_freq=157.45,end_freq=203.5,step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=203.5,end_freq=157.45,step=0.25, channel=0)


    # _raw_move_EOM_resonance(exp, start_freq=157.45-10,end_freq=157.45-5,step=0.25, channel=0)

    # _raw_move_EOM_resonance(exp, start_freq=152.45,end_freq=203.5,step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=203.5,end_freq=152.45,step=0.25, channel=0)

    # _raw_move_EOM_resonance(exp, start_freq=152.45,end_freq=147.5,step=0.25, channel=0)

    # _raw_move_EOM_resonance(exp, start_freq=571+8.05,end_freq=571+8.05+9.45/2, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=114,end_freq=120, step=0.25, channel=1)

    # _raw_move_EOM_resonance(exp, start_freq=575+4-6+6.27,end_freq=575+4-6+6.27+10, step=0.25, channel=0)

    #   _raw_move_EOM_resonance(exp, start_freq=583,end_freq=575+4-8, step=0.25, channel=0)
    #   _raw_move_EOM_resonance(exp, start_freq=571,end_freq=571+8.34, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=530+0.93 ,end_freq=530+0.93-7.375/(2*0.15)/2, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=530+0.93-7.375/(2*0.15)/2 ,end_freq=530+0.93-36.094/2, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=530+0.93-10/2-5 ,end_freq=530+0.93-16/2, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=530+0.93-16/2 ,end_freq=530+0.93-36.094/2, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=530+0.93-16/2 ,end_freq=530-24/2, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=530-24/2+12.89 ,end_freq=530+0.89-36.094/2, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=530+0.89-36.094/2 ,end_freq=530-20/2, step=0.25, channel=0)


    # _raw_move_EOM_resonance(exp, start_freq=530+10/2,end_freq=530.+0.92, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=530.36,end_freq=530.-10/2, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=530.36-36.094/2 ,end_freq=530.36, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=530.36 ,end_freq=530.36-36.094/2, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=530.92 ,end_freq=530.92-36.094/2, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=530.92-36.094/2 ,end_freq=530.92, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=530.92 ,end_freq=530-14/2, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=523+28 ,end_freq=530-14/2, step=0.25, channel=0)
    
    # _raw_move_EOM_resonance(exp, start_freq=530.92 ,end_freq=530-14/2, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=530+14/2 ,end_freq=530.83, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=530.83 ,end_freq=530.83-36.094/2, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=530.83-36.094/2,end_freq=530.83, step=0.25, channel=0)

    # _raw_move_EOM_resonance(exp, start_freq=523+14/2 ,end_freq=530.49, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=530.49 ,end_freq=530.49-36.094/2, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=530.49-36.094/2,end_freq=530.49, step=0.25, channel=0)

    # _raw_move_EOM_resonance(exp, start_freq=578.72,end_freq=530-12/2, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=530-12/2,end_freq=530-12/2+8.36, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=530-12/2+8.36,end_freq=530-12/2, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=530.47-36/2,end_freq=530.47, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=530.47,end_freq=530.47-36/2, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=535,end_freq=525, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=530,end_freq=525+2.535, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=527.5,end_freq=525, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=530,end_freq=527.346, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=527.5,end_freq=525, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=525+5,end_freq=525, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=525+4,end_freq=525+1.934, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=526.934,end_freq=575, step=0.25, channel=0)



    # _raw_move_EOM_resonance(exp, start_freq=530.47,end_freq=542-20/2, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=542-20/2,end_freq=542+20/2, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=542+20/2,end_freq=542+20/2+3-14/2, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=548-14/2-9,end_freq=555.14, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=555,end_freq=555-10, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=565,end_freq=554.97, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=554.97,end_freq=554.97-30/2, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=565,end_freq=554.9, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=540,end_freq=555-10/2, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=555-10/2,end_freq=555.149, step=0.25, channel=0)


    # _raw_move_EOM_resonance(exp, start_freq=554.97,end_freq=579-14/2, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=572,end_freq=579.09, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=579.09,end_freq=579.09-36/2, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=555.149,end_freq=579-10/2, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=579+10/2,end_freq=579-5+4.9, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=579,end_freq=579-10/2, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=579+10/2,end_freq=579-5+4.33, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=579-5+3.5,end_freq=579-10/2, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=579,end_freq=579-3/2, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=579+3/2,end_freq=579-3/2-4/2, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=577,end_freq=577-3/2, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=575+4,end_freq=575+1.933, step=0.25, channel=0)

    # _raw_move_EOM_resonance(exp, start_freq=198.845,end_freq=198.845+20, step=0.5, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=198.845+20,end_freq=198.845, step=0.5, channel=0)


    # _raw_move_EOM_resonance(exp, start_freq=530.83,end_freq=583, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=583,end_freq=583-14/2, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=579,end_freq=578-14/2, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=585,end_freq=579.12, step=0.25, channel=0)

    # _raw_move_EOM_resonance(exp, start_freq=579.12,end_freq=579.12-10/2, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=579.01,end_freq=579.01-36.094/2, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=579.01-36.094/2,end_freq=579.01, step=0.25, channel=0)


    # _raw_move_EOM_resonance(exp, start_freq=579-10/2,end_freq=579.-10/2+4.39, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=578.39,end_freq=578.39 + 16/2, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=578.39 + 16/2,end_freq=578.39, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=578.39+16/2,end_freq=578.39 + 20/2, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=578.39+20/2,end_freq=578.39 + 18/2, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=578.39-16/2,end_freq=578.72, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=578,end_freq=578-16/2, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=578+10/2,end_freq=578.55, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=583,end_freq=578-10/2+5.16, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=577.822,end_freq=577.822-5-3, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=577.822-5-3,end_freq=577.822, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=578-6+12,end_freq=578.292, step=0.25, channel=0)


    # _raw_move_EOM_resonance(exp, start_freq=578.72-36/2,end_freq=578.72, step=0.25, channel=0)


    # _raw_move_EOM_resonance(exp, start_freq=530.+16/2 ,end_freq=530.-16/2+7.718, step=0.25, channel=0)


    # _raw_move_EOM_resonance(exp, start_freq= 152.45,end_freq=152.45+20, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq= 152.45+20,end_freq=152.45, step=0.25, channel=0)

    # _raw_move_EOM_resonance(exp, start_freq=577-20,end_freq=577, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=577+5,end_freq=582-10, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=584.514,end_freq=575, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=575+4,end_freq=575+1.652, step=0.25, channel=0)

    # _raw_move_EOM_resonance(exp, start_freq=576.652,end_freq=584.514, step=0.25, channel=0)


    # _raw_move_EOM_resonance(exp, start_freq=582+10,end_freq=584.68, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=584.68,end_freq=580, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=586.5-4.5,end_freq=584.672, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=588,end_freq=584.661, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=584.661,end_freq=582, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=582+5,end_freq=584.626, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=584.626,end_freq=582, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=582+5,end_freq=582+2.514, step=0.25, channel=0)

    # _raw_move_EOM_resonance(exp, start_freq=584.5-6+12,end_freq=584.453, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=584.454+4,end_freq=584.236, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=584.196+5,end_freq=584.788, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=584+4,end_freq=584.104, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=585-400,end_freq=430, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=430,end_freq=430-10, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=427.8,end_freq=427.8-4, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=460,end_freq=430-30, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=423.75+8,end_freq=427.846, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=584-5+10,end_freq=584.203, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=584.203,end_freq=550, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=565.345+5,end_freq=565.438, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=565.5+4,end_freq=565.096, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=549 ,end_freq=547.931, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=529.357+2 ,end_freq=528.990, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=529.185 ,end_freq=529-10, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=529.185 ,end_freq=529-10, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=578.+4 ,end_freq=578.065, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=547.931,end_freq=548-5, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=548+8,end_freq=550.760, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=551+4,end_freq=551.977, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=547.828+4,end_freq=547.805, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=553.373,end_freq=553.5-4, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=584+5,end_freq=584.049, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=584+5,end_freq=584.765, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=548.3+8,end_freq=551.150, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=551.+3,end_freq=552.156, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=547.3,end_freq=529-3, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=529+3,end_freq=528.993, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=582.8-3+6,end_freq=583.73, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=583.8-1,end_freq=583.8-5, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=577.984+5 ,end_freq=577.998, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=577.816-1 ,end_freq=583-2, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=583.8640 ,end_freq=578-5, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=578+5 ,end_freq=577.992, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=583.728-1 ,end_freq=565-5, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=565. ,end_freq=548-3, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=548.+5 ,end_freq=554.053, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=554 ,end_freq=530-4, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=530 ,end_freq=578.3-5, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=578.+4 ,end_freq=578.027, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=578. ,end_freq=565.3-5, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=565.3+4 ,end_freq=565.334, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=565.3 ,end_freq=428-4, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=428+4 ,end_freq=427.773, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=427.7+4,end_freq=427.751, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=427.7,end_freq=583.7-3, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=585+10,end_freq=587-1.5, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=574,end_freq=580.135, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=577.6,end_freq=565-5, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=565+4,end_freq=564.983, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=547.325,end_freq=554-4, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=553,end_freq=549.496, step=0.25, channel=0)
    # _raw_move_EOM_resonance(exp, start_freq=549.496,end_freq=529-5, step=0.25, channel=0)
    _raw_move_EOM_resonance(exp, start_freq=577.65,end_freq=584, step=0.25, channel=0)

    #   _raw_move_EOM_resonance(exp, start_freq=130,end_freq=120, step=0.25, channel=1)
    #   _raw_move_EOM_resonance(exp, start_freq=120,end_freq=130, step=0.25, channel=1)

