import numpy as np
import matplotlib.pyplot as plt
import sys
sys.path.append('C:\Chimera\B240_data_analysis\Library\ChimeraGenTools')


import AnalysisHelpers as ah
import MatplotlibPlotters as mp
from fitters import linear


freqLUT = np.load('./freqLUT_20251208_data2.npy')
print(freqLUT.shape)
gridShape = freqLUT.shape[:2]

print(gridShape)
print(freqLUT[0,:,0])

x_grid_idx,x_freq = np.arange(gridShape[1]), freqLUT[0,:,0]
function = linear
p0 = None
punc,p,_ = ah.fit_data(x_grid_idx,x_freq, fit_function=function, use_unc=False)

x,y = x_grid_idx,x_freq
fig,ax = mp._plotStandard1D(x,y,exp_file=None, fitb=True, fit_function=function,
                                guess=p0, ignore_zero_unc=True, use_unc=False, plot_guess=False, fit_result_on_title=True, data_name_newline=True, ylim=None)
ax.set_title(ax.get_title())
ax.set_xlabel('atom index')
ax.set_ylabel('survival')
print(f'rounded p = [{np.round(p[0],3)}, {p[1]}]')
plt.show()
p = [np.round(p[0],5), p[1]]

x,y = x_grid_idx,x_freq-linear.f(x_grid_idx, *p)
fig,ax = mp._plotStandard1D(x,y,np.zeros_like(y),exp_file=None, fitb=False, fit_function=function,
                                guess=p0, ignore_zero_unc=True, use_unc=False, plot_guess=False, fit_result_on_title=True, data_name_newline=True, ylim=None)
ax.set_title(ax.get_title())
ax.set_xlabel('atom index')
ax.set_ylabel('survival')
fig.tight_layout()
mp.GoldenRatio(fig)
plt.show()

freqLUT_new = np.zeros_like(freqLUT)
freqLUT_new[0,:,0] = np.round(linear.f(x_grid_idx, *p), 5)
# freqLUT_new[0,:,1] = np.round(freqLUT[0,:,1].mean(), 5)
freqLUT_new[0,:,1] = np.repeat(freqLUT[0,0,1], gridShape[0])

print(freqLUT_new-freqLUT)
print(freqLUT_new.shape)
# np.save('./freqLUT_uniform_spacing.npy', freqLUT_new)
# freqLUT = freqLUT.reshape(*pts_Marana_SLM.shape)
# freqLUT.shape