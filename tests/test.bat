@echo off
REM test script for xbios.exe

cls

setlocal enabledelayedexpansion

if "%~1" == "-?" goto :help
if "%~1" == "/?" goto :help

:setup
    REM ensure we we in the correct directory
    if NOT "%CD%\" == "%~dp0" (
        echo cd: %CD%
        echo Changing directory to %~dp0
        cd "%~dp0"
        echo cd: %CD%
        pause
    )

    set error_flag=0

    set "exe=xbios.exe"

    set "MCPX_ROM_1_0=-mcpx mcpx\mcpx_1.0.bin"
    set "MCPX_ROM_1_1=-mcpx mcpx\mcpx_1.1.bin"

    if not exist "mcpx" mkdir mcpx
    if not exist "bios" mkdir bios

    if not exist "bios\custom" mkdir bios\custom

    if not exist "bios\og_1_0" mkdir bios\og_1_0
    if not exist "bios\og_1_1" mkdir bios\og_1_1
    if not exist "bios\512kb" mkdir bios\512kb

    if not exist "decode.ini" echo. > decode.ini
    if not exist "logs\" mkdir logs

:cleanup_test_files
    echo Cleaning up test files...
    del /q logs\*.log 2>nul || (
        echo Failed to clear logs directory
        exit /b 1
    )    
    del /q *.bin 2>nul || (
        echo Failed to clear root directory
        exit /b 1
    )
    del /q *.img 2>nul || (
        echo Failed to clear root directory
        exit /b 1
    )
    del /q xbios.exe 2>nul || (
        echo Failed to clear root directory
        exit /b 1
    )
    echo Cleaned up.    
    if "%~1" == "-c" exit /b 0

:check_files
    if not exist "mcpx\mcpx_1.0.bin" (
        echo mcpx\mcpx_1.0.bin not found
        exit /b 1
    )
    if not exist "mcpx\mcpx_1.1.bin" (
        echo mcpx\mcpx_1.1.bin not found
        exit /b 1
    )

    if not exist ..\bin\xbios.exe (
        echo xbios.exe not found
        exit /b 1
    )
    echo Copying xbios.exe to test directory...
    copy /y ..\bin\xbios.exe xbios.exe || (
        echo Failed to copy xbios.exe to test directory
        exit /b 1
    )

:run_tests
    set jobs_passed=0
    set jobs_total=0
    set "cur_job="

    if "%~1" == "-1.0" (
        goto :mcpx_1_0_bios_tests
    ) else if "%~1" == "-1.1" (
        goto :mcpx_1_1_bios_tests
    ) else if "%~1" == "-512" (
        goto :mcpx_1_0_512kb_bios_tests
    ) else if "%~1" == "-custom" (
        goto :custom_bios_tests
    ) else if NOT "%~1" == "" (
        echo Error: unknown test option '%~1'
        exit /b 1
    )
    goto :execute_tests

:execute_tests
    REM test help
    call :do_test "-?" 0

    REM test garbage input
    call :do_test "" -1
    call :do_test "-ls noexist.bin" -1
    call :do_test "-ls xbios.exe" -1
    call :do_test "-ls decode.ini" -1
    call :do_test "-nocommand blah" -1

REM run original tests for bios less than 4817
:mcpx_1_0_bios_tests   
    call :run_og_test "bios\og_1_0" "%MCPX_ROM_1_0%"
if "%~1" == "-1.0" goto :exit

REM run original tests for bios greater than or equal to 4817
:mcpx_1_1_bios_tests
    call :run_og_test "bios\og_1_1" "%MCPX_ROM_1_1%"
if "%~1" == "-1.1" goto :exit

REM run original tests for 512kb bios (less than 4817)
:mcpx_1_0_512kb_bios_tests
    call :run_og_test "bios\512kb" "%MCPX_ROM_1_0%" "-binsize 512 -romsize 512"
if "%~1" == "-512" goto :exit

:custom_bios_tests
    REM test custom bios
    for %%f in (bios\custom\*.bin) do (
        set "arg=%%f"
        set arg_name=%%~nf
        REM test custom bios decoding and visor simulation

        call :do_test "-xcode-decode !arg!" 0 "!arg_name!"
        call :do_test "-xcode-sim !arg!" 0 "!arg_name!"
    )
if "%~1" == "-custom" goto :exit

