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

sed -i -e "s/SERVICE_MANAGER_URL/$SERVICE_MANAGER_URL/g" /.xpcf/SolARService_RelocalizationMarkers_properties.xml

echo "XML configuration file ready"

export LD_LIBRARY_PATH=/SolARServiceRelocalizationMarkers:/SolARServiceRelocalizationMarkers/modules/

## Start client
cd /SolARServiceRelocalizationMarkers
./SolARService_RelocalizationMarkers -m /.xpcf/SolARService_RelocalizationMarkers_modules.xml -p /.xpcf/SolARService_RelocalizationMarkers_properties.xml
