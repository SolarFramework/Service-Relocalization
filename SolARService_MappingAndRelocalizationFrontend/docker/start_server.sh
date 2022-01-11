#!/bin/sh

## Detect RELOCALIZATION_SERVICE_URL var and use its value
## to set the Relocalization service URL in XML configuration file

if [ -z "$RELOCALIZATION_SERVICE_URL" ]
then
    echo "Error: You must define RELOCALIZATION_SERVICE_URL env var with the Relocalization Service URL"
    exit 1 
else
    echo "RELOCALIZATION_SERVICE_URL defined: $RELOCALIZATION_SERVICE_URL"
fi

echo "Try to replace the Relocalization Service URL in the XML configuration file..."

sed -i -e "s/RELOCALIZATION_SERVICE_URL/$RELOCALIZATION_SERVICE_URL/g" /.xpcf/SolARService_MappingAndRelocalizationFrontend_properties.xml

## Detect MAPPING_SERVICE_URL var and use its value
## to set the Mapping service URL in XML configuration file

if [ -z "$MAPPING_SERVICE_URL" ]
then
    echo "Error: You must define MAPPING_SERVICE_URL env var with the Mapping Service URL"
    exit 1
else
    echo "MAPPING_SERVICE_URL defined: $MAPPING_SERVICE_URL"
fi

echo "Try to replace the Mapping Service URL in the XML configuration file..."

sed -i -e "s/MAPPING_SERVICE_URL/$MAPPING_SERVICE_URL/g" /.xpcf/SolARService_MappingAndRelocalizationFrontend_properties.xml

echo "XML configuration file ready"

export LD_LIBRARY_PATH=/SolARServiceMappingAndRelocalizationFrontend:/SolARServiceMappingAndRelocalizationFrontend/modules/

cd /SolARServiceMappingAndRelocalizationFrontend

## Start Envoy
envoy -c envoy_config.yaml --log-path ./envoy_output.log &

## Start proxy
./SolARService_MappingAndRelocalizationProxy -f /.xpcf/SolARService_MappingAndRelocalizationProxy_conf.xml >> proxy_output.log &

## Start front end
./SolARService_MappingAndRelocalizationFrontend -m /.xpcf/SolARService_MappingAndRelocalizationFrontend_modules.xml -p /.xpcf/SolARService_MappingAndRelocalizationFrontend_properties.xml

