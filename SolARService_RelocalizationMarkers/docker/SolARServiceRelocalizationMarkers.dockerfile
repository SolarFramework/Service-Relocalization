FROM ubuntu:18.04
MAINTAINER Christophe Cutullic christophe.cutullic@b-com.com

## Configure Ubuntu environment
RUN apt-get update -y
RUN apt-get install -y libgtk2.0-dev
RUN apt-get install -y libva-dev
RUN apt-get install -y libvdpau-dev

## Copy SolARServiceRelocalizationMarkers app files
RUN mkdir SolARServiceRelocalizationMarkers

## Data files (markers description)
RUN mkdir SolARServiceRelocalizationMarkers/data
RUN mkdir SolARServiceRelocalizationMarkers/data/marker
ADD data/marker/* /SolARServiceRelocalizationMarkers/data/marker/

## Libraries and modules
RUN mkdir SolARServiceRelocalizationMarkers/modules
ADD modules/* /SolARServiceRelocalizationMarkers/modules/
ADD modules_common/* /SolARServiceRelocalizationMarkers/modules/
ADD modules_no_cuda/* /SolARServiceRelocalizationMarkers/modules/

## Project files
ADD SolARService_RelocalizationMarkers /SolARServiceRelocalizationMarkers/
RUN chmod +x /SolARServiceRelocalizationMarkers/SolARService_RelocalizationMarkers
RUN mkdir .xpcf
ADD *.xml /.xpcf/
ADD docker/start_server.sh .
RUN chmod +x start_server.sh

## Set application gRPC server url
ENV XPCF_GRPC_SERVER_URL=0.0.0.0:8080
## Set service external URL
ENV SERVER_EXTERNAL_URL=172.17.0.1:50050
## Set application gRPC max receive message size
ENV XPCF_GRPC_MAX_RECV_MSG_SIZE=7000000
## Set application gRPC max send message size
ENV XPCF_GRPC_MAX_SEND_MSG_SIZE=20000

## Set application log level
## Log level expected: DEBUG, CRITICAL, ERROR, INFO, TRACE, WARNING
ENV SOLAR_LOG_LEVEL=INFO

## Run Server
CMD [ "./start_server.sh"  ]
