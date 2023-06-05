FROM ubuntu:18.04
MAINTAINER Christophe Cutullic christophe.cutullic@b-com.com

## Configure Ubuntu environment
RUN apt-get update -y
RUN apt-get install -y libgtk2.0-dev
RUN apt-get install -y libva-dev
RUN apt-get install -y libvdpau-dev

## Data files (fbow vocabulary)
RUN mkdir SolARServiceRelocalization
RUN mkdir SolARServiceRelocalization/data
RUN mkdir SolARServiceRelocalization/data/fbow_voc
ADD data/fbow_voc/akaze.fbow /SolARServiceRelocalization/data/fbow_voc/

## Persistent volume for configuration files
VOLUME .xpcf

## Libraries and modules
RUN mkdir SolARServiceRelocalization/modules
ADD modules/* /SolARServiceRelocalization/modules/
ADD modules_common/* /SolARServiceRelocalization/modules/
ADD modules_no_cuda/* /SolARServiceRelocalization/modules/

## Project files
ADD SolARService_Relocalization /SolARServiceRelocalization/
RUN chmod +x /SolARServiceRelocalization/SolARService_Relocalization
RUN mkdir .xpcf
ADD *.xml /.xpcf/
ADD docker/start_server.sh .
RUN chmod +x start_server.sh

## Set application gRPC server url
ENV XPCF_GRPC_SERVER_URL=0.0.0.0:8080
## Set service external URL
ENV SERVER_EXTERNAL_URL=172.17.0.1:50052
## Set application gRPC max receive message size
ENV XPCF_GRPC_MAX_RECV_MSG_SIZE=-1
## Set application gRPC max send message size
ENV XPCF_GRPC_MAX_SEND_MSG_SIZE=-1

## Set url to Map Update Service
ENV MAPUPDATE_SERVICE_URL=map-update-service

## Set application log level
## Log level expected: DEBUG, CRITICAL, ERROR, INFO, TRACE, WARNING
ENV SOLAR_LOG_LEVEL=INFO

## Run Server
CMD [ "./start_server.sh"  ]
