from UtilityFunctions import *

if __name__=='__main__':

    config_name_paths = [
        'C:/Chimera/Chimera-Cryo/Configurations/CryoTweezerLoading/tweezerloading_manyBodyRydbergLifetimeDressingNoStrobo.Config',
        # 'C:/Chimera/Chimera-Cryo/Configurations/CryoTweezerLoading/tweezerloading.Config',
        # 'C:/Chimera/Chimera-Cryo/Configurations/CryoTweezerLoading/tweezerloading_RydbergRabi_wRearrangement.Config',
        # 'C:/Chimera/Chimera-Cryo/Configurations/SiteSelectiveIonization/siteSelectiveIonization.Config',
        # 'C:/Chimera/Chimera-Cryo/Configurations/CryoTweezerLoading/tweezerloading_singleBodyRydbergLifetime.Config',
        # 'C:/Chimera/Chimera-Cryo/Configurations/ExperimentAutomation/AOD_grid_alignment_on_SLM.Config',
        # 'C:/Chimera/Chimera-Cryo/Configurations/ExperimentAutomation/1013alignment_trapRamp.Config',
        # 'C:/Chimera/Chimera-Cryo/Configurations/ExperimentAutomation/420alignment_ionization.Config',
        # 'C:/Chimera/Chimera-Cryo/Configurations/ExperimentAutomation/420alignment_with_d1.Config',
        # 'C:/Chimera/Chimera-Cryo/Configurations/ExperimentAutomation/1013alignment_with_d1.Config',
        # 'C:/Chimera/Chimera-Cryo/Configurations/ExperimentAutomation/420polarization_with_stroboscopic_depumping.Config',
        # 'C:/Chimera/Chimera-Cryo/Configurations/ExperimentAutomation/OpticalPumpingOptimization.Config',
        # 'C:/Chimera/Chimera-Cryo/Configurations/ExperimentAutomation/LGM_cooling_detuning_Optimization.Config',
        # 'C:/Chimera/Chimera-Cryo/Configurations/ExperimentAutomation/LGM_enhancedloading_Optimization.Config',
        # 'C:/Chimera/Chimera-Cryo/Configurations/ExperimentAutomation/EfieldCalibration_rearrangement.Config',
        # 'C:/Chimera/Chimera-Cryo/Configurations/ExperimentAutomation/rearrangement.Config',
        # 'C:/Chimera/Chimera-Cryo/Configurations/ExperimentAutomation/RydbergRabi_MWionization_wRearrangement.Config',
        # 'C:/Chimera/Chimera-Cryo/Configurations/ExperimentAutomation/420ionizationrate_vs_trapdepth.Config',
        # 'C:/Chimera/Chimera-Cryo/Configurations/ExperimentAutomation/VacuumLifetime.Config',
    ]

    for cnp in config_name_paths:    
        config_file = ConfigurationFile(cnp)
        print(cnp)
        # update_imaging_cooling_bias_field(config_file=config_file,fields=CRYO_BIAS_FIELD)
        update_imaging_cooling_bias_field(config_file=config_file,fields=RT_BIAS_FIELD)
        config_file.save()