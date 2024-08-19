import sys
from datetime import datetime
from time import sleep
import numpy as np

from ZynqController.ZynqStarter import ZynqStarter
from ZynqController.TCPClient import TCPClient
from Logging.PrintLogger import Logger
import DataProcess.DataAnalyzer as da
from ConfigurationManager.ConfigurationFile import ConfigurationFile
from ConfigurationManager.ConfigurationScanParameter.ScanRange import ScanRange


class ExperimentProcedure:
    CONFIGURATION_DIR = "C:/Chimera/Chimera-Cryo/Configurations/ExperimentAutomation/"
    CHIMERA_HOST = 'localhost'
    CHIMERA_PORT = 8888
    ZYNQ_HOST = '10.10.0.2'
    ZYNQ_PORT = 8080
    SYNACCESS_HOST_ZYNQ = '10.10.0.100'
    SYNACCESS_HOST_COIL = '10.10.0.101'

    def __init__(self):
        # Set up logging
        current_datetime = datetime.now()
        formatted_datetime_dash = current_datetime.strftime("%Y-%m-%d_%H-%M-%S")
        self.logger = Logger(f'./log/{formatted_datetime_dash}.log')
        sys.stdout = self.logger
        
        # Initialize TCP client and hardware controller
        self.chimera_client = TCPClient(self.CHIMERA_HOST, self.CHIMERA_PORT)
        self.hardware_controller = ZynqStarter(self.ZYNQ_HOST, self.ZYNQ_PORT, 
                                               self.SYNACCESS_HOST_ZYNQ, self.SYNACCESS_HOST_COIL)

        # Establish connection with Chimera
        self.chimera_client.connect()

    def chimera_command(self, command: str) -> bool:
        result = self.chimera_client.query(command)
        print(result)
        return not self._judge_message_failure(result)
        
    @staticmethod
    def _judge_message_failure(result_message: str) -> bool:
        failure_keywords = ["Error", "ERROR", "error"]
        if any(keyword in result_message for keyword in failure_keywords):
            print(f"Command failed: {result_message}")
            return True
        else:
            print(f"Command succeeded: {result_message}")
            return False

    def run_experiment(self, name: str):
        if self.chimera_command("Save-All"):
            self.chimera_command(f"Start-Experiment ${name}")

    def open_configuration(self, config_path_name: str):
        self.chimera_command(f"Open-Configuration ${config_path_name}")

    def open_master_script(self, script_path_name: str):
        self.chimera_command(f"Open-Master-Script ${script_path_name}")

    def save_all(self):
        self.chimera_command(f"Save-All")

    def is_experiment_running(self) -> bool:
        result = self.chimera_client.query("Is-Experiment-Running?")
        self._judge_message_failure(result)
        if 'TRUE' in result:
            return True
        elif 'FALSE' in result:
            return False
        else:
            print(f"Unexpected response received: {result}")
        
def today():
    today = datetime.now()
    # Extract year, month, and day
    year = str(today.year)           # Convert year to string
    month = today.strftime('%B')    # Full month name (already a string)
    day = str(today.day)            # Convert day to string
    return year, month, day

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


# Example usage:
if __name__ == "__main__":
    EfieldCalibrationProcedure()
