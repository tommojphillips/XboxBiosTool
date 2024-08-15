@echo off
REM test script for xbios.exe

cls

setlocal enabledelayedexpansion

REM XBIOS ERROR CODES

REM 0: ERROR_SUCCESS
REM 1: ERROR_GENERAL_FAILURE

REM 4: ERROR_BUFFER_OVERFLOW
REM 5: ERROR_OUT_OF_MEMORY
REM 6: ERROR_INVALID_DATA


if "%~1" == "-?" goto :help
if "%~1" == "/?" goto :help

set "enable_logs=0"

:setup
    REM ensure we we in the correct directory
    if NOT "%CD%\" == "%~dp0" (
        echo Change directory to %~dp0
        exit /b 1
    )

    set error_flag=0

    set "exe=..\bin\xbios.exe"

    set "MCPX_ROM_1_0=-mcpx mcpx\mcpx_1.0.bin"
    set "MCPX_ROM_1_1=-mcpx mcpx\mcpx_1.1.bin"

    if not exist "mcpx" mkdir mcpx
    if not exist "bios" mkdir bios

    if not exist "bios\custom" mkdir bios\custom

    if not exist "bios\og_1_0" mkdir bios\og_1_0
    if not exist "bios\og_1_1" mkdir bios\og_1_1
    if not exist "bios\512kb" mkdir bios\512kb
    if not exist "bios\img" mkdir bios\img

    if not exist "decode.ini" echo. > decode.ini
    if not exist "decode2.ini" echo. > decode2.ini
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

    if not exist "!exe!" (
        echo '!exe!' not found
        exit /b 1
    )    

:run_tests
    set jobs_passed=0
    set jobs_total=0
    set "cur_job="

    set "test_group=%~1"

    if "!test_group!" == "-1.0" (
        echo Running tests for 1.0 bios...
        goto :mcpx_1_0_bios_tests
    ) else if "!test_group!" == "-1.1" (
        echo Running tests for 1.1 bios...
        goto :mcpx_1_1_bios_tests
    ) else if "!test_group!" == "-512" (
        echo Running tests for 512kb bios...
        goto :mcpx_1_0_512kb_bios_tests
    ) else if "!test_group!" == "-custom" (
        echo Running custom bios tests...
        goto :custom_bios_tests
    ) else if "!test_group!" == "-img" (
        echo Running tests for xb img files...
        goto :img_tests
    ) else if NOT "!test_group!" == "" (
        echo Error: unknown test option '!test_group!'
        exit /b 1
    )
    goto :execute_tests

:execute_tests
    REM test help
    call :do_test "-?" 0

    REM test garbage input
    call :do_test "-ls noexist.bin" 1
    call :do_test "-ls !exe!" 1
    call :do_test "-nocommand blah" 1

REM run original tests for bios less than 4817
:mcpx_1_0_bios_tests   
    call :run_og_test "bios\og_1_0" "%MCPX_ROM_1_0%"

    for %%f in (bios\og_1_0\*.bin) do (
        set "arg=%%f"
        set arg_name=%%~nf

        call :do_test "-split !arg!" 0 "!arg_name!"
        REM note this test expects bios to be 1MB
        call :do_test "-combine !arg_name!_bank1.bin !arg_name!_bank2.bin !arg_name!_bank3.bin !arg_name!_bank4.bin" 0 "!arg_name!"
        call :do_test "-split bios.bin" 0 "!arg_name!"
        call :do_test "-decomp-krnl bios_bank1.bin %MCPX_ROM_1_0%" 0 "!arg_name!"
        call :do_test "-decomp-krnl bios_bank2.bin %MCPX_ROM_1_0%" 0 "!arg_name!"
        call :do_test "-decomp-krnl bios_bank3.bin %MCPX_ROM_1_0%" 0 "!arg_name!"
        call :do_test "-decomp-krnl bios_bank4.bin %MCPX_ROM_1_0%" 0 "!arg_name!"
    )
if "!test_group!" == "-1.0" goto :exit

REM run original tests for bios greater than or equal to 4817
:mcpx_1_1_bios_tests
    call :run_og_test "bios\og_1_1" "%MCPX_ROM_1_1%"
if "!test_group!" == "-1.1" goto :exit

