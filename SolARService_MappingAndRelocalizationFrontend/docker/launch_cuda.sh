#!/bin/sh

# Get Relocalization Service URL from parameters
if [ "$1" ]
then
    echo "Relocalization Service URL = $1"
else
    echo "You need to give Relocalization Service URL as 1st parameter!"
    exit 1
fi

# Get Relocalization Markers Service URL from parameters
if [ "$2" ]
then
    echo "Relocalization Markers Service URL = $2"
else
    echo "You need to give Relocalization Markers Service URL as 2nd parameter!"
    exit 1
fi

# Get Mapping Service URL from parameters
if [ "$3" ]
then
    echo "Mapping Service URL = $3"
else
    echo "You need to give Mapping Service URL as 3rd parameter!"
    exit 1
fi

# Set Relocalization Service URL
export RELOCALIZATION_SERVICE_URL=$1

# Set Relocalization Markers Service URL
export RELOCALIZATION_MARKERS_SERVICE_URL=$2

# Set Mapping Service URL
export MAPPING_SERVICE_URL=$3

# Get output logs to display (if specified)
if [ "$4" ]
then
        echo "Logs to display = $4"
        export DISPLAY_LOG=$4
else
        echo "You can specify the logs to display on console using ENVOY/PROXY/FRONTEND as 4th parameter (all logs by default)"
        export DISPLAY_LOG=ALL
fi

# Set application log level
# Log level expected: DEBUG, CRITICAL, ERROR, INFO, TRACE, WARNING
export SOLAR_LOG_LEVEL=INFO

docker rm -f solarservicemappingandrelocalizationfrontendcuda
docker run -d -p 60055:8080 -p 5100:5000 -p 5101:5001 -p 5102:5002 -p 5103:5003 -p 5104:5004 -p 5105:5005 -p 5106:5006 -p 5107:5007 -p 5108:5008 -p 5109:5009 -e SOLAR_LOG_LEVEL -e RELOCALIZATION_SERVICE_URL -e RELOCALIZATION_MARKERS_SERVICE_URL -e MAPPING_SERVICE_URL -e DISPLAY_LOG -e "SERVICE_NAME=SolARServiceMappingAndRelocalizationFrontendCuda" --log-opt max-size=50m -e "SERVICE_TAGS=SolAR" --name solarservicemappingandrelocalizationfrontendcuda artwin/solar/services/mappingandrelocalizationfrontend-service:latest
