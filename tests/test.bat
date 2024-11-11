@echo off
REM test script for xbios.exe

cls

setlocal enabledelayedexpansion

if "%~1" == "-?" goto :help
if "%~1" == "/?" goto :help

:setup
    
    title xbios testing

    REM ensure we got (cmpsrc.exe) on the PATH.
    where "cmpsrc.exe" 2>nul || (
        echo tommojphillips' "cmpsrc.exe" not found.
	echo https://github.com/tommojphillips/CompareSrc/releases/
        exit /b 1
    )

    REM ensure we we in the correct directory
    if NOT "%CD%\" == "%~dp0" (
        echo Change directory to %~dp0
        exit /b 1
    )

    set "error_flag=0"
    
    set "exe=..\bin\xbios.exe"

    set "MCPX_ROM_1_0=-mcpx mcpx\mcpx_1.0.bin"
    set "MCPX_ROM_1_1=-mcpx mcpx\mcpx_1.1.bin"

    if not exist "mcpx" mkdir mcpx
    if not exist "bios" mkdir bios

    if not exist "bios\preldr" mkdir bios\preldr    
    if not exist "bios\custom" mkdir bios\custom
    if not exist "bios\custom_512kb_noenc" mkdir bios\custom_512kb_noenc

    if not exist "bios\og_1_0" mkdir bios\og_1_0
    if not exist "bios\og_1_1" mkdir bios\og_1_1
    if not exist "bios\512kb" mkdir bios\512kb
    if not exist "bios\img" mkdir bios\img
    if not exist "logs\" mkdir logs
    
    set "x3_preldr=bios\preldr\x3preldr.bin"
    
    if not exist "decode.ini" echo. > decode.ini

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

    if not exist "!x3_preldr!" (
        echo bone stock x3 preldr not found. Used to verify preldr was extracted correctly. ^( for BIOSes ^>= 4817 ^)
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
    call :do_test "-ls very_very_very_very_very_very_very_very_very_very_very_very_very_very_very_very_very_very_very_very_very_very_very_very_long_file_name.bin" 1
    call :do_test "-ls_very_very_very_very_very_very_very_very_very_very_very_very_very_very_very_very_very_very_very_very_very_very_very_very_long_command.bin bios.bin" 1

    REM ensure we got help for ALL commands
    call :do_test "-? -help-all" 0

REM run original tests for bios less than 4817
:mcpx_1_0_bios_tests   
    call :run_og_test "bios\og_1_0" "%MCPX_ROM_1_0%"

    for %%f in (bios\og_1_0\*.bin) do (
        set "arg=%%f"
        set arg_name=%%~nf

        call :do_test "-split !arg!" 0 "!arg_name!"

        REM note this test expects bios to be 1MB
        call :do_test "-combine !arg_name!_bank1.bin !arg_name!_bank2.bin !arg_name!_bank3.bin !arg_name!_bank4.bin" 0 "!arg_name!"
           
        call :cmp_file "!arg_name!_bank1.bin" "!arg_name!_bank2.bin"
        call :cmp_file "!arg_name!_bank1.bin" "!arg_name!_bank3.bin"
        call :cmp_file "!arg_name!_bank1.bin" "!arg_name!_bank4.bin"
        
        call :do_test "-ls !arg! -bootable !mcpx_rom! !extra_args!" 0 "!arg_name!"
    )
if "!test_group!" == "-1.0" goto :exit

REM run original tests for bios greater than or equal to 4817
:mcpx_1_1_bios_tests
    call :run_og_test "bios\og_1_1" "%MCPX_ROM_1_1%"

    for %%f in (bios\og_1_1\*.bin) do (
        set "arg=%%f"
        set arg_name=%%~nf
                
        REM test extracting preldr. cmp preldr.
        call :do_test "-extr !arg! %MCPX_ROM_1_1% -preldr preldr.bin" 0
        
        REM compare preldr with REAL preldr. ensure we extracting it exactly as intended.
        call :cmp_file "preldr.bin" "!x3_preldr!"

        REM test get16
        call :do_test "-get16 !arg! %MCPX_ROM_1_1%" 0
        
        call :do_test "-ls !arg! -bootable !mcpx_rom! !extra_args!" 0 "!arg_name!"
    )
if "!test_group!" == "-1.1" goto :exit

REM run original tests for 512kb bios (less than 4817)
:mcpx_1_0_512kb_bios_tests
    call :run_og_test "bios\512kb" "%MCPX_ROM_1_0%" "-binsize 512 -romsize 512"

    for %%f in (bios\512kb\*.bin) do (
        set "arg=%%f"
        set arg_name=%%~nf

        call :do_test "-split !arg! -romsize 512" 0 "!arg_name!"
        call :do_test "-combine !arg_name!_bank1.bin !arg_name!_bank2.bin" 0 "!arg_name!"
        
        call :cmp_file "!arg_name!_bank1.bin" "!arg_name!_bank2.bin"
    )
if "!test_group!" == "-512" goto :exit

:custom_bios_tests
    
    REM custom bios decoding and visor simulation
    for %%f in (bios\custom\*.bin) do (
        set "arg=%%f"
        set arg_name=%%~nf
        
        call :do_test "-xcode-decode !arg!" 0 "!arg_name!"
        call :do_test "-xcode-sim !arg!" 0 "!arg_name!"
        call :do_test "-xcode-sim !arg! -d -out mem_sim.bin" 0 "!arg_name!"
        call :do_test "-disasm mem_sim.bin" 0 "!arg_name!"
        
        call :do_test "-ls !arg! -bootable !mcpx_rom! !extra_args!" 0 "!arg_name!"
    )

    REM custom bios that need 512kb and w/ no bldr (2bl) encryption (x2)
    call :run_og_test "bios\custom_512kb_noenc" "" "-romsize 512 -enc-bldr -enc-krnl"

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
        set /p "view_log=Rerun? (y/n): "
        if /i "!view_log!" == "y" (
            cls                
            echo !cur_job!
            !cur_job!
            echo.
            goto :ask_log
        )        
        exit /b 1
    ) else (
        echo.
        echo !jobs_passed!/!jobs_total! tests passed.
    )

    endlocal
    exit /b !last_error!

