#!/bin/sh

# Get service port from parameters
if [ "$1" ]
then
    echo "Front End service port = $1"
else
    echo "You need to give Front End service port as first parameter!"
    exit 1
fi

# Set Front End service external URL
export SERVER_EXTERNAL_URL=172.17.0.1:$1

# Get Service Manager URL from parameters
if [ "$2" ]
then
    echo "Service Manager URL = $2"
else
    echo "You need to give Service Manager URL as second parameter!"
    exit 1
fi

# Set Service Manager URL
export SERVICE_MANAGER_URL=$2

# Get output logs to display (if specified)
if [ "$3" ]
then
        echo "Logs to display = $3"
        export DISPLAY_LOG=$3
else
        echo "You can specify the logs to display on console using ENVOY/PROXY/FRONTEND as 3rd parameter (all logs by default)"
        export DISPLAY_LOG=ALL
fi

# Set application log level
# Log level expected: DEBUG, CRITICAL, ERROR, INFO, TRACE, WARNING
export SOLAR_LOG_LEVEL=INFO

# Define path for local configuration files
export CONFIG_FILE_PATH=$HOME/.arcad/config_files/config_files_mappingandrelocalizationfrontend

mkdir -p $CONFIG_FILE_PATH

docker volume create \
  --driver local \
  --opt type="none" \
  --opt device=$CONFIG_FILE_PATH \
  --opt o="bind" config_files_mappingandrelocalizationfrontend

docker rm -f solarservicemappingandrelocalizationfrontendcuda

docker run -d \
  -v config_files_mappingandrelocalizationfrontend:/.xpcf \
  -p $1:8080 \
  -p 5100:5000 \
  -p 5101:5001 \
  -p 5102:5002 \
  -p 5103:5003 \
  -p 5104:5004 \
  -p 5105:5005 \
  -p 5106:5006 \
  -p 5107:5007 \
  -p 5108:5008 \
  -p 5109:5009 \
  -e SOLAR_LOG_LEVEL \
  -e SERVICE_MANAGER_URL \
  -e DISPLAY_LOG \
  -e "SERVICE_NAME=SolARServiceMappingAndRelocalizationFrontendCuda" \
  --log-opt max-size=50m \
  -e "SERVICE_TAGS=SolAR" \
  --name solarservicemappingandrelocalizationfrontendcuda artwin/solar/services/mappingandrelocalizationfrontend-service:latest
