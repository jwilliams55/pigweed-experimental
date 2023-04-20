:<<"::WINDOWS_ONLY"
@echo off
:: Copyright 2021 The Pigweed Authors
::
:: Licensed under the Apache License, Version 2.0 (the "License"); you may not
:: use this file except in compliance with the License. You may obtain a copy of
:: the License at
::
::     https://www.apache.org/licenses/LICENSE-2.0
::
:: Unless required by applicable law or agreed to in writing, software
:: distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
:: WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
:: License for the specific language governing permissions and limitations under
:: the License.
::WINDOWS_ONLY
:; echo "ERROR: Attempting to run Windows .bat from a Unix/POSIX shell!"
:; echo "Instead, run the following command."
:; echo ""
:; echo "    source ./activate.sh"
:; echo ""
:<<"::WINDOWS_ONLY"

:: The bootstrap.bat must be run initially to install all required programs.
:: After that, use activate.bat to enter the environment in a shell.
:: ~dp0 is the batchism for the directory in which a .bat file resides.
set "PIGWEED_EXPERIMENTAL_ROOT=%~dp0."

:: Allow forcing a specific Python version through the environment variable
:: PW_BOOTSTRAP_PYTHON. Otherwise, use the system Python if one exists.
if not "%PW_BOOTSTRAP_PYTHON%" == "" (
  set "python=%PW_BOOTSTRAP_PYTHON%"
  goto find_environment_root
)

:: Detect python installation.
where python >NUL 2>&1
if %ERRORLEVEL% EQU 0 (
  set "python=python"
  goto find_environment_root
)

echo.
echo Error: no system Python present
echo.
echo   Pigweed's bootstrap process requires a local system Python.
echo   Please install Python on your system, add it to your PATH
echo   and re-try running bootstrap.
goto finish

:find_environment_root
set "PW_PROJECT_ROOT=%PIGWEED_EXPERIMENTAL_ROOT%"
set "PW_ROOT=%PIGWEED_EXPERIMENTAL_ROOT%\third_party\pigweed"

:: Set your project's banner and color.
set "PW_BRANDING_BANNER=%PIGWEED_EXPERIMENTAL_ROOT%\banner.txt"
set "PW_BRANDING_BANNER_COLOR=cyan"

:: PW_ENVIRONMENT_ROOT allows developers to specify where the environment should
:: be installed. _PW_ACTUAL_ENVIRONMENT_ROOT is where Pigweed keeps that
:: information. This separation allows Pigweed to assume PW_ENVIRONMENT_ROOT
:: came from the developer and not from a previous bootstrap possibly from
:: another workspace.
:: Not prefixing environment with "." since that doesn't hide it anyway.

if "%PW_ENVIRONMENT_ROOT%"=="" (
  :: Set the environment directory location to the root of this repo.
  set "_PW_ACTUAL_ENVIRONMENT_ROOT=%PIGWEED_EXPERIMENTAL_ROOT%\environment"
) else (
  :: Set environment where the user specified.
  set "_PW_ACTUAL_ENVIRONMENT_ROOT=%PW_ENVIRONMENT_ROOT%"
)

set "shell_file=%_PW_ACTUAL_ENVIRONMENT_ROOT%\activate.bat"

set "_pw_start_script=%PW_ROOT%\pw_env_setup\py\pw_env_setup\windows_env_start.py"

:: If PW_SKIP_BOOTSTRAP is set, only run the activation stage instead of the
:: complete env_setup.
if not "%PW_SKIP_BOOTSTRAP%" == "" goto skip_bootstrap

:: Without the trailing slash in %PW_ROOT%/, batch combines that token with
:: the --shell-file argument.
call "%python%" "%PW_ROOT%\pw_env_setup\py\pw_env_setup\env_setup.py" ^
    --pw-root "%PW_ROOT%" ^
    --shell-file "%shell_file%" ^
    --install-dir "%_PW_ACTUAL_ENVIRONMENT_ROOT%" ^
    --config-file "%PIGWEED_EXPERIMENTAL_ROOT%\pigweed.json" ^
    --project-root "%PW_PROJECT_ROOT%"
goto activate_shell

:skip_bootstrap
if exist "%shell_file%" (
  call "%python%" "%_pw_start_script%"
) else (
  call "%python%" "%_pw_start_script%" --no-shell-file
  goto finish
)
:activate_shell
call "%shell_file%"

:: Add user-defined initial setup here.

:finish
::WINDOWS_ONLY

