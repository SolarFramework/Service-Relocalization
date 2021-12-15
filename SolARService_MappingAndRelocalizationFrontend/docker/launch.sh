#!/bin/sh

# Get Relocalization Service URL from parameters
if [ "$1" ]
then
    echo "Relocalization Service URL = $1"
else
    echo "You need to give Relocalization Service URL as parameter!"
    exit 1
fi

# Get Mapping Service URL from parameters
if [ "$2" ]
then
    echo "Mapping Service URL = $2"
else
    echo "You need to give Mapping Service URL as parameter!"
    exit 1
fi

# Set Relocalization Service URL
export RELOCALIZATION_SERVICE_URL=$1

# Set Mapping Service URL
export MAPPING_SERVICE_URL=$2

# Set application log level
# Log level expected: DEBUG, CRITICAL, ERROR, INFO, TRACE, WARNING
export SOLAR_LOG_LEVEL=INFO

docker rm -f solarservicemappingandrelocalizationfrontend
docker run -d -p 50055:8080 -p 5001:5001 -e SOLAR_LOG_LEVEL -e RELOCALIZATION_SERVICE_URL -e MAPPING_SERVICE_URL -e "SERVICE_NAME=SolARServiceMappingAndRelocalizationFrontend" --log-opt max-size=50m -e "SERVICE_TAGS=SolAR" --name solarservicemappingandrelocalizationfrontend artwin/solar/services/mappingandrelocalizationfrontend-service:latest
