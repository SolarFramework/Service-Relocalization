ECHO off

REM Get Map Update Service URL from parameters
IF "%1"=="" (
    ECHO You need to give Map Update Service URL as first parameter!
    GOTO end
) ELSE (
    ECHO Map Update Service URL = %1
)

REM Get Relocalization Service URL from parameters
IF "%2"=="" (
    ECHO You need to give Relocalization Service URL as 2nd parameter!
    GOTO end
) ELSE (
    ECHO Relocalization Service URL = %2
)

REM Get Relocalization Markers Service URL from parameters
IF "%3"=="" (
    ECHO You need to give Relocalization Markers Service URL as 3rd parameter!
    GOTO end
) ELSE (
    ECHO Relocalization Markers Service URL = %3
)

REM Get Mapping Service URL from parameters
IF "%4"=="" (
    ECHO You need to give Mapping Service URL as 4th parameter!
    GOTO end
) ELSE (
    ECHO Mapping Service URL = %4
)

REM Get Mapping Stereo Service URL from parameters
IF "%5"=="" (
    ECHO You need to give Mapping Stereo Service URL as 5th parameter (or a fake URL if no stereo service is available)!
    GOTO end
) ELSE (
    ECHO Mapping Service URL = %5
)

REM Set Map Update Service URL
SET MAPUPDATE_SERVICE_URL=%1

REM Set Relocalization Service URL
SET RELOCALIZATION_SERVICE_URL=%2

REM Set Relocalization Markers Service URL
SET RELOCALIZATION_MARKERS_SERVICE_URL=%3

REM Set Mapping Service URL
SET MAPPING_SERVICE_URL=%4

REM Set Mapping Stereo Service URL
SET MAPPING_STEREO_SERVICE_URL=%5

REM Get output logs to display (if specified)
IF "%5"=="" (
        ECHO You can specify the logs to display on console using ENVOY/PROXY/FRONTEND as 5th parameter (all logs by default)
        SET DISPLAY_LOG=ALL
) ELSE (
        ECHO Logs to display = %5
        SET DISPLAY_LOG=%5
)

REM Set application log level
REM Log level expected: DEBUG, CRITICAL, ERROR, INFO, TRACE, WARNING
SET SOLAR_LOG_LEVEL=INFO

docker rm -f solarservicemappingandrelocfrontend
docker run -d -p 50055:8080 -p 5000:5000 -p 5001:5001 -p 5002:5002 -p 5003:5003 -p 5004:5004 -p 5005:5005 -p 5006:5006 -p 5007:5007 -p 5008:5008 -p 5009:5009 -e SOLAR_LOG_LEVEL -e MAPUPDATE_SERVICE_URL -e RELOCALIZATION_SERVICE_URL -e RELOCALIZATION_MARKERS_SERVICE_URL -e MAPPING_SERVICE_URL -e DISPLAY_LOG -e "SERVICE_NAME=SolARServiceMappingAndRelocFrontend" --log-opt max-size=50m -e "SERVICE_TAGS=SolAR" --name solarservicemappingandrelocfrontend artwin/solar/services/mappingandrelocfrontend-service:latest

:end
