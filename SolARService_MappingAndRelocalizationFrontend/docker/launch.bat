ECHO off

REM Get Relocalization Service URL from parameters
IF "%1"=="" (
    ECHO You need to give Relocalization URL as first parameter!
    GOTO end
) ELSE (
    ECHO Relocalization Service URL = %1
)

REM Get Mapping Service URL from parameters
IF "%2"=="" (
    ECHO You need to give Mapping URL as second parameter!
    GOTO end
) ELSE (
    ECHO Mapping Service URL = %2
)

REM Set Relocalization Service URL
SET RELOCALIZATION_SERVICE_URL=%1

REM Set Mapping Service URL
SET MAPPING_SERVICE_URL=%2

REM Set application log level
REM Log level expected: DEBUG, CRITICAL, ERROR, INFO, TRACE, WARNING
SET SOLAR_LOG_LEVEL=INFO

docker rm -f solarservicemappingandrelocalizationfrontend
docker run -d -p 50055:8080 -e SOLAR_LOG_LEVEL -e RELOCALIZATION_SERVICE_URL -e MAPPING_SERVICE_URL -e "SERVICE_NAME=SolARServiceMappingAndRelocalizationFrontend" --log-opt max-size=50m -e "SERVICE_TAGS=SolAR" --name solarservicemappingandrelocalizationfrontend artwin/solar/services/mappingandrelocalizationfrontend-service:latest

:end
