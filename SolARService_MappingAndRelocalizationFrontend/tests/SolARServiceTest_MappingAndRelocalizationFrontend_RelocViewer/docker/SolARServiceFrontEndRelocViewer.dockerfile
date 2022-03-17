FROM ubuntu:18.04
MAINTAINER Christophe Cutullic christophe.cutullic@b-com.com

## Configure Ubuntu environment
RUN apt-get update -y
RUN apt-get install -y libgtk-3-0
RUN apt-get install -y libva-dev
RUN apt-get install -y libvdpau-dev

# Manage graphical display
RUN apt-get install -y xterm
RUN useradd -ms /bin/bash xterm

## Copy SolARServiceMappingAndRelocalizationFrontendRelocViewer app files
RUN mkdir SolARServiceMappingAndRelocalizationFrontendRelocViewer

## Libraries and modules
RUN mkdir SolARServiceMappingAndRelocalizationFrontendRelocViewer/modules
ADD modules/* /SolARServiceMappingAndRelocalizationFrontendRelocViewer/modules/

## Project files
ADD SolARServiceTest_MappingAndRelocalizationFrontend_RelocViewer /SolARServiceMappingAndRelocalizationFrontendRelocViewer/
RUN chmod +x /SolARServiceMappingAndRelocalizationFrontendRelocViewer/SolARServiceTest_MappingAndRelocalizationFrontend_RelocViewer
RUN mkdir .xpcf
ADD *.xml /.xpcf/
ADD docker/start_client.sh .
RUN chmod +x start_client.sh

## Set application log level
## Log level expected: DEBUG, CRITICAL, ERROR, INFO, TRACE, WARNING
ENV SOLAR_LOG_LEVEL=INFO

## Run Server
CMD [ "./start_client.sh"  ]
