#!/bin/sh

# Set application log level
# Log level expected: DEBUG, CRITICAL, ERROR, INFO, TRACE, WARNING
export SOLAR_LOG_LEVEL=INFO

docker rm -f solarservicerelocalizationmarkers
docker run -d -p 50050:8080 -e SOLAR_LOG_LEVEL -e "SERVICE_NAME=SolARServiceRelocalizationMarkers" --log-opt max-size=50m -e "SERVICE_TAGS=SolAR" --name solarservicerelocalizationmarkers artwin/solar/services/relocalization-markers-service:latest