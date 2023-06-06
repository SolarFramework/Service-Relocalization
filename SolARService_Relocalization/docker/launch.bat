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

docker volume create \
  --driver local \
  --opt type="none" \
  --opt device="$HOME/.arcad/config_files/config_files_relocalization" \
  --opt o="bind" config_files_relocalization

docker rm -f solarservicerelocalization

docker run -d -p %1:8080 \
-v config_files_relocalization:/.xpcf \
-e SERVER_EXTERNAL_URL \
-e SERVICE_MANAGER_URL \
-e SOLAR_LOG_LEVEL \
-e "SERVICE_NAME=SolARServiceRelocalization" \
--log-opt max-size=50m \
-e "SERVICE_TAGS=SolAR" \
--name solarservicerelocalization artwin/solar/services/relocalization-service:latest

:end
