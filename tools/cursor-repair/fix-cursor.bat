@echo off
setlocal
title Cursor Repair Tool

set "PROFILE_DIR=%APPDATA%\Cursor"
set "INSTALL_USER=%LOCALAPPDATA%\Programs\cursor"
set "INSTALL_SYS=%ProgramFiles%\cursor"
set "UPDATER_DIR=%LOCALAPPDATA%\cursor-updater"
set "CLI_DIR=%USERPROFILE%\.cursor"

set "DESK=%USERPROFILE%"
if exist "%USERPROFILE%\Desktop" set "DESK=%USERPROFILE%\Desktop"
if defined OneDrive if exist "%OneDrive%\Desktop" set "DESK=%OneDrive%\Desktop"
set "LOG=%DESK%\cursor-diagnostics.txt"

set "TS="
for /f "usebackq delims=" %%i in (`powershell -NoProfile -Command "Get-Date -Format yyyyMMdd-HHmmss"`) do set "TS=%%i"
if not defined TS set "TS=backup"

set "ADMIN=no"
net session >nul 2>&1
if %errorlevel%==0 set "ADMIN=yes"

:menu
cls
call :findexe
echo ==========================================================
echo   CURSOR REPAIR TOOL
echo ==========================================================
echo   Admin rights : %ADMIN%
echo   Profile      : %PROFILE_DIR%
echo   Cursor.exe   : %EXESHOW%
echo   Report file  : %LOG%
if "%DUAL%"=="yes" echo   WARNING      : two installs found, user and system wide
echo ==========================================================
echo.
echo   Try to start Cursor after each step.
echo.
echo   1 - Diagnostics. Writes a report. Changes nothing.
echo   2 - Kill Cursor processes and clear cache. Safe.
echo   3 - Reset profile. Keeps a backup and your settings.
echo   4 - Prepare clean reinstall. Deletes program files only.
echo   5 - Add antivirus exclusions. Needs admin rights.
echo   6 - Start Cursor without extensions and without GPU.
echo   7 - Undo step 3. Restores the profile from backup.
echo   8 - Repair settings.json. Fixes Cursor closing at once.
echo   9 - Run 1, 2, 8, 3 and 6 in a row. Recommended.
echo   0 - Exit
echo.
set "CH="
set /p "CH=Enter number and press Enter: "

if "%CH%"=="1" goto diag_menu
if "%CH%"=="2" goto kill_menu
if "%CH%"=="3" goto reset_menu
if "%CH%"=="4" goto cleaninstall
if "%CH%"=="5" goto exclusions
if "%CH%"=="6" goto launch_menu
if "%CH%"=="7" goto restore
if "%CH%"=="8" goto fix_menu
if "%CH%"=="9" goto autorun
if "%CH%"=="0" goto done
goto menu

:diag_menu
call :diag
pause
goto menu

:kill_menu
call :killcache
pause
goto menu

:reset_menu
call :resetprofile ask
pause
goto menu

:launch_menu
call :launch
pause
goto menu

:fix_menu
call :fixsettings
pause
goto menu

:autorun
echo.
echo Steps 1, 2, 8, 3 and 6 will run now.
echo Step 3 moves your profile to a backup folder named Cursor.bak-%TS% .
echo Nothing is deleted. Option 7 puts everything back.
echo.
set "GO="
set /p "GO=Continue? Type y and press Enter: "
if /i not "%GO%"=="y" goto menu
call :diag
call :killcache
call :fixsettings
call :resetprofile force
call :launch
echo.
echo ==========================================================
echo   All steps finished.
echo   If Cursor still does not open, run option 5, then 4.
echo   Send this file in the chat:
echo   %LOG%
echo ==========================================================
echo.
pause
goto menu

:cleaninstall
echo.
echo This deletes the Cursor program files only.
echo Settings and chats in the profile folder stay untouched.
echo Then install Cursor again from the download page.
echo.
set "OK="
set /p "OK=Continue? Type y and press Enter: "
if /i not "%OK%"=="y" goto menu
call :killproc
if exist "%INSTALL_USER%" rd /s /q "%INSTALL_USER%" 2>nul
rd /s /q "%UPDATER_DIR%" 2>nul
if exist "%INSTALL_USER%" echo Some files are locked. Restart Windows and run this option again.
if exist "%INSTALL_SYS%\unins000.exe" goto uninst_sys
if exist "%INSTALL_SYS%" goto delete_sys
goto clean_end

