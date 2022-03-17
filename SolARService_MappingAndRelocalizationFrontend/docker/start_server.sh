#!/bin/sh

## Detect RELOCALIZATION_SERVICE_URL var and use its value
## to set the Relocalization service URL in XML configuration file

if [ -z "$RELOCALIZATION_SERVICE_URL" ]
then
    echo "Error: You must define RELOCALIZATION_SERVICE_URL env var with the Relocalization Service URL"
    exit 1 
else
    ## Detect port in service URL
    if echo "$RELOCALIZATION_SERVICE_URL" | grep -q ":"
    then
        echo "Port is defined in Relocalization service URL"
    else
        echo "No port set in Relocalization service URL: add port 80 (default)"
        export RELOCALIZATION_SERVICE_URL="${RELOCALIZATION_SERVICE_URL}:80"
    fi

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
    ## Detect port in service URL
    if echo "$MAPPING_SERVICE_URL" | grep -q ":"
    then
        echo "Port is defined in Mapping service URL"
    else
        echo "No port set in Mapping service URL: add port 80 (default)"
        export MAPPING_SERVICE_URL="${MAPPING_SERVICE_URL}:80"
    fi

    echo "MAPPING_SERVICE_URL defined: $MAPPING_SERVICE_URL"
fi

echo "Try to replace the Mapping Service URL in the XML configuration file..."

sed -i -e "s/MAPPING_SERVICE_URL/$MAPPING_SERVICE_URL/g" /.xpcf/SolARService_MappingAndRelocalizationFrontend_properties.xml

echo "XML configuration file ready"

export LD_LIBRARY_PATH=/SolARServiceMappingAndRelocalizationFrontend:/SolARServiceMappingAndRelocalizationFrontend/modules/

cd /SolARServiceMappingAndRelocalizationFrontend

## Start Envoy
if echo $DISPLAY_LOG | grep -q "ENVOY"
then
	echo "Display envoy logs"
	envoy -c envoy_config.yaml &
else
	envoy -c envoy_config.yaml --log-path ./envoy_output.log >> envoy_display.log &
fi

## Start proxy
if echo $DISPLAY_LOG | grep -q "PROXY"
then
	echo "Display proxy logs"
	./SolARService_MappingAndRelocalizationProxy -f /.xpcf/SolARService_MappingAndRelocalizationProxy_conf.xml &
else
	./SolARService_MappingAndRelocalizationProxy -f /.xpcf/SolARService_MappingAndRelocalizationProxy_conf.xml >> proxy_display.log &
fi

## Start front end
if echo $DISPLAY_LOG | grep -q "FRONTEND"
then
	echo "Display frontend logs"
	./SolARService_MappingAndRelocalizationFrontend -m /.xpcf/SolARService_MappingAndRelocalizationFrontend_modules.xml -p /.xpcf/SolARService_MappingAndRelocalizationFrontend_properties.xml
else
	if [ "$DISPLAY_LOG" ]
	then
		./SolARService_MappingAndRelocalizationFrontend -m /.xpcf/SolARService_MappingAndRelocalizationFrontend_modules.xml -p /.xpcf/SolARService_MappingAndRelocalizationFrontend_properties.xml >> frontend_display.log
	else
		echo "Display frontend logs (by default)"
		./SolARService_MappingAndRelocalizationFrontend -m /.xpcf/SolARService_MappingAndRelocalizationFrontend_modules.xml -p /.xpcf/SolARService_MappingAndRelocalizationFrontend_properties.xml
	fi
fi

