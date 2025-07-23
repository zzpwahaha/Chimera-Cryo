This is a newer boost build by ZZP.
Added asio, bind and thread for GIGAMOOG

https://stackoverflow.com/questions/440585/building-boost-bcp
https://stackoverflow.com/questions/13407664/how-do-i-use-boost-bcp
https://stackoverflow.com/questions/20915123/error-running-boost-bcp-tool-the-boost-path-appears-to-have-been-incorrectly-s/20922288

C:\Software\Boost\boost_1_76_0> .\dist\bin\bcp.exe bind ..\myBoost\
C:\Software\Boost\boost_1_76_0> .\dist\bin\bcp.exe thread ..\myBoost\
C:\Software\Boost\boost_1_76_0> .\dist\bin\bcp.exe asio ..\myBoost\

20241121, add format 
C:\Software\Boost\boost_1_76_0> .\dist\bin\bcp.exe format ..\myBoost\


20250704, add math for PondermotiveIntegratorBoost
C:\Software\Boost\boost_1_76_0> .\dist\bin\bcp.exe math ..\PondermotiveIntegratorBoost\

To build the new boost:
https://stackoverflow.com/questions/28565416/build-dependent-boost-libs-after-doing-bcp
C:\Software\Boost\boost_1_76_0> .\dist\bin\bcp.exe build ../PondermotiveIntegratorBoost
C:\Software\Boost\boost_1_76_0> cd ..\PondermotiveIntegratorBoost\
C:\Software\Boost\PondermotiveIntegratorBoost> .\bootstrap.bat
C:\Software\Boost\PondermotiveIntegratorBoost> ./b2