:do_test
    if NOT !error_flag! == 0 exit /b 0

    set "cur_job=!exe! %~1"
    set "expected_error=%~2"
    set "job_name=%~3"

    if "%~1" == "" exit /b 0

    REM get cmd from cur_job
    for /f "tokens=1 delims= " %%a in ("!cur_job!") do set "cmd=%%a"
    set "cmd=!cmd:~1!"

    if "%expected_error%" == "" set "expected_error=0"
    
    set /a jobs_total+=1

    echo.
    echo Test !jobs_total! '!cur_job!'

    !cur_job! > nul 2> nul
    set last_error=!errorlevel!
    if !errorlevel! neq !expected_error! (
        set error_flag=!last_error!
        exit /b 0
    )

    set /a jobs_passed+=1
    echo Pass.
    
    exit /b 0

:cmp_file
    REM check files match using cmpsrc.exe
	if NOT !error_flag! == 0 exit /b 0

    set /a jobs_total+=1
    set expected_error=0
    set "cur_job=cmpsrc -f %~1 %~2 -c"
    
    echo.
    echo Test !jobs_total! '!cur_job!'

    !cur_job!
    
    set last_error=!errorlevel!
    if !errorlevel! neq !expected_error! (
        set error_flag=!last_error!        
        echo.
        exit /b 0
    )
    
    set /a jobs_passed+=1
    echo Pass.
    
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

        call :do_test "-ls !arg! !mcpx_rom! !extra_args!" 0 "!arg_name!"
        call :do_test "-ls !arg! -nv2a" 0 "!arg_name!"
        call :do_test "-ls !arg! -datatbl" 0 "!arg_name!"
        call :do_test "-ls !arg! -dump-krnl !mcpx_rom! !extra_args!" 0 "!arg_name!"
        
        call :do_test "-xcode-decode !arg!" 0 "!arg_name!"

        REM test extracting bios 
        call :do_test "-extr !arg! !mcpx_rom! !extra_args! -krnl krnl.bin" 0
        
        REM decompress extracted krnl.
        call :do_test "-decompress krnl.bin -out krnl_decompressed.img" 0 "!arg_name!"

        REM cmp decompressed files
        call :cmp_file "krnl_decompressed.img" "krnl.img"

        REM compress decompressed extracted krnl.
        call :do_test "-compress krnl_decompressed.img -out krnl_compressed.bin" 0 "!arg_name!"

        REM cmp compressed files.
        call :cmp_file "krnl.bin" "krnl_compressed.bin"
        
        REM build that extracted bios ; replicate to 1mb ; encrypt kernel; encrypting with mcpx 1.0
        call :do_test "-bld -bldr bldr.bin -inittbl inittbl.bin -krnl krnl.bin -krnldata krnl_data.bin %MCPX_ROM_1_0% -enc-krnl !extra_args! -binsize 1024 -out bios.bin" 0
        
        REM test built bios; running -ls calls most things in the program.
        call :do_test "-ls bios.bin %MCPX_ROM_1_0% !extra_args!" 0
	
	  REM compare decompressed kernel with an already decompressed kernel image to ensure we havent fucked anything up.
        call :cmp_file "krnl.img" "bios\img\!arg_name!_krnl.img"
    )
    exit /b 0