REM run original tests for 512kb bios (less than 4817)
:mcpx_1_0_512kb_bios_tests
    call :run_og_test "bios\512kb" "%MCPX_ROM_1_0%" "-binsize 512 -romsize 512"

    for %%f in (bios\512kb\*.bin) do (
        set "arg=%%f"
        set arg_name=%%~nf

        call :do_test "-split !arg! -romsize 512" 0 "!arg_name!"
        call :do_test "-combine !arg_name!_bank1.bin !arg_name!_bank2.bin" 0 "!arg_name!"
        call :do_test "-split bios.bin -romsize 512" 0 "!arg_name!"
        call :do_test "-decomp-krnl bios_bank1.bin %MCPX_ROM_1_0% -romsize 512" 0 "!arg_name!"
        call :do_test "-decomp-krnl bios_bank2.bin %MCPX_ROM_1_0% -romsize 512" 0 "!arg_name!"        
    )
if "!test_group!" == "-512" goto :exit

:custom_bios_tests
    REM test custom bios
    for %%f in (bios\custom\*.bin) do (
        set "arg=%%f"
        set arg_name=%%~nf
        REM test custom bios decoding and visor simulation

        call :do_test "-xcode-decode !arg!" 0 "!arg_name!"
        call :do_test "-xcode-decode -ini decode.ini !arg!" 0 "!arg_name!"
        call :do_test "-xcode-decode -ini decode2.ini !arg!" 0 "!arg_name!"
        call :do_test "-xcode-sim !arg!" 0 "!arg_name!"
    )
if "!test_group!" == "-custom" goto :exit

:img_tests
    REM test xb img files
    for %%f in (bios\img\*.img) do (
        set "arg=%%f"
        set arg_name=%%~nf

        call :do_test "-dump-img !arg!" 0 "!arg_name!"
    )
if "!test_group!" == "-img" goto :exit

:exit
:error
    if !jobs_total! equ 0 ( 
        echo No tests were found to run.
    )
    if !error_flag! neq 0 ( 
        echo Test !jobs_total! failed. expected: '!expected_error!' got: '!last_error!'
        REM ask user to if they want to see the log
:ask_log
        set "view_log=n"
        if !enable_logs! neq 1 (
            set /p "view_log=Rerun? (y/n): "
            if /i "!view_log!" == "y" (
                cls                
                echo !exe! !cur_job!
                !exe! !cur_job!
                echo.
                goto :ask_log
            )
        ) else (
            if exist !end_log! (
                set /p "view_log=View log? (y/n): "
                if /i "!view_log!" == "y" (
                    cls
                    echo !exe! !cur_job!
                    type !end_log!
                )
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
    set "cmd=!cmd:~1!"

    if "%expected_error%" == "" set "expected_error=0"
    
    set /a jobs_total+=1

    echo.
    echo Test !jobs_total! '!cur_job!'

    if !enable_logs! neq 1 (
        set "end_log="
        !exe! !cur_job! > nul 2> nul
    ) else (
        if "!job_name!" == "" ( set "job_name=last_test" ) else ( set "job_name=!cmd!_!job_name!" )
        set "end_log=logs\!job_name!.log"
        !exe! !cur_job! > !end_log! 2>&1
    )    
    set last_error=!errorlevel!

    if !errorlevel! neq !expected_error! (
        set error_flag=!last_error!
        exit /b 0
    )
    echo Pass.

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

        call :do_test "-decomp-krnl !arg! !mcpx_rom! !extra_args! -out !arg_name!_krnl.img" 0 "!arg_name!"
        call :do_test "-ls !arg! !mcpx_rom! !extra_args!" 0 "!arg_name!"
        call :do_test "-ls !arg! -nv2a -datatbl -dump-krnl !mcpx_rom! !extra_args!" 0 "!arg_name!"
        call :do_test "-xcode-decode !arg! !extra_args!" 0 "!arg_name!"
        call :do_test "-xcode-decode -ini decode.ini !arg!" 0 "!arg_name!"
        call :do_test "-xcode-decode -ini decode2.ini !arg!" 0 "!arg_name!"

        REM test extracting bios 
        call :do_test "-extr !arg! !mcpx_rom! !extra_args!" 0
        REM build that extracted bios ; replicate to 1mb ; encrypt kernel; encrypting with mcpx 1.0
        call :do_test "-bld -bldr bldr.bin -inittbl inittbl.bin -krnl krnl.bin -krnldata krnl_data.bin %MCPX_ROM_1_0% -enc-krnl !extra_args! -binsize 1024 -out bios.bin" 0
        REM test built bios; running -ls calls most things in the program.
        call :do_test "-ls bios.bin %MCPX_ROM_1_0% !extra_args!" 0
    )
    exit /b 0
