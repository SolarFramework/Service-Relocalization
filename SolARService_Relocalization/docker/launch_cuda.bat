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

docker rm -f solarservicerelocalizationcuda
docker run --gpus all -d -p %1:8080 -e SERVER_EXTERNAL_URL -e SERVICE_MANAGER_URL -e SOLAR_LOG_LEVEL -e "SERVICE_NAME=SolARServiceRelocalizationCuda" --log-opt max-size=50m -e "SERVICE_TAGS=SolAR" --name solarservicerelocalizationcuda artwin/solar/services/relocalization-cuda-service:latest

:end
