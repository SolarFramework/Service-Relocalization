#!/bin/sh

# Get Map Update Service URL from parameters
if [ "$1" ]
then
    echo "Map Update Service URL = $1"
else
    echo "You need to give Map Update Service URL as 1st parameter!"
    exit 1
fi

# Set MapUpdate Service URL
export MAPUPDATE_SERVICE_URL=$1

# Get MappingAndRelocFrontend Service URL from parameters
if [ "$2" ]
then
    echo "MappingAndRelocFrontend Service URL = $2"
else
    echo "You need to give MappingAndRelocFrontend Service URL as 2nd parameter!"
    exit 1
fi

# Set MappingAndRelocFrontend Service URL
export MAPPINGANDRELOCFRONTEND_SERVICE_URL=$2

# Get host IP for display
if [ "$3" ]
then
    echo "Display IP = $3"
else
    echo "You need to give host IP address for display as 3rd parameter!"
    exit 1
fi

# Set Display IP
export DISPLAY=$3:0.0

# Set application log level
# Log level expected: DEBUG, CRITICAL, ERROR, INFO, TRACE, WARNING
export SOLAR_LOG_LEVEL=INFO

docker rm -f solarservicemappingandrelocfrontendmultiviewer
docker run -it -d -e DISPLAY -e MAPUPDATE_SERVICE_URL -e MAPPINGANDRELOCFRONTEND_SERVICE_URL -e SOLAR_LOG_LEVEL -e "SERVICE_NAME=SolARServiceMappingAndRelocFrontendMultiViewer" -v /tmp/.X11-unix:/tmp/.X11-unix --net=host --log-opt max-size=50m -e "SERVICE_TAGS=SolAR" --name solarservicemappingandrelocfrontendmultiviewer artwin/solar/services/mappingandrelocfrontend-multi-viewer:latest
