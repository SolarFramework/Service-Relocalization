#!/bin/sh

cd /SolARServiceMappingAndRelocalizationFrontendRelocViewer

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

sed -i -e "s/MAPUPDATE_SERVICE_URL/$MAPUPDATE_SERVICE_URL/g" /.xpcf/SolARServiceTest_MappingAndRelocalizationFrontend_RelocViewer_conf.xml

## Detect MAPPINGANDRELOCALIZATIONFRONTEND_SERVICE_URL var and use its value
## to set the MappingAndRelocalizationFrontend service URL in XML configuration file

if [ -z "$MAPPINGANDRELOCALIZATIONFRONTEND_SERVICE_URL" ]
then
    echo "Error: You must define MAPPINGANDRELOCALIZATIONFRONTEND_SERVICE_URL env var with the MappingAndRelocalizationFrontend Service URL"
    exit 1
else
    echo "MAPPINGANDRELOCALIZATIONFRONTEND_SERVICE_URL defined: $MAPPINGANDRELOCALIZATIONFRONTEND_SERVICE_URL"
fi

echo "Try to replace the MappingAndRelocalizationFrontend Service URL in the XML configuration file..."

sed -i -e "s/MAPPINGANDRELOCALIZATIONFRONTEND_SERVICE_URL/$MAPPINGANDRELOCALIZATIONFRONTEND_SERVICE_URL/g" /.xpcf/SolARServiceTest_MappingAndRelocalizationFrontend_RelocViewer_conf.xml

echo "XML configuration file ready"

## Set application log level
## Log level expected: DEBUG, CRITICAL, ERROR, INFO, TRACE, WARNING
export SOLAR_LOG_LEVEL=INFO

export LD_LIBRARY_PATH=.:./modules/

## Start client
./SolARServiceTest_MappingAndRelocalizationFrontend_RelocViewer -f /.xpcf/SolARServiceTest_MappingAndRelocalizationFrontend_RelocViewer_conf.xml

