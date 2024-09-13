import sys
from datetime import datetime
from time import sleep
import time
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
        if self.save_all():
            self.chimera_command(f"Start-Experiment ${name}")

    def abort_experiment(self, save=True, file_name="placeholder"):
        action = 'save' if save else 'delete'
        if not save and file_name=="placeholder":
            print('Failed to abort and delete data: no valid file name provided.')
            return
        self.chimera_command(f"Abort-Experiment ${action} ${file_name}")

    def open_configuration(self, config_path_name: str):
        self.chimera_command(f"Open-Configuration ${config_path_name}")

    def open_master_script(self, script_path_name: str):
        self.chimera_command(f"Open-Master-Script ${script_path_name}")

    def save_all(self):
        return self.chimera_command(f"Save-All")

    def is_experiment_running(self) -> bool:
        result = self.chimera_client.query("Is-Experiment-Running?")
        self._judge_message_failure(result)
        if 'TRUE' in result:
            return True
        elif 'FALSE' in result:
            return False
        else:
            print(f"Unexpected response received: {result}")
    
    def run_calibration(self, name: str):
        if self.save_all():
            self.chimera_command(f"Start-Calibration ${name}")

    def is_calibration_running(self) -> bool:
        result = self.chimera_client.query("Is-Calibration-Running?")
        self._judge_message_failure(result)
        if 'TRUE' in result:
            return True
        elif 'FALSE' in result:
            return False
        else:
            print(f"Unexpected response received: {result}")

    def setDAC(self):
        return self.chimera_command(f"Set-DAC")
    
    def setDDS(self):
        return self.chimera_command(f"Set-DDS")

    def setOL(self):
        return self.chimera_command(f"Set-OL")

    def setZynqOutput(self):
        self.setDAC()
        time.sleep(1)
        self.setDDS()
        time.sleep(1)
        self.setOL()
        time.sleep(1)

def experiment_monitoring(exp : ExperimentProcedure, timeout_control = {'use':False, 'timeout':600}):
    # Monitor experiment status
    exp_start_time = time.time()
    timeout = timeout_control.get('timeout', 600)
    time.sleep(1)
    while exp.is_experiment_running():
        elapsed_time = time.time() - exp_start_time
        if timeout_control.get('use', False) and elapsed_time > timeout:
            exp.abort_experiment(save=True)
            print(f"Experiment aborted after {timeout} seconds due to timeout.")
            time.sleep(10)
            break
        print("Waiting for experiment to finish...")
        time.sleep(10) 
    return

def analog_in_calibration_monitoring(exp : ExperimentProcedure, timeout_control = {'use':False, 'timeout':100}):
    # Monitor experiment status
    exp_start_time = time.time()
    timeout = timeout_control.get('timeout', 100)
    time.sleep(1)
    while exp.is_calibration_running():
        elapsed_time = time.time() - exp_start_time
        if timeout_control.get('use', False) and elapsed_time > timeout:
            # exp.abort_experiment(save=True)
            print(f"Calibration aborted after {timeout} seconds due to timeout. THIS SHOULDN'T HAPPEN!!!!")
            time.sleep(10)
            break
        print("Waiting for calibration to finish...")
        time.sleep(1) 
    time.sleep(5) 
    return

def analog_in_calibration(exp : ExperimentProcedure, name : str):
    # Setup calibration details
    exp.run_calibration(name)
    # Monitor calibration status
    analog_in_calibration_monitoring(exp = exp);

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
    # EfieldCalibrationProcedure()
    exp = ExperimentProcedure()
    exp.run_calibration("prb_pwr")

