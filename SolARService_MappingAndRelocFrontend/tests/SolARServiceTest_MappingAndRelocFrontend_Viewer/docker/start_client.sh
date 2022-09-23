#!/bin/sh

cd /SolARServiceMappingAndRelocFrontendViewer

## Detect MAPUPDATE_SERVICE_URL var and use its value
## to set the Map Update service URL in XML configuration file

if [ -z "$MAPUPDATE_SERVICE_URL" ]
then
    echo "Error: You must define MAPUPDATE_SERVICE_URL env var with the MapUpdate Service URL"
    exit 1
else
    echo "MAPUPDATE_SERVICE_URL defined: $MAPUPDATE_SERVICE_URL"
fi

echo "Try to replace the MapUpdate Service URL in the XML configuration file..."

sed -i -e "s/MAPUPDATE_SERVICE_URL/$MAPUPDATE_SERVICE_URL/g" /.xpcf/SolARServiceTest_MappingAndRelocFrontend_Viewer_conf.xml

## Detect MAPPINGANDRELOCFRONTEND_SERVICE_URL var and use its value
## to set the MappingAndRelocFrontend service URL in XML configuration file

if [ -z "$MAPPINGANDRELOCFRONTEND_SERVICE_URL" ]
then
    echo "Error: You must define MAPPINGANDRELOCFRONTEND_SERVICE_URL env var with the MappingAndRelocFrontend Service URL"
    exit 1
else
    echo "MAPPINGANDRELOCFRONTEND_SERVICE_URL defined: $MAPPINGANDRELOCFRONTEND_SERVICE_URL"
fi

echo "Try to replace the MappingAndRelocFrontend Service URL in the XML configuration file..."

sed -i -e "s/MAPPINGANDRELOCFRONTEND_SERVICE_URL/$MAPPINGANDRELOCFRONTEND_SERVICE_URL/g" /.xpcf/SolARServiceTest_MappingAndRelocFrontend_Viewer_conf.xml

echo "XML configuration file ready"

## Set application log level
## Log level expected: DEBUG, CRITICAL, ERROR, INFO, TRACE, WARNING
export SOLAR_LOG_LEVEL=INFO

export LD_LIBRARY_PATH=.:./modules/

## Start client
./SolARServiceTest_MappingAndRelocFrontend_Viewer -f /.xpcf/SolARServiceTest_MappingAndRelocFrontend_Viewer_conf.xml

