import numpy as np
from ExperimentProcedure import *
from ExperimentProcedure import experiment_monitoring, analog_in_calibration_monitoring
from RydbergBeamMoveProcedure import move_beam_to_target, RYDBERG_BEAM_420_POSITION, RYDBERG_BEAM_1013_POSITION
import time


YEAR, MONTH, DAY = today()
exp = ExperimentProcedure()

config_name = "Microwave.Config"
config_path = exp.CONFIGURATION_DIR + config_name
config_file = ConfigurationFile(config_path)

config_file.modify_parameter("REPETITIONS", "Reps:", str(7))
for variable in config_file.config_param.variables:
    config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    
config_file.config_param.update_variable("resonance_scan", scan_type="Variable", new_initial_values=[76], new_final_values=[84])
config_file.config_param.update_scan_dimension(0, range_index=0, variations=33)

exp.open_configuration("\\ExperimentAutomation\\" + config_name)

# analysis grid
window = [0, 0, 200, 30]
thresholds = 100
binnings = np.linspace(0, 240, 241)
analysis_locs = da.DataAnalysis(year='2024', month='August', day='26', data_name='data_1', 
                                window=window, thresholds=70, binnings=binnings)


def resonace_scan_coarse(exp_idx, timeout_control = {'use':False, 'timeout':1200}):
    script_name = "Calibration_MW_excitation.mScript"

    # Update configuration
    config_file.modify_parameter("REPETITIONS", "Reps:", str(2))
    mw_section = config_file.get_section("MICROWAVE_SYSTEM")
    mw_section.mw_lists[1].parameters['Freq:'] = "rb87_hfsplitting+resonance_scan"
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_scan_dimension(0, new_ranges=[ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=51)])
    config_file.config_param.update_variable("resonance_scan", scan_type="Variable", 
                                             new_initial_values=[7.15], new_final_values=[7.3])
    # config_file.config_param.update_variable("time_scan", constant_value = 0.11)
    config_file.save()
    
    # Setup experiment details
    YEAR, MONTH, DAY = today()
    exp_name = f"MW-RESONANCE-SCAN-COARSE-{exp_idx}"
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
    print(f"Optimal resoance for {exp_name} is {optimal_field:.5S} ")

    fit_fail = (optimal_field.s > 1) or (analysis_result[0].n > 2) or (analysis_result[0].n < 0)
    if fit_fail:
        raise ValueError(f"Optimal resoance {optimal_field:.5S} has a variance larger than 1 or {analysis_result[0]:.5S} is outside the normal range, this typically means bad data.")

    # Update configuration with the optimal field
    config_file.config_param.update_variable("resonance_scan", constant_value = round(optimal_field.n, 5))
    config_file.save()
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    return  fit_fail

def resonace_scan_fine(exp_idx, timeout_control = {'use':False, 'timeout':1200}):
    script_name = "Calibration_MW_excitation.mScript"

    # Update configuration
    config_file.modify_parameter("REPETITIONS", "Reps:", str(3))
    mw_section = config_file.get_section("MICROWAVE_SYSTEM")
    mw_section.mw_lists[1].parameters['Freq:'] = "rb87_hfsplitting+resonance_scan"
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_scan_dimension(0, new_ranges=[ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=21)])
    coarse_center = config_file.config_param.get_variable("resonance_scan").constant_value
    config_file.config_param.update_variable("resonance_scan", scan_type="Variable", 
                                             new_initial_values=[coarse_center-0.01], new_final_values=[coarse_center+0.01])
    # config_file.config_param.update_variable("time_scan", constant_value = 0.11)
    config_file.save()
    
    # Setup experiment details
    YEAR, MONTH, DAY = today()
    exp_name = f"MW-RESONANCE-SCAN-FINE-{exp_idx}"
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
    print(f"Optimal resoance for {exp_name} is {optimal_field:.5S} ")

    fit_fail = (optimal_field.s > 1) or (analysis_result[0].n > 2) or (analysis_result[0].n < 0)
    if fit_fail:
        raise ValueError(f"Optimal resoance {optimal_field:.5S} has a variance larger than 1 or {analysis_result[0]:.5S} is outside the normal range, this typically means bad data.")

    # Update configuration with the optimal field
    config_file.config_param.update_variable("resonance_scan", constant_value = round(optimal_field.n, 5))
    config_file.save()
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    return  fit_fail

def rabi_scan(exp_idx, timeout_control = {'use':False, 'timeout':600}):
    script_name = "Calibration_MW_excitation.mScript"
    config_file.modify_parameter("REPETITIONS", "Reps:", str(4))
    mw_section = config_file.get_section("MICROWAVE_SYSTEM")
    mw_section.mw_lists[1].parameters['Freq:'] = "rb87_hfsplitting+resonance_scan"
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_scan_dimension(0, new_ranges=[
        ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=41),
        ])
    config_file.config_param.update_variable("time_scan", scan_type="Variable", 
                                             new_initial_values=[0.01,], 
                                             new_final_values=[1.01,])

    config_file.save()
    
    YEAR, MONTH, DAY = today()
    exp_name = f"MW-RABI-SCAN-{exp_idx}"
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.open_master_script("\\ExperimentAutomation\\" + script_name)
    exp.run_experiment(exp_name)

    # Monitor experiment status
    experiment_monitoring(exp=exp, timeout_control=timeout_control)

    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    return