:exit
:error
    if !jobs_total! equ 0 ( 
        echo No tests were found to run.
    )
    if !error_flag! neq 0 ( 
        echo Test !jobs_total! failed. expected: '!expected_error!' got: '!last_error!'
        REM ask user to if they want to see the log
        if exist !end_log! (
            echo log: !end_log!
            set /p "view_log=View log? (y/n): "
            if /i "!view_log!" == "y" ( 
                type !end_log!
            )
        )
        exit /b 1
    ) else ( 
        echo !jobs_passed!/!jobs_total! tests passed.
    )

    endlocal
    exit /b !last_error!

:do_test
    if NOT !error_flag! == 0 exit /b 0

    set "cur_job=%~1"
    set "expected_error=%~2"
    set "job_name=%~3"

    if "!cur_job!" == "" exit /b 0

    REM get cmd from cur_job
    for /f "tokens=1 delims= " %%a in ("!cur_job!") do set "cmd=%%a"
    REM remove '-' from cmd
    set "cmd=!cmd:~1!"

    if "!job_name!" == "" ( set "job_name=last_test" ) else ( set "job_name=!job_name!_!cmd!" )

    if "%expected_error%" == "" set "expected_error=0"
    
    set /a jobs_total+=1

    echo.
    echo Test !jobs_total! '!cur_job!' expecting '!expected_error!'

    set "end_log=logs\!job_name!.log"
    !exe! !cur_job! > !end_log! 2>&1
    set last_error=!errorlevel!

    if !errorlevel! neq !expected_error! (
        set error_flag=!last_error!
        exit /b 0
    )
    echo Test !jobs_total! passed ( !errorlevel! )

    set /a jobs_passed+=1
    
    exit /b 0

:help
    echo Usage: %~nx0 [-h] [-c] [-1.0] [-1.1] [-512]
    echo.
    echo -h: Display this help message
    echo -c: Clean up the test directory
    echo -1.0: Run tests for mcpx 1.0
    echo -1.1: Run tests for mcpx 1.1
    echo -512: Run tests for 512kb bios
    echo -custom: Run tests for custom bios
    exit /b 0

:run_og_test
    set "bios_dir=%~1"
    set "mcpx_rom=%~2"
    set "extra_args=%~3"

    for %%f in (!bios_dir!\*.bin) do (
        set "arg=%%f"
        set arg_name=%%~nf
        REM test all original bios are decryptable using specified mcpx rom

        call :do_test "-decomp-krnl !arg! !mcpx_rom! !extra_args!" 0 "!arg_name!"
        call :do_test "-ls !arg! !mcpx_rom! !extra_args!" 0 "!arg_name!"
        call :do_test "-xcode-decode !arg! !extra_args!" 0 "!arg_name!"

        REM test extracting bios 
        call :do_test "-extr !arg! !mcpx_rom! !extra_args!" 0 "!arg_name!"
        REM build that extracted bios ; replicate to 1mb ; encrypt kernel; encrypting with mcpx 1.0
        call :do_test "-bld -bldr bldr.bin -inittbl inittbl.bin -krnl krnl.bin -krnldata krnl_data.bin %MCPX_ROM_1_0% -enc-krnl !extra_args! -binsize 1024 -out !arg_name!_bld.bin" 0 "!arg_name!"
        REM test built bios; running -ls calls most things in the program.
        call :do_test "-ls !arg_name!_bld.bin %MCPX_ROM_1_0% !extra_args!" 0
        REM test decompressing the built bios' kernel ;  this ensures the kernel is decryptable (we encrypted it earlier)
        call :do_test "-decomp-krnl !arg_name!_bld.bin %MCPX_ROM_1_0% !extra_args!" 0

        REM build that extracted bios ; no replicate ; no encryption
        call :do_test "-bld -bldr bldr.bin -inittbl inittbl.bin -krnl krnl.bin -krnldata krnl_data.bin -out !arg_name!_no_enc_bld.bin !extra_args!" 0
        REM test built bios; running -ls calls most things in the program.
        call :do_test "-ls !arg_name!_no_enc_bld.bin -enc-krnl !extra_args!" 0
        REM test decompressing the built bios' kernel ;  this ensures the kernel is decryptable (we encrypted it earlier)
        call :do_test "-decomp-krnl !arg_name!_no_enc_bld.bin -enc-krnl !extra_args!" 0
    )
    exit /b 0
