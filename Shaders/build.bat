
@echo off
setlocal

set "base_dir=%cd%"
set "ignore_name=build.bat"

for /r %%f in (*) do (
    set "full_path=%%f"
    setlocal enabledelayedexpansion
    set "rel_path=!full_path:%base_dir%\=!"
    if "!rel_path:~0,1!" == "\" (
        set "rel_path=!rel_path:~1!"
    )
    
    if /i not "!rel_path!"=="%ignore_name%" (
        echo !rel_path!
        shader_importer !rel_path!
        echo:
    )

    endlocal
)

endlocal