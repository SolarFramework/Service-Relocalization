#!/bin/sh

## Detect MAPUPDATE_SERVICE_URL var and use its value
## to set the Map Update service URL in XML configuration file

if [ -z "$MAPUPDATE_SERVICE_URL" ]
then
    echo "Error: You must define MAPUPDATE_SERVICE_URL env var with the Map Update Service URL"
    exit 1
else
    ## Detect port in service URL
    if echo "$MAPUPDATE_SERVICE_URL" | grep -q ":"
    then
        echo "Port is defined in Map Update service URL"
    else
        echo "No port set in Map Update service URL: add port 80 (default)"
        export MAPUPDATE_SERVICE_URL="${MAPUPDATE_SERVICE_URL}:80"
    fi

    echo "MAPUPDATE_SERVICE_URL defined: $MAPUPDATE_SERVICE_URL"
fi

echo "Try to replace the Map Update Service URL in the XML configuration file..."

sed -i -e "s/MAPUPDATE_SERVICE_URL/$MAPUPDATE_SERVICE_URL/g" /.xpcf/SolARService_MappingAndRelocFrontend_properties.xml

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

sed -i -e "s/RELOCALIZATION_SERVICE_URL/$RELOCALIZATION_SERVICE_URL/g" /.xpcf/SolARService_MappingAndRelocFrontend_properties.xml

## Detect RELOCALIZATION_MARKERS_SERVICE_URL var and use its value
## to set the Relocalization Markers service URL in XML configuration file

if [ -z "$RELOCALIZATION_MARKERS_SERVICE_URL" ]
then
    echo "Error: You must define RELOCALIZATION_MARKERS_SERVICE_URL env var with the Relocalization Markers Service URL"
    exit 1
else
    ## Detect port in service URL
    if echo "$RELOCALIZATION_MARKERS_SERVICE_URL" | grep -q ":"
    then
        echo "Port is defined in Relocalization Markers service URL"
    else
        echo "No port set in Relocalization Markers service URL: add port 80 (default)"
        export RELOCALIZATION_MARKERS_SERVICE_URL="${RELOCALIZATION_MARKERS_SERVICE_URL}:80"
    fi

    echo "RELOCALIZATION_MARKERS_SERVICE_URL defined: $RELOCALIZATION_MARKERS_SERVICE_URL"
fi

echo "Try to replace the Relocalization Markers Service URL in the XML configuration file..."

sed -i -e "s/RELOCALIZATION_MARKERS_SERVICE_URL/$RELOCALIZATION_MARKERS_SERVICE_URL/g" /.xpcf/SolARService_MappingAndRelocFrontend_properties.xml

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

sed -i -e "s/MAPPING_SERVICE_URL/$MAPPING_SERVICE_URL/g" /.xpcf/SolARService_MappingAndRelocFrontend_properties.xml

## Detect MAPPING_STEREO_SERVICE_URL var and use its value
## to set the Mapping Stereo service URL in XML configuration file

if [ -z "$MAPPING_STEREO_SERVICE_URL" ]
then
    echo "Error: You must define MAPPING_STEREO_SERVICE_URL env var with the Mapping Stereo Service URL"
    exit 1
else
    ## Detect port in service URL
    if echo "$MAPPING_STEREO_SERVICE_URL" | grep -q ":"
    then
        echo "Port is defined in Mapping Stereo service URL"
    else
        echo "No port set in Mapping Stereo service URL: add port 80 (default)"
        export MAPPING_STEREO_SERVICE_URL="${MAPPING_STEREO_SERVICE_URL}:80"
    fi

    echo "MAPPING_STEREO_SERVICE_URL defined: $MAPPING_STEREO_SERVICE_URL"
fi

echo "Try to replace the Mapping Stereo Service URL in the XML configuration file..."

sed -i -e "s/MAPPING_STEREO_SERVICE_URL/$MAPPING_STEREO_SERVICE_URL/g" /.xpcf/SolARService_MappingAndRelocFrontend_properties.xml

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
