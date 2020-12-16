# Chimera-Control-Trim
## Works under conditions of MSVC:
* Windows SDK version: 10.0.18362.0
* C++ Lang Stand: C++17
* Use of MFC: Standard Windows Libraries. //Now are free of MFC stuffs
* Optimization scheme: Link Time Code Generation
* Debug environment: `<PATH=$(QTDIR)\bin;%(Path)% >`
  This is intended to override the Anaconda's Qt PATH, since the prepended real Qt PATH is ahead of Anaconda's and thus it will be used.

* Character Set: Use Multi-Byte Character Set. see https://stackoverflow.com/questions/1319461/how-do-i-turn-off-unicode-in-a-vc-project or https://stackoverflow.com/questions/1319461/how-do-i-turn-off-unicode-in-a-vc-project
* Qt build config: Release. Now everything should work with Release mode.
* Should use Release-64-Master.props for the configuration
* Includes: an easy macro from MSVC Qt Extension is `<$(Qt_INCLUDEPATH_)>`
* Library dependencies: an easy macro from MSVC Qt Extension is `<$(Qt_LIBPATH_)>`
* Library input: an easy macro from MSVC Qt Extension is `<$(Qt_LIBS_)>`

* C/C++
  * C++ optimization: Maximum Optimization (Favor Speed) (/O2)
  * Run time checks: default
  * Precompiled headers: Use
  * Preprocessor: can be None
* Linker:
  * debugging: Generate Debug Information (/DEBUG) # for esay debug
  * optimization: Use Fast Link Time Code Generation (/LTCG:incremental) # to be consistent with previously chosen LTCG
