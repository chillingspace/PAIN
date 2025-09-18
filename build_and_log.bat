@echo off
powershell -Command "& {.\build.bat 2>&1 | Tee-Object -FilePath 'build_log.txt'}"
pause