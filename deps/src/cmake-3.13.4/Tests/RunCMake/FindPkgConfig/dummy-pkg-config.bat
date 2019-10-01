@ECHO OFF

:LOOP

IF "%1"=="" (
  EXIT /B 255
)

IF "%1"=="--version" (
  ECHO 0.0-cmake-dummy
  EXIT /B 0
)

IF "%1"=="--exists" (
  SHIFT
  ECHO Expected: %*
  ECHO Found:    %PKG_CONFIG_PATH%
  IF NOT "%*"=="%PKG_CONFIG_PATH%" (
    EXIT /B 1
  ) ELSE (
    EXIT /B 0
  )
)
SHIFT
IF NOT "%~1"=="" GOTO LOOP

EXIT /B 255