:uninst_sys
echo.
echo A system wide install was found in %INSTALL_SYS%
echo Its own uninstaller will open now. Confirm it in that window.
start /wait "" "%INSTALL_SYS%\unins000.exe"
goto clean_end

:delete_sys
if not "%ADMIN%"=="yes" goto clean_needadmin
rd /s /q "%INSTALL_SYS%" 2>nul
if exist "%INSTALL_SYS%" echo Could not remove %INSTALL_SYS% . Remove Cursor in Settings, Apps.
goto clean_end

:clean_needadmin
echo Cursor sits in %INSTALL_SYS% and removing it needs admin rights.
echo Restart this script with Run as administrator.
goto clean_end

:clean_end
echo.
echo Program files removed.
echo Download the installer and run it as administrator.
start "" https://cursor.com/download
echo.
pause
goto menu

:exclusions
echo.
if not "%ADMIN%"=="yes" goto needadmin
echo Adding antivirus exclusions...
if exist "%INSTALL_USER%" powershell -NoProfile -Command "Add-MpPreference -ExclusionPath '%INSTALL_USER%' -ErrorAction SilentlyContinue"
if exist "%INSTALL_SYS%" powershell -NoProfile -Command "Add-MpPreference -ExclusionPath '%INSTALL_SYS%' -ErrorAction SilentlyContinue"
powershell -NoProfile -Command "Add-MpPreference -ExclusionPath '%PROFILE_DIR%' -ErrorAction SilentlyContinue; Add-MpPreference -ExclusionProcess 'Cursor.exe','rg.exe','inno_updater.exe' -ErrorAction SilentlyContinue; Write-Output 'exclusions added'"
echo.
echo If you use another antivirus, add the same two folders there by hand.
echo.
pause
goto menu

:needadmin
echo This option needs admin rights.
echo Close this window, right click fix-cursor.bat, choose Run as administrator.
echo.
pause
goto menu

:restore
echo.
set "BAK="
for /f "delims=" %%d in ('dir /b /ad /o-n "%APPDATA%\Cursor.bak-*" 2^>nul') do if not defined BAK set "BAK=%%d"
if not defined BAK goto nobak
echo Newest backup: %BAK%
echo The current profile will be renamed, then this backup takes its place.
echo.
set "OK="
set /p "OK=Continue? Type y and press Enter: "
if /i not "%OK%"=="y" goto menu
call :killproc
if exist "%PROFILE_DIR%" move "%PROFILE_DIR%" "%APPDATA%\Cursor.fresh-%TS%" >nul 2>&1
move "%APPDATA%\%BAK%" "%PROFILE_DIR%" >nul 2>&1
if exist "%PROFILE_DIR%\User" echo Profile restored with your old settings and chats.
if not exist "%PROFILE_DIR%\User" echo Restore failed. Close every Cursor.exe and try again.
echo.
pause
goto menu

:nobak
echo No backup folder found in %APPDATA%
echo.
pause
goto menu

:done
echo.
echo Report file: %LOG%
echo.
endlocal
exit /b 0

:: ---------------- subroutines ----------------

:findexe
set "EXE="
if exist "%INSTALL_USER%\Cursor.exe" set "EXE=%INSTALL_USER%\Cursor.exe"
if not defined EXE if exist "%INSTALL_SYS%\Cursor.exe" set "EXE=%INSTALL_SYS%\Cursor.exe"
set "EXESHOW=NOT FOUND"
if defined EXE set "EXESHOW=%EXE%"
set "DUAL=no"
if exist "%INSTALL_USER%\Cursor.exe" if exist "%INSTALL_SYS%\Cursor.exe" set "DUAL=yes"
exit /b 0

