ECHO off

REM Get service port from parameters
IF "%1"=="" (
    ECHO You need to give Relocalization service port as first parameter!
    GOTO end
) ELSE (
    ECHO Relocalization service port = %1
)

REM Set Relocalization service external URL
SET SERVER_EXTERNAL_URL=172.17.0.1:%1

REM Get Service Manager URL from parameters
IF "%2"=="" (
    ECHO You need to give Service Manager URL as second parameter!
    GOTO end
) ELSE (
    ECHO Service Manager Service URL = %2
)

REM Set Service Manager URL
SET SERVICE_MANAGER_URL=%2

REM Set application log level
REM Log level expected: DEBUG, CRITICAL, ERROR, INFO, TRACE, WARNING
SET SOLAR_LOG_LEVEL=INFO

REM Define path for local configuration files
SET CONFIG_FILE_PATH=%USERPROFILE%\.arcad\config_files\config_files_relocalization

mkdir %CONFIG_FILE_PATH%

docker volume create --driver local --opt type="none" --opt device=%CONFIG_FILE_PATH% --opt o="bind" config_files_relocalization

docker rm -f solarservicerelocalizationcuda

docker run --gpus all -d -p %1:8080 -v config_files_relocalization:/.xpcf -e SERVER_EXTERNAL_URL -e SERVICE_MANAGER_URL -e SOLAR_LOG_LEVEL -e "SERVICE_NAME=SolARServiceRelocalizationCuda" --log-opt max-size=50m -e "SERVICE_TAGS=SolAR" --name solarservicerelocalizationcuda artwin/solar/services/relocalization-cuda-service:latest

:end
