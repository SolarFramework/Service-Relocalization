FROM ubuntu:18.04
MAINTAINER Christophe Cutullic christophe.cutullic@b-com.com

## Configure Ubuntu environment
RUN apt-get update -y
RUN apt-get install -y libgtk2.0-dev
RUN apt-get install -y libva-dev
RUN apt-get install -y libvdpau-dev

# Manage graphical display
RUN apt-get install -y xterm
RUN useradd -ms /bin/bash xterm

## Copy SolARServiceMappingAndRelocFrontendMultiViewer app files
RUN mkdir SolARServiceMappingAndRelocFrontendMultiViewer

## Libraries and modules
RUN mkdir SolARServiceMappingAndRelocFrontendMultiViewer/modules
ADD modules/* /SolARServiceMappingAndRelocFrontendMultiViewer/modules/

## Project files
ADD SolARServiceTest_MappingAndRelocFrontend_Multi_Viewer /SolARServiceMappingAndRelocFrontendMultiViewer/
RUN chmod +x /SolARServiceMappingAndRelocFrontendMultiViewer/SolARServiceTest_MappingAndRelocFrontend_Multi_Viewer
RUN mkdir .xpcf
ADD *.xml /.xpcf/
ADD docker/start_client.sh .
RUN chmod +x start_client.sh

## Set application log level
## Log level expected: DEBUG, CRITICAL, ERROR, INFO, TRACE, WARNING
ENV SOLAR_LOG_LEVEL=INFO

## Run Server
CMD [ "./start_client.sh"  ]