:fixsettings
echo.
echo Checking settings.json for values that crash startup...
powershell -NoProfile -Command "$f = Join-Path $env:APPDATA 'Cursor\User\settings.json'; if (-not (Test-Path $f)) { Write-Output 'no settings.json, nothing to repair'; exit }; $enc = New-Object System.Text.UTF8Encoding $false; Copy-Item $f ($f + '.bak-%TS%') -Force; $raw = Get-Content -Raw -Path $f; $keep = @(); foreach ($ln in ($raw -split '\r?\n')) { if (-not $ln.TrimStart().StartsWith('//')) { $keep += $ln } }; $clean = $keep -join [char]10; try { $j = ConvertFrom-Json $clean } catch { Move-Item $f ($f + '.broken-%TS%') -Force; [System.IO.File]::WriteAllText($f, '{}', $enc); Write-Output 'settings.json was not valid JSON. Moved aside, empty one created.'; exit }; $fixed = @(); foreach ($k in @('http.noProxy', 'cursor.general.globalIgnoreList', 'files.watcherExclude.list')) { $prop = $j.PSObject.Properties[$k]; if ($prop -and ($prop.Value -is [string])) { $arr = @(); foreach ($part in ($prop.Value -split '[,;]')) { $t = $part.Trim(); if ($t) { $arr += $t } }; $prop.Value = $arr; $fixed += $k } }; if ($fixed.Count -gt 0) { [System.IO.File]::WriteAllText($f, (ConvertTo-Json $j -Depth 30), $enc); Write-Output ('repaired and saved: ' + ($fixed -join ', ')) } else { Write-Output 'no startup crashing value found in settings.json' }"
echo.
echo A copy of the old file is kept next to it with a .bak name.
echo.
exit /b 0

:killproc
taskkill /f /im Cursor.exe >nul 2>&1
taskkill /f /im "Cursor Nightly.exe" >nul 2>&1
taskkill /f /im cursor-updater.exe >nul 2>&1
taskkill /f /im inno_updater.exe >nul 2>&1
wsl --shutdown >nul 2>&1
exit /b 0

:diag
echo.
echo Collecting diagnostics...
echo Cursor diagnostics %TS% >"%LOG%"
echo. >>"%LOG%"
echo === Windows version === >>"%LOG%"
ver >>"%LOG%" 2>&1
echo Admin rights: %ADMIN% >>"%LOG%"
echo. >>"%LOG%"
echo === Cursor processes running now === >>"%LOG%"
tasklist /fi "imagename eq Cursor.exe" >>"%LOG%" 2>&1
echo. >>"%LOG%"
echo === Program files, user install === >>"%LOG%"
set "S=MISSING"
if exist "%INSTALL_USER%\Cursor.exe" set "S=FOUND"
echo %S% %INSTALL_USER%\Cursor.exe >>"%LOG%"
dir /b "%INSTALL_USER%" >>"%LOG%" 2>&1
echo. >>"%LOG%"
echo === Program files, system install === >>"%LOG%"
set "S=MISSING"
if exist "%INSTALL_SYS%\Cursor.exe" set "S=FOUND"
echo %S% %INSTALL_SYS%\Cursor.exe >>"%LOG%"
echo. >>"%LOG%"
echo === settings.json, this is where startup crashes come from === >>"%LOG%"
type "%PROFILE_DIR%\User\settings.json" >>"%LOG%" 2>&1
echo. >>"%LOG%"
echo === Installs recorded by Windows === >>"%LOG%"
reg query "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall" /s /f cursor >>"%LOG%" 2>&1
reg query "HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall" /s /f cursor >>"%LOG%" 2>&1
echo. >>"%LOG%"
echo === Which Cursor.exe would start === >>"%LOG%"
call :findexe
echo launch target: %EXESHOW% >>"%LOG%"
echo two installs at once: %DUAL% >>"%LOG%"
echo. >>"%LOG%"
echo === Leftovers from a broken update === >>"%LOG%"
set "S=none"
if exist "%INSTALL_USER%\_" set "S=present"
echo leftover _ folder: %S% >>"%LOG%"
set "S=none"
if exist "%UPDATER_DIR%" set "S=present"
echo cursor-updater folder: %S% >>"%LOG%"
echo. >>"%LOG%"
echo === Profile folder === >>"%LOG%"
set "S=MISSING"
if exist "%PROFILE_DIR%" set "S=FOUND"
echo %S% %PROFILE_DIR% >>"%LOG%"
set "S=no"
if exist "%PROFILE_DIR%\User\globalStorage\state.vscdb" set "S=yes"
echo chat history file present: %S% >>"%LOG%"
dir /b "%PROFILE_DIR%" >>"%LOG%" 2>&1
echo. >>"%LOG%"
echo === Windows Cursor tries to reopen at startup === >>"%LOG%"
dir /b "%PROFILE_DIR%\User\workspaceStorage" >>"%LOG%" 2>&1
echo. >>"%LOG%"
echo === Existing profile backups === >>"%LOG%"
dir /b /ad "%APPDATA%\Cursor.bak-*" >>"%LOG%" 2>&1
echo. >>"%LOG%"
echo === CLI folder, this one is kept on purpose === >>"%LOG%"
set "S=MISSING"
if exist "%CLI_DIR%" set "S=FOUND"
echo %S% %CLI_DIR% >>"%LOG%"
echo. >>"%LOG%"
echo === Tail of main.log === >>"%LOG%"
powershell -NoProfile -Command "$p='%PROFILE_DIR%\logs'; if (Test-Path $p) { $f = Get-ChildItem -Path $p -Recurse -Filter main.log -ErrorAction SilentlyContinue | Sort-Object LastWriteTime | Select-Object -Last 1; if ($f) { $f.FullName; Get-Content -Path $f.FullName -Tail 60 } else { 'no main.log found' } } else { 'no logs folder' }" >>"%LOG%" 2>&1
echo. >>"%LOG%"
echo === Antivirus exclusions === >>"%LOG%"
powershell -NoProfile -Command "try { $p = Get-MpPreference; $p.ExclusionPath; $p.ExclusionProcess } catch { 'cannot read Defender settings' }" >>"%LOG%" 2>&1
echo. >>"%LOG%"
echo === Last antivirus detections === >>"%LOG%"
powershell -NoProfile -Command "try { Get-MpThreatDetection | Select-Object -Last 5 | Format-List } catch { 'cannot read quarantine' }" >>"%LOG%" 2>&1
echo.
echo Report saved to:
echo   %LOG%
echo Send that file in the chat.
echo.
exit /b 0

