ECHO off

REM Get Map Update Service URL from parameters
IF "%1"=="" (
    ECHO You need to give Map Update URL as first parameter!
    GOTO end
) ELSE (
    ECHO Map Update Service URL = %1
)

REM Set Map Update Service URL
SET MAPUPDATE_SERVICE_URL=%1

REM Set application log level
REM Log level expected: DEBUG, CRITICAL, ERROR, INFO, TRACE, WARNING
SET SOLAR_LOG_LEVEL=INFO

docker rm -f solarservicerelocalization
docker run -d -p 50052:8080 -e SOLAR_LOG_LEVEL -e MAPUPDATE_SERVICE_URL -e "SERVICE_NAME=SolARServiceRelocalization" --log-opt max-size=50m -e "SERVICE_TAGS=SolAR" --name solarservicerelocalization artwin/solar/services/relocalization-service:latest

:end
