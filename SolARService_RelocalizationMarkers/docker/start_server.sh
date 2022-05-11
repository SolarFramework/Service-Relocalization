#!/bin/sh

export LD_LIBRARY_PATH=/SolARServiceRelocalizationMarkers:/SolARServiceRelocalizationMarkers/modules/

## Start client
cd /SolARServiceRelocalizationMarkers
./SolARService_RelocalizationMarkers -m /.xpcf/SolARService_RelocalizationMarkers_modules.xml -p /.xpcf/SolARService_RelocalizationMarkers_properties.xml