:killcache
echo.
echo Closing Cursor...
call :killproc
echo Clearing cache...
for %%d in (Cache CachedData "Code Cache" GPUCache DawnCache ShaderCache CachedProfilesData) do rd /s /q "%PROFILE_DIR%\%%~d" 2>nul
rd /s /q "%PROFILE_DIR%\Crashpad" 2>nul
echo Done. Cache removed. Settings and chats untouched.
echo.
exit /b 0

:resetprofile
echo.
if not exist "%PROFILE_DIR%" goto reset_none
if "%1"=="force" goto reset_go
echo This moves the profile to a backup folder and starts Cursor fresh.
echo Settings, keybindings and snippets are copied into the new profile.
echo Chat history stays in the backup folder. Option 7 puts it all back.
echo Backup name: Cursor.bak-%TS%
echo.
set "OK="
set /p "OK=Continue? Type y and press Enter: "
if /i not "%OK%"=="y" goto reset_skip

:reset_go
call :killproc
move "%PROFILE_DIR%" "%APPDATA%\Cursor.bak-%TS%" >nul 2>&1
if exist "%PROFILE_DIR%" goto reset_fail
md "%PROFILE_DIR%\User" 2>nul
copy "%APPDATA%\Cursor.bak-%TS%\User\settings.json" "%PROFILE_DIR%\User\settings.json" >nul 2>&1
copy "%APPDATA%\Cursor.bak-%TS%\User\keybindings.json" "%PROFILE_DIR%\User\keybindings.json" >nul 2>&1
xcopy "%APPDATA%\Cursor.bak-%TS%\User\snippets" "%PROFILE_DIR%\User\snippets" /e /i /q >nul 2>&1
call :fixsettings
echo Profile reset. Backup folder:
echo   %APPDATA%\Cursor.bak-%TS%
echo Cursor will now start with an empty window and will not reopen the old folder.
echo.
exit /b 0

:reset_fail
echo Could not move the profile. Cursor is still running.
echo Open Task Manager, end every Cursor.exe, then run option 3 again.
echo.
exit /b 0

:reset_none
echo No profile folder found, nothing to reset.
echo.
exit /b 0

:reset_skip
echo Skipped.
echo.
exit /b 0

:launch
echo.
call :findexe
if not defined EXE goto launch_missing
echo Starting %EXE%
start "" "%EXE%" --disable-extensions --disable-gpu -n
echo.
echo If Cursor opens now, the cause was an extension or the GPU driver.
echo Do not open the VPN folder yet.
echo.
exit /b 0

:launch_missing
echo Cursor.exe not found. The install is broken or an antivirus removed it.
echo Run option 5, then option 4.
echo.
exit /b 0
