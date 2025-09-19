@echo off
powershell -Command "& {.\script_build_and_run.bat 2>&1 | Tee-Object -FilePath 'build_log.txt'}"
pause