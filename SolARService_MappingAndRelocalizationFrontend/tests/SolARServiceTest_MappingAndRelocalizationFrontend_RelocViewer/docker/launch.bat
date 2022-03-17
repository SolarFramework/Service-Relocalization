ECHO off

REM Get MappingAndRelocalizationFrontend Service URL from parameters
IF "%1"=="" (
    ECHO You need to give MappingAndRelocalizationFrontend Service URL as first parameter!
    GOTO end
) ELSE (
    ECHO MappingAndRelocalizationFrontend Service URL = %1
)

REM Set MappingAndRelocalizationFrontend Service URL
SET MAPPINGANDRELOCALIZATIONFRONTEND_SERVICE_URL=%1

REM Get host IP for display
IF "%2"=="" (
    ECHO You need to give host IP address for display as second parameter!
    GOTO end
) ELSE (
    ECHO Set display IP
    SET DISPLAY=%2:0.0
)

ECHO Display IP = %DISPLAY%

REM Set application log level
REM Log level expected: DEBUG, CRITICAL, ERROR, INFO, TRACE, WARNING
SET SOLAR_LOG_LEVEL=INFO

docker rm -f solarservicemappingandrelocalizationfrontendrelocviewer
docker run -it -d -e DISPLAY -e MAPPINGANDRELOCALIZATIONFRONTEND_SERVICE_URL -e SOLAR_LOG_LEVEL -e "SERVICE_NAME=SolARServiceMappingANdRelocalizationFrontendRelocViewer" -v /tmp/.X11-unix:/tmp/.X11-unix --net=host --log-opt max-size=50m -e "SERVICE_TAGS=SolAR" --name solarservicemappingandrelocalizationfrontendrelocviewer artwin/solar/services/mappingandrelocalizationfrontend-relocviewer:latest

:end
