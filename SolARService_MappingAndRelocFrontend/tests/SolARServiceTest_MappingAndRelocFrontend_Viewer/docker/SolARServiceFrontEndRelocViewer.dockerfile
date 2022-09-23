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

## Copy SolARServiceMappingAndRelocFrontendViewer app files
RUN mkdir SolARServiceMappingAndRelocFrontendViewer

## Libraries and modules
RUN mkdir SolARServiceMappingAndRelocFrontendViewer/modules
ADD modules/* /SolARServiceMappingAndRelocFrontendViewer/modules/

## Project files
ADD SolARServiceTest_MappingAndRelocFrontend_Viewer /SolARServiceMappingAndRelocFrontendViewer/
RUN chmod +x /SolARServiceMappingAndRelocFrontendViewer/SolARServiceTest_MappingAndRelocFrontend_Viewer
RUN mkdir .xpcf
ADD *.xml /.xpcf/
ADD docker/start_client.sh .
RUN chmod +x start_client.sh

## Set application log level
## Log level expected: DEBUG, CRITICAL, ERROR, INFO, TRACE, WARNING
ENV SOLAR_LOG_LEVEL=INFO

## Run Server
CMD [ "./start_client.sh"  ]
