#!/bin/sh

## Detect MAPPINGANDRELOCALIZATIONFRONTEND_SERVICE_URL var and use its value
## to set the MappingAndRelocalizationFrontend service URL in XML configuration file

cd /SolARServiceMappingAndRelocalizationFrontendClient

if [ -z "$MAPPINGANDRELOCALIZATIONFRONTEND_SERVICE_URL" ]
then
    echo "Error: You must define MAPPINGANDRELOCALIZATIONFRONTEND_SERVICE_URL env var with the MappingAndRelocalizationFrontend Service URL"
    exit 1 
else
    echo "MAPPINGANDRELOCALIZATIONFRONTEND_SERVICE_URL defined: $MAPPINGANDRELOCALIZATIONFRONTEND_SERVICE_URL"
fi

echo "Try to replace the MappingAndRelocalizationFrontend Service URL in the XML configuration file..."

sed -i -e "s/MAPPINGANDRELOCALIZATIONFRONTEND_SERVICE_URL/$MAPPINGANDRELOCALIZATIONFRONTEND_SERVICE_URL/g" /.xpcf/SolARServiceTest_MappingAndRelocalizationFrontend_conf.xml

echo "XML configuration file ready"

## Set application log level
## Log level expected: DEBUG, CRITICAL, ERROR, INFO, TRACE, WARNING
export SOLAR_LOG_LEVEL=INFO

export LD_LIBRARY_PATH=.:./modules/

## Start client
./SolARServiceTest_MappingAndRelocalizationFrontend -f /.xpcf/SolARServiceTest_MappingAndRelocalizationFrontend_conf.xml

