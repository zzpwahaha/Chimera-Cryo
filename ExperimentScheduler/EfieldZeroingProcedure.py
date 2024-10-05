def EfieldCalibrationProcedure():
    YEAR, MONTH, DAY = today()
    exp = ExperimentProcedure()
    config_name = "EfieldCalibration.Config"
    config_path = exp.CONFIGURATION_DIR + config_name

    window = [0, 0, 200, 30]
    thresholds = 60
    binnings = np.linspace(0, 240, 241)
    analysis_locs = da.DataAnalysis(year='2024', month='May', day='15', data_name='data_1', 
                                 window=window, thresholds=104, binnings=binnings)

    # for bias E x range = [-0.5, 1] 16, E y range = [-1.2, 0.0] 16, E z range = [-1., 1.0] 16
    config_file = ConfigurationFile(config_path)
    config_file.config_param.update_scan_dimension(0, range_index=0, variations=7)
    config_file.config_param.update_scan_dimension(1, range_index=0, variations=29)

    config_file.config_param.update_variable("bias_e_x", scan_type="Variable", new_initial_values=[-0.5], new_final_values=[1.5])
    config_file.config_param.update_variable("bias_e_y", scan_type="Constant", new_initial_values=[-1.2], new_final_values=[0.0])
    config_file.config_param.update_variable("bias_e_z", scan_type="Constant", new_initial_values=[-1.0], new_final_values=[1.0])
    config_file.config_param.update_variable("resonance_scan", scan_type="Variable", new_initial_values=[73], new_final_values=[87])

    exp.hardware_controller.restart_zynq_control()

    # E_x
    YEAR, MONTH, DAY = today()
    config_file.config_param.update_variable("resonance_scan", scan_dimension=1, scan_type="Variable")
    config_file.config_param.update_variable("bias_e_x", scan_dimension=0, scan_type="Variable")
    config_file.config_param.update_variable("bias_e_y", scan_dimension=0, scan_type="Constant")
    config_file.config_param.update_variable("bias_e_z", scan_dimension=0, scan_type="Constant")
    config_file.save()

    exp_name = "EFIELD-X"
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.run_experiment(exp_name)
    sleep(1)
    while exp.is_experiment_running():
        sleep(10)
        print("Waiting for experiment to finish")

    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    analysis_result = data_analysis.analyze_data_2D()
    optimal_field = analysis_result[1]
    print(f"Optimal field for {exp_name} is {optimal_field:.3S} ")

    exp.hardware_controller.restart_zynq_control()

    # E_y
    YEAR, MONTH, DAY = today()
    config_file.config_param.update_variable("resonance_scan", scan_dimension=1, scan_type="Variable")
    config_file.config_param.update_variable("bias_e_x", scan_dimension=0, scan_type="Constant")
    config_file.config_param.update_variable("bias_e_y", scan_dimension=0, scan_type="Variable")
    config_file.config_param.update_variable("bias_e_z", scan_dimension=0, scan_type="Constant")
    config_file.config_param.update_variable("bias_e_x", constant_value = round(optimal_field.n, 3))
    config_file.save()

    exp_name = "EFIELD-Y"
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.run_experiment(exp_name)
    sleep(1)
    while exp.is_experiment_running():
        sleep(10)
        print("Waiting for experiment to finish")

    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    analysis_result = data_analysis.analyze_data_2D()
    optimal_field = analysis_result[1]
    print(f"Optimal field for {exp_name} is {optimal_field:.3S} ")

    exp.hardware_controller.restart_zynq_control()

    # E_z
    YEAR, MONTH, DAY = today()
    config_file.config_param.update_variable("resonance_scan", scan_dimension=1, scan_type="Variable")
    config_file.config_param.update_variable("bias_e_x", scan_dimension=0, scan_type="Constant")
    config_file.config_param.update_variable("bias_e_y", scan_dimension=0, scan_type="Constant")
    config_file.config_param.update_variable("bias_e_z", scan_dimension=0, scan_type="Variable")
    config_file.config_param.update_variable("bias_e_y", constant_value = round(optimal_field.n, 3))
    config_file.save()

    exp_name = "EFIELD-Z"
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.run_experiment(exp_name)
    sleep(1)
    while exp.is_experiment_running():
        sleep(10)
        print("Waiting for experiment to finish")

    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    analysis_result = data_analysis.analyze_data_2D()
    optimal_field = analysis_result[1]
    print(f"Optimal field for {exp_name} is {optimal_field:.3S} ")

