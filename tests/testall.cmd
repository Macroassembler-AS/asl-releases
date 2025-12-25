@echo off
if "%1"=="" goto main

cd %1
type %1.doc | ..\..\addcr
set ASCMD=@asflags
..\..\asl -i ..\..\include -L +t 31 %1.asm
set ASCMD=
set P2BINCMD=@p2binflags
..\..\p2bin -k -l 0 -r $-$ %1
set P2BINCMD=
..\..\bincmp %1.bin %1.ori
if errorlevel 1 goto errcond
echo Test %1 succeeded!
set SUMPASS=%SUMPASS%+
echo %1 : OK >> ..\..\testlog
:goon
echo +---------------------------------------------------------------+
type %1.lst | find "assembly" >> ..\..\testlog
type %1.lst | find "Assemblierzeit" >> ..\..\testlog
if exist %1.lst del %1.lst >nul
if exist %1.inc del %1.inc >nul
if exist %1.bin del %1.bin >nul
cd ..
goto end

:errcond
echo Test %1 failed!
set SUMFAIL=%SUMFAIL%-
echo %1 : failed >> ..\..\testlog
goto goon

:main
if exist ..\addcr.exe goto nocomp
echo Compiling 'addcr'...
gcc -o ..\addcr.exe ..\addcr.c
:nocomp
if exist ..\bincmp.exe goto nocomp2
echo Compiling 'bincmp'...
gcc -o ..\bincmp.exe ..\bincmp.c
:nocomp2
echo executing self tests...
echo "=================================================================" >..\testlog
echo Summaric results: >> ..\testlog
set SUMPASS=
set SUMFAIL=
for /D %%T in (t_*) do call testall %T%
echo successes: %SUMPASS% >> ..\testlog
echo failures: %SUMFAIL% >> ..\testlog
type ..\testlog

:end
