#!/bin/sh

# Get service port from parameters
if [ "$1" ]
then
    echo "Relocalization service port = $1"
else
    echo "You need to give Relocalization service port as first parameter!"
    exit 1
fi

# Set Relocalization service external URL
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

# Set application log level
# Log level expected: DEBUG, CRITICAL, ERROR, INFO, TRACE, WARNING
export SOLAR_LOG_LEVEL=INFO

# Define path for local configuration files
export CONFIG_FILE_PATH=$HOME/.arcad/config_files/config_files_relocalization

mkdir -p $CONFIG_FILE_PATH

docker volume create \
  --driver local \
  --opt type="none" \
  --opt device=$CONFIG_FILE_PATH \
  --opt o="bind" config_files_relocalization

docker volume create --driver local --opt type="none" --opt device=%CONFIG_FILE_PATH% --opt o="bind" config_files_relocalization

docker rm -f solarservicerelocalizationcuda

docker run --gpus all -d -p $1:8080 \
  -v config_files_relocalization:/.xpcf \
  -e SERVER_EXTERNAL_URL \
  -e SERVICE_MANAGER_URL \
  -e SOLAR_LOG_LEVEL \
  -e "SERVICE_NAME=SolARServiceRelocalizationCuda" \
  --log-opt max-size=50m \
  -e "SERVICE_TAGS=SolAR" \
  --name solarservicerelocalizationcuda artwin/solar/services/relocalization-cuda-service:latest
