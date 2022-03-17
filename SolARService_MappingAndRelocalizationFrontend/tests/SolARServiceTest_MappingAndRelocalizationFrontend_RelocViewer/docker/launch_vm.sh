#!/bin/sh

# Get MappingAndRelocalizationFrontend Service URL from parameters
if [ "$1" ]
then
    echo "MappingAndRelocalizationFrontend Service URL = $1"
else
    echo "You need to give MappingAndRelocalizationFrontend Service URL as parameter!"
    exit 1
fi

# Set MappingAndRelocalizationFrontend Service URL
export MAPPINGANDRELOCALIZATIONFRONTEND_SERVICE_URL=$1

# Set Display
export DISPLAY=${DISPLAY}
xhost local:docker

# Set application log level
# Log level expected: DEBUG, CRITICAL, ERROR, INFO, TRACE, WARNING
export SOLAR_LOG_LEVEL=INFO

docker rm -f solarservicemappingandrelocalizationfrontendrelocviewer
docker run -it -d -e DISPLAY -e MAPPINGANDRELOCALIZATIONFRONTEND_SERVICE_URL -e SOLAR_LOG_LEVEL -e "SERVICE_NAME=SolARServiceMappingANdRelocalizationFrontendRelocViewer" -v /tmp/.X11-unix:/tmp/.X11-unix --net=host --log-opt max-size=50m -e "SERVICE_TAGS=SolAR" --name solarservicemappingandrelocalizationfrontendrelocviewer artwin/solar/services/mappingandrelocalizationfrontend-relocviewer:latest
