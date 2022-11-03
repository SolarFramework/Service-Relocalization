#!/bin/sh

# Get Map Update Service URL from parameters
if [ "$1" ]
then
    echo "Map Update Service URL = $1"
else
    echo "You need to give Map Update Service URL as 1st parameter!"
    exit 1
fi

# Get Relocalization Service URL from parameters
if [ "$2" ]
then
    echo "Relocalization Service URL = $2"
else
    echo "You need to give Relocalization Service URL as 2nd parameter!"
    exit 1
fi

# Get Relocalization Markers Service URL from parameters
if [ "$3" ]
then
    echo "Relocalization Markers Service URL = $3"
else
    echo "You need to give Relocalization Markers Service URL as 3rd parameter!"
    exit 1
fi

# Get Mapping Service URL from parameters
if [ "$4" ]
then
    echo "Mapping Service URL = $4"
else
    echo "You need to give Mapping Service URL as 4th parameter!"
    exit 1
fi

# Get Stereo Mapping Service URL from parameters
if [ "$5" ]
then
    echo "Stereo Mapping Service URL = $5"
else
    echo "You need to give Stereo Mapping Service URL as 5th parameter!"
    exit 1
fi

# Set Map Update Service URL
export MAPUPDATE_SERVICE_URL=$1

# Set Relocalization Service URL
export RELOCALIZATION_SERVICE_URL=$2

# Set Relocalization Markers Service URL
export RELOCALIZATION_MARKERS_SERVICE_URL=$3

# Set Mapping Service URL
export MAPPING_SERVICE_URL=$4

# Set Stereo Mapping Service URL
export MAPPING_STEREO_SERVICE_URL=$5

# Get output logs to display (if specified)
if [ "$6" ]
then
        echo "Logs to display = $6"
        export DISPLAY_LOG=$6
else
        echo "You can specify the logs to display on console using ENVOY/PROXY/FRONTEND as 6th parameter (all logs by default)"
        export DISPLAY_LOG=ALL
fi

# Set application log level
# Log level expected: DEBUG, CRITICAL, ERROR, INFO, TRACE, WARNING
export SOLAR_LOG_LEVEL=INFO

docker rm -f solarservicemappingandrelocalizationfrontend
docker run -d -p 50055:8080 -p 5000:5000 -p 5001:5001 -p 5002:5002 -p 5003:5003 -p 5004:5004 -p 5005:5005 -p 5006:5006 -p 5007:5007 -p 5008:5008 -p 5009:5009 -e SOLAR_LOG_LEVEL -e MAPUPDATE_SERVICE_URL -e RELOCALIZATION_SERVICE_URL -e RELOCALIZATION_MARKERS_SERVICE_URL -e MAPPING_SERVICE_URL -e MAPPING_STEREO_SERVICE_URL -e DISPLAY_LOG -e "SERVICE_NAME=SolARServiceMappingAndRelocalizationFrontend" --log-opt max-size=50m -e "SERVICE_TAGS=SolAR" --name solarservicemappingandrelocalizationfrontend artwin/solar/services/mappingandrelocalizationfrontend-service:latest
