from UtilityFunctions import *

if __name__=='__main__':

    config_name_paths = [
        'C:/Chimera/Chimera-Cryo/Configurations/CryoTweezerLoading/tweezerloading.Config',
        'C:/Chimera/Chimera-Cryo/Configurations/ExperimentAutomation/AOD_grid_alignment_on_SLM.Config',
        'C:/Chimera/Chimera-Cryo/Configurations/ExperimentAutomation/1013alignment_trapRamp.Config',
        'C:/Chimera/Chimera-Cryo/Configurations/ExperimentAutomation/420alignment_ionization.Config',
        'C:/Chimera/Chimera-Cryo/Configurations/ExperimentAutomation/420alignment_with_d1.Config',
        'C:/Chimera/Chimera-Cryo/Configurations/ExperimentAutomation/1013alignment_with_d1.Config',
    ]

    for cnp in config_name_paths:    
        config_file = ConfigurationFile(cnp)
        # update_imaging_cooling_bias_field(config_file=config_file,fields=CRYO_BIAS_FIELD)
        update_imaging_cooling_bias_field(config_file=config_file,fields=RT_BIAS_FIELD)
        config_file.save()