#!/bin/sh

## Detect MAPUPDATE_SERVICE_URL var and use its value 
## to set the Map Update service URL in XML configuration file

if [ -z "$MAPUPDATE_SERVICE_URL" ]
then
    echo "Error: You must define MAPUPDATE_SERVICE_URL env var with the MapUpdate Service URL"
    exit 1 
else
    ## Detect port in service URL
    if echo "$MAPUPDATE_SERVICE_URL" | grep -q ":"
    then
        echo "Port is defined in MapUpdate service URL"
    else
        echo "No port set in MapUpdate service URL: add port 80 (default)"
        export MAPUPDATE_SERVICE_URL="${MAPUPDATE_SERVICE_URL}:80"
    fi

    echo "MAPUPDATE_SERVICE_URL defined: $MAPUPDATE_SERVICE_URL"
fi

echo "Try to replace the MapUpdate Service URL in the XML configuration file..."

sed -i -e "s/MAPUPDATE_SERVICE_URL/$MAPUPDATE_SERVICE_URL/g" /.xpcf/SolARService_Relocalization_properties.xml

echo "XML configuration file ready"

export LD_LIBRARY_PATH=/SolARServiceRelocalization:/SolARServiceRelocalization/modules/

## Start client
cd /SolARServiceRelocalization
./SolARService_Relocalization -m /.xpcf/SolARService_Relocalization_modules.xml -p /.xpcf/SolARService_Relocalization_properties.xml
