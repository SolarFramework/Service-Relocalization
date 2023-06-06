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

docker volume create \
  --driver local \
  --opt type="none" \
  --opt device="$HOME/.arcad/config_files/config_files_frontend" \
  --opt o="bind" config_files_frontend

docker rm -f solarservicemappingandrelocalizationfrontend

docker run -d \
-v config_files_frontend:/.xpcf \
-p %1:8080 \
-p 5000:5000 \
-p 5001:5001 \
-p 5002:5002 \
-p 5003:5003 \
-p 5004:5004 \
-p 5005:5005 \
-p 5006:5006 \
-p 5007:5007 \
-p 5008:5008 \
-p 5009:5009 \
-e SOLAR_LOG_LEVEL \
-e SERVICE_MANAGER_URL \
-e DISPLAY_LOG \
-e "SERVICE_NAME=SolARServiceMappingAndRelocalizationFrontend" \
--log-opt max-size=50m \
-e "SERVICE_TAGS=SolAR" \
--name solarservicemappingandrelocalizationfrontend artwin/solar/services/mappingandrelocalizationfrontend-service:latest

:end
