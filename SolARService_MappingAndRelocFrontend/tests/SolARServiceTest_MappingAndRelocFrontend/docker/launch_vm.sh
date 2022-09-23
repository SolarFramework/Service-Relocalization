#!/bin/sh

# Get MappingAndRelocFrontend Service URL from parameters
if [ "$1" ]
then
    echo "MappingAndRelocFrontend Service URL = $1"
else
    echo "You need to give MappingAndRelocFrontend Service URL as parameter!"
    exit 1
fi

# Set MappingAndRelocFrontend Service URL
export MAPPINGANDRELOCFRONTEND_SERVICE_URL=$1

# Set Display
export DISPLAY=${DISPLAY}
xhost local:docker

# Set application log level
# Log level expected: DEBUG, CRITICAL, ERROR, INFO, TRACE, WARNING
export SOLAR_LOG_LEVEL=INFO

docker rm -f solarservicemappingandrelocfrontendclient
docker run -it -d -e DISPLAY -e MAPPINGANDRELOCFRONTEND_SERVICE_URL -e SOLAR_LOG_LEVEL -e "SERVICE_NAME=SolARServiceMappingAndRelocFrontendClient" -v /tmp/.X11-unix:/tmp/.X11-unix --net=host --log-opt max-size=50m -e "SERVICE_TAGS=SolAR" --name solarservicemappingandrelocfrontendclient artwin/solar/services/mappingandrelocfrontend-client:latest
