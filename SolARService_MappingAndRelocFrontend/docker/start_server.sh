#!/bin/sh

## Detect SERVICE_MANAGER_URL var and use its value
## to set the Service Manager URL in XML configuration file

if [ -z "$SERVICE_MANAGER_URL" ]
then
    echo "Error: You must define SERVICE_MANAGER_URL env var with the Service Manager URL"
    exit 1
else
    ## Detect port in service URL
    if echo "$SERVICE_MANAGER_URL" | grep -q ":"
    then
        echo "Port is defined in Service Manager URL"
    else
        echo "No port set in Service Manager URL: add port 80 (default)"
        export SERVICE_MANAGER_URL="${SERVICE_MANAGER_URL}:80"
    fi

    echo "SERVICE_MANAGER_URL defined: $SERVICE_MANAGER_URL"
fi

echo "Try to replace the Service Manager URL in the XML configuration file..."

sed -i -e "s/SERVICE_MANAGER_URL/$SERVICE_MANAGER_URL/g" /.xpcf/SolARService_MappingAndRelocFrontend_properties.xml

echo "XML configuration file ready"

export LD_LIBRARY_PATH=/SolARServiceMappingAndRelocFrontend:/SolARServiceMappingAndRelocFrontend/modules/

cd /SolARServiceMappingAndRelocFrontend

## Start Envoy
if echo $DISPLAY_LOG | grep -q "ENVOY"
then
	echo "Display envoy logs"
	envoy -c envoy_config.yaml &
        ./SolARService_MappingAndRelocalizationProxy -f /.xpcf/SolARService_MappingAndRelocalizationProxy_conf.xml >/dev/null &
        ./SolARService_MappingAndRelocFrontend -m /.xpcf/SolARService_MappingAndRelocFrontend_modules.xml -p /.xpcf/SolARService_MappingAndRelocFrontend_properties.xml >/dev/null
        exit 0
fi

## Start proxy
if echo $DISPLAY_LOG | grep -q "PROXY"
then
	echo "Display proxy logs"
        envoy -c envoy_config.yaml >/dev/null 2>&1 &
        ./SolARService_MappingAndRelocalizationProxy -f /.xpcf/SolARService_MappingAndRelocalizationProxy_conf.xml &
        ./SolARService_MappingAndRelocFrontend -m /.xpcf/SolARService_MappingAndRelocFrontend_modules.xml -p /.xpcf/SolARService_MappingAndRelocFrontend_properties.xml >/dev/null
        exit 0
fi

## Start front end
if echo $DISPLAY_LOG | grep -q "FRONTEND"
then
	echo "Display frontend logs"
        envoy -c envoy_config.yaml >/dev/null 2>&1 &
        ./SolARService_MappingAndRelocalizationProxy -f /.xpcf/SolARService_MappingAndRelocalizationProxy_conf.xml >/dev/null &
        ./SolARService_MappingAndRelocFrontend -m /.xpcf/SolARService_MappingAndRelocFrontend_modules.xml -p /.xpcf/SolARService_MappingAndRelocFrontend_properties.xml
        exit 0
fi

echo "Display all logs"
envoy -c envoy_config.yaml &
./SolARService_MappingAndRelocalizationProxy -f /.xpcf/SolARService_MappingAndRelocalizationProxy_conf.xml &
./SolARService_MappingAndRelocFrontend -m /.xpcf/SolARService_MappingAndRelocFrontend_modules.xml -p /.xpcf/SolARService_MappingAndRelocFrontend_properties.xml

