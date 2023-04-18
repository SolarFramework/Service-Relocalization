ECHO off

REM Get Map Update Service URL from parameters
IF "%1"=="" (
    ECHO You need to give Map Update Service URL as 1st parameter!
    GOTO end
) ELSE (
    ECHO Map Update Service URL = %1
)

REM Set MapUpdate Service URL
SET MAPUPDATE_SERVICE_URL=%1

REM Get MappingAndRelocFrontend Service URL from parameters
IF "%2"=="" (
    ECHO You need to give MappingAndRelocFrontend Service URL as second parameter!
    GOTO end
) ELSE (
    ECHO MappingAndRelocFrontend Service URL = %2
)

REM Set MappingAndRelocFrontend Service URL
SET MAPPINGANDRELOCFRONTEND_SERVICE_URL=%2

REM Get host IP for display
IF "%3"=="" (
    ECHO You need to give host IP address for display as third parameter!
    GOTO end
) ELSE (
    ECHO Set display IP
    SET DISPLAY=%3:0.0
)

ECHO Display IP = %DISPLAY%

REM Set application log level
REM Log level expected: DEBUG, CRITICAL, ERROR, INFO, TRACE, WARNING
SET SOLAR_LOG_LEVEL=INFO

docker rm -f solarservicemappingandrelocfrontendmultiviewer
docker run -it -d -e DISPLAY -e MAPUPDATE_SERVICE_URL -e MAPPINGANDRELOCFRONTEND_SERVICE_URL -e SOLAR_LOG_LEVEL -e "SERVICE_NAME=SolARServiceMappingANdRelocFrontendMultiViewer" -v /tmp/.X11-unix:/tmp/.X11-unix --net=host --log-opt max-size=50m -e "SERVICE_TAGS=SolAR" --name solarservicemappingandrelocfrontendmultiviewer artwin/solar/services/mappingandrelocfrontend-multi-viewer:latest

:end
