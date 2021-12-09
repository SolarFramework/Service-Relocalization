FROM ubuntu:18.04
MAINTAINER Christophe Cutullic christophe.cutullic@b-com.com

## Configure Ubuntu environment
RUN apt-get update -y
RUN apt-get install -y libgtk-3-0
RUN apt-get install -y libva-dev
RUN apt-get install -y libvdpau-dev

## Copy SolARServiceMappingAndRelocalizationFrontend app files
RUN mkdir SolARServiceMappingAndRelocalizationFrontend

## Data files (marker definition)
RUN mkdir SolARServiceMappingAndRelocalizationFrontend/data
RUN mkdir SolARServiceMappingAndRelocalizationFrontend/data/data_hololens
ADD data/data_hololens/* /SolARServiceMappingAndRelocalizationFrontend/data/data_hololens/

## Libraries and modules
RUN mkdir SolARServiceMappingAndRelocalizationFrontend/modules
ADD modules/* /SolARServiceMappingAndRelocalizationFrontend/modules/

## Project files
ADD SolARService_MappingAndRelocalizationFrontend /SolARServiceMappingAndRelocalizationFrontend/
RUN chmod +x /SolARServiceMappingAndRelocalizationFrontend/SolARService_MappingAndRelocalizationFrontend
RUN mkdir .xpcf
ADD *.xml /.xpcf/
ADD docker/start_server.sh .
RUN chmod +x start_server.sh

## Set application gRPC server url
ENV XPCF_GRPC_SERVER_URL=0.0.0.0:8080
## Set application gRPC max receive message size
ENV XPCF_GRPC_MAX_RECV_MSG_SIZE=7000000
## Set application gRPC max send message size
ENV XPCF_GRPC_MAX_SEND_MSG_SIZE=20000

## Set url to Relocalization Service
ENV RELOCALIZATION_SERVICE_URL=relocalization-service.artwin.svc.cluster.local:80
## Set url to Mapping Service
ENV MAPPING_SERVICE_URL=mapping-service.artwin.svc.cluster.local:80

## Set application log level
## Log level expected: DEBUG, CRITICAL, ERROR, INFO, TRACE, WARNING
ENV SOLAR_LOG_LEVEL=INFO

## Run Server
CMD [ "./start_server.sh"  ]