def pi_pi2_time_scan(exp_idx, timeout_control = {'use':False, 'timeout':600}):
    script_name = "Calibration_MW_excitation.mScript"

    # Update configuration
    config_file.modify_parameter("REPETITIONS", "Reps:", str(4))
    mw_section = config_file.get_section("MICROWAVE_SYSTEM")
    mw_section.mw_lists[1].parameters['Freq:'] = "rb87_hfsplitting+resonance_scan"
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_scan_dimension(0, new_ranges=[ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=31)])
    config_file.config_param.update_variable("time_scan", scan_type="Variable", new_initial_values=[0.01], new_final_values=[0.16])
    config_file.save()
    
    # Setup experiment details
    YEAR, MONTH, DAY = today()
    exp_name = f"MW-PI-TIME-SCAN-{exp_idx}"
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.open_master_script("\\ExperimentAutomation\\" + script_name)
    exp.run_experiment(exp_name)
    
    # Monitor experiment status
    experiment_monitoring(exp=exp, timeout_control=timeout_control)

    RABI_GUESS = [0.9,5,3.14,0.5]
    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    fit_result = data_analysis.analyze_data(function = da.trigonometric, p0=RABI_GUESS)
    punc, p = fit_result, da.ah.nominal(fit_result)
    
    # find pi time
    angle = np.pi-p[3]
    angle = np.fmod(angle, 2 * np.pi)
    if angle < 0:
        angle += 2 * np.pi
    angle = da.ah.unc.ufloat(angle, punc[2].std_dev)
    t_pi = angle / (2*np.pi*punc[1])
    if  t_pi<0.01 or t_pi>0.25:
        print(f"Faulty fitted pi time {t_pi:.3S}")
    else:
        print(f"Good fitted pi time {t_pi:.3S}")
        config_file.config_param.update_variable("mw_pi", constant_value = round(t_pi.n, 4))


    # find pi2 time
    angle = da.ah.unp.arccos((0.5-punc[3])/(punc[0]/2))-punc[2]
    angle_n, angle_s = angle.nominal_value, angle.std_dev
    angle_n = np.fmod(angle_n, 2 * np.pi)
    if angle_n < 0:
        angle_n += np.pi
    angle = da.ah.unc.ufloat(angle_n, angle_s)
    t_pi2 = angle / (2*np.pi*punc[1])
    if  t_pi2<0.01 or t_pi2>0.25:
        raise ValueError(f"Faulty fitted pi2 time {t_pi2:.3S}")
    else:
        print(f"Good fitted pi2 time {t_pi2:.3S}")
        config_file.config_param.update_variable("mw_pi2", constant_value = round(t_pi2.n, 4))

    # Update configuration with the optimal field
    config_file.save()
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)

def ramsey_scan_echo(exp_idx, timeout_control = {'use':False, 'timeout':600}):
    script_name = "rydberg_420_MWlightshift_Ramsey.mScript"
    config_file.modify_parameter("REPETITIONS", "Reps:", str(5))
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_scan_dimension(0, new_ranges=[
        ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=11),
        ScanRange(index=1,left_inclusive=True, right_inclusive=True, variations=11),
        ScanRange(index=2,left_inclusive=True, right_inclusive=True, variations=11)
        ])
    config_file.config_param.update_variable("time_scan", scan_type="Variable", 
                                             new_initial_values=[0.001,0.1,0.2], 
                                             new_final_values=[0.021,0.12,0.22])
    config_file.save()
    
    YEAR, MONTH, DAY = today()
    exp_name = f"MW-RAMSEY-SCAN-420-BEAM-ECHO-{exp_idx}"
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.open_master_script("\\ExperimentAutomation\\" + script_name)
    exp.run_experiment(exp_name)

    # Monitor experiment status
    experiment_monitoring(exp=exp, timeout_control=timeout_control)

    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    return

def _calibration():
    exp.setZynqOutput()
    analog_in_calibration(exp=exp, name = "prb_pwr")
    exp.save_all()
    config_file.reopen()
    sleep(1)


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
    try:
        # _calibration()
        # recenter_beams()
        resonace_scan_coarse(exp_idx=exp_idx, timeout_control = {'use':True, 'timeout':1200})
        pass
    except Exception as e:
        print(e)
        exp.hardware_controller.restart_zynq_control()
        # calibration(exp_idx)
        return
    try:
        exp.hardware_controller.restart_zynq_control()
        # _calibration()
        # recenter_beams()
        resonace_scan_fine(exp_idx=exp_idx, timeout_control = {'use':True, 'timeout':1200})
        sleep(3)

        exp.hardware_controller.restart_zynq_control()
        # _calibration()
        # recenter_beams()
        rabi_scan(exp_idx=exp_idx, timeout_control = {'use':True, 'timeout':1200}) #1500
        sleep(3)

        exp.hardware_controller.restart_zynq_control()
        # _calibration()
        # recenter_beams()
        pi_pi2_time_scan(exp_idx=exp_idx, timeout_control = {'use':True, 'timeout':1200}) #1500
        sleep(3)

        exp.hardware_controller.restart_zynq_control()
        _calibration()
        recenter_beams()
        ramsey_scan_echo(exp_idx=exp_idx, timeout_control = {'use':True, 'timeout':1200}) #1500
        sleep(3)

    except Exception as e:
        print(e)
        exp.hardware_controller.restart_zynq_control()
        return


def procedure():
    for idx in np.arange(1,3):
        # if idx<12: continue
        # if idx<12: continue
        print(f"Running experiment sets number {idx}")
        if idx != 0:
            exp.hardware_controller.restart_zynq_control()
        calibration(idx)

if __name__=='__main__':
    procedure()
