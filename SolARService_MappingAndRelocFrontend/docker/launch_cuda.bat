ECHO off

REM Get service port from parameters
IF "%1"=="" (
    ECHO You need to give Front End service port as first parameter!
    GOTO end
) ELSE (
    ECHO Front End service port = %1
)

REM Set Front End service external URL
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

REM Get output logs to display (if specified)
IF "%3"=="" (
        ECHO You can specify the logs to display on console using ENVOY/PROXY/FRONTEND as 3rd parameter (all logs by default)
        SET DISPLAY_LOG=ALL
) ELSE (
        ECHO Logs to display = %3
        SET DISPLAY_LOG=%3
)

REM Set application log level
REM Log level expected: DEBUG, CRITICAL, ERROR, INFO, TRACE, WARNING
SET SOLAR_LOG_LEVEL=INFO

REM Define path for local configuration files
SET CONFIG_FILE_PATH=%USERPROFILE%\.arcad\config_files\config_files_frontend

mkdir %CONFIG_FILE_PATH%

docker volume create --driver local --opt type="none" --opt device=%CONFIG_FILE_PATH% --opt o="bind" config_files_frontend

docker rm -f solarservicemappingandrelocalizationfrontend

docker run -d -v config_files_frontend:/.xpcf -p %1:8080 -p 5100:5000 -p 5101:5001 -p 5102:5002-p 5103:5003 -p 5104:5004 -p 5105:5005 -p 5106:5006 -p 5107:5007 -p 5108:5008 -p 5109:5009 -e SOLAR_LOG_LEVEL -e SERVICE_MANAGER_URL -e DISPLAY_LOG -e "SERVICE_NAME=SolARServiceMappingAndRelocalizationFrontendCuda" --log-opt max-size=50m -e "SERVICE_TAGS=SolAR" --name solarservicemappingandrelocalizationfrontend artwin/solar/services/mappingandrelocalizationfrontend-service:latest

:end
