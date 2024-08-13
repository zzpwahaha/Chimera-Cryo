import sys
# Constants for directory paths
LIBRARY_DIR = "C:\\Chimera\\B240_data_analysis\\Library\\"
CHIMERA_GENTOOL_DIR = LIBRARY_DIR + "ChimeraGenTools\\"
FITTER_DIR = CHIMERA_GENTOOL_DIR + "fitters"

sys.path.append(CHIMERA_GENTOOL_DIR)
sys.path.append(FITTER_DIR)


import numpy as np
import ExpFile as exp
import AnalysisHelpers as ah
import MatplotlibPlotters as mp
from fitters.Gaussian import gaussian
from remoteDataPaths import get_data_files

class DataAnalysis:
    def __init__(self, year, month, day, data_name, 
                 window, thresholds, binnings, maximaLocs=None,
                 annotate_title="", annotate_note="",
                 save_cache=False, read_cache=False, cache_path=None):
        self.year = year
        self.month = month
        self.day = day
        self.data_name = data_name
        self.annotate_title = annotate_title
        self.annotate_note = annotate_note
        self.save_cache = save_cache
        self.read_cache = read_cache
        self.cache_path = cache_path
        self.data_path = None
        self.remote_data_folder = None
        self.cached_data_folder = None
        self.exp_file = None
        self.andor_datas = None

        # Instance variables for analysis parameters, with defaults
        self.window = window
        self.thresholds = thresholds
        self.binnings = binnings
        self.maximaLocs = maximaLocs
        
        self._initialize_data()

    def _initialize_data(self):
        datas, remote_data_folder, cached_data_folder = get_data_files(
            year=self.year, month=self.month, day=self.day, data_runs=self.data_name, 
            save_cache=self.save_cache, read_cache=self.read_cache, cache_path=self.cache_path
        )
        self.data_path = datas
        self.remote_data_folder = remote_data_folder
        self.cached_data_folder = cached_data_folder

        if not exp.checkAnnotation(self.data_name, data_address=self.data_path):
            with exp.ExpFile(data_address=self.data_path) as file:
                file.open_hdf5(self.data_name, openFlag='a', useBase=True)
                exp._annotate(file, self.annotate_title, self.annotate_note)

        CURRENT_VERSION = 1.01
        self.exp_file = exp.ExpFile(file_id=self.data_name, data_address=self.data_path, expFile_version=CURRENT_VERSION)
        print(exp.getAnnotation(fid=self.data_name, data_address=self.data_path))
        self.andor_datas = self.exp_file.get_pics(image_num_per_rep=self.exp_file.pics_per_rep)

        if self.maximaLocs is None:
            self.maximaLocs = self.findAtomLocs()

    def findAtomLocs(self):
        maximaLocs_MP = ah.findAtomLocs(
            pic=ah.softwareBinning(None, self.andor_datas[0].mean(axis=(0, 1))),
            n_cluster_row=1,
            neighborhood_size=11,
            threshold=0,
            window=self.window,
            sort='MatchArray',
            debug_plot=False,
            advanced_option=dict({"active": True, "image_threshold": 99, "score_threshold": 10}),
            multi_points_option=dict({"active": True, "search_square": 3, "num_points": 3})
        )
        return maximaLocs_MP

    def analyze_data(self, debug=False):
        result = ah.getAtomSurvivalData(
            self.andor_datas,
            atomLocation=self.maximaLocs,
            window=self.window,
            bins=self.binnings,
            thresholds=self.thresholds
        )

        x, y, yerr = self.exp_file.individual_keys[0][:], result['survival_mean'][:], result['survival_err'][:]
        x_fit = x[yerr!=0]; y_fit = y[yerr!=0]; yerr_fit = yerr[yerr!=0]
        function = gaussian
        p0 = gaussian.guess(x, y, peak=False)
        p, c = ah.fit(function.f, x_fit, y_fit, sigma=yerr_fit, p0=p0)
        punc = ah.getConfidentialInterval(p, c, n_sample=x.size)
        print(ah.printFittingResult(func=function, popt_unc=punc)[1])

        if debug:
            guess = None
            fig, ax = mp._plotStandard1D(
                x, y, yerr, exp_file=self.exp_file, fitb=True, fit_function=function, guess=guess, 
                ignore_zero_unc=True, use_unc=True, plot_guess=False
            )
            ax.set_xlabel(self.exp_file.key_name[0])
            ax.set_ylabel('survival')
            mp.plt.show()

        return punc

# Usage example
if __name__ == "__main__":
    window = [0, 0, 200, 30]
    thresholds = 60
    binnings = np.linspace(0, 240, 241)
    analysis_locs = DataAnalysis(year='2024', month='May', day='15', data_name='data_1', 
                                 window=window, thresholds=104, binnings=binnings)
    analysis = DataAnalysis(year='2024', month='August', day='8', data_name='data_1', 
                            maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings)
    analysis.analyze_data(debug=True)
