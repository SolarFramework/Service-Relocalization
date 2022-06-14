FROM ubuntu:18.04
MAINTAINER Christophe Cutullic christophe.cutullic@b-com.com

## Configure Ubuntu environment
RUN apt-get update -y
RUN apt-get install -y libgtk2.0-dev
RUN apt-get install -y libva-dev
RUN apt-get install -y libvdpau-dev

## Envoy installation
RUN apt-get install apt-transport-https gnupg2 curl lsb-release -y
RUN curl -sL 'https://deb.dl.getenvoy.io/public/gpg.8115BA8E629CC074.key' | gpg --dearmor -o /usr/share/keyrings/getenvoy-keyring.gpg
## Verify the keyring - this should yield "OK"
RUN echo a077cb587a1b622e03aa4bf2f3689de14658a9497a9af2c427bba5f4cc3c4723 /usr/share/keyrings/getenvoy-keyring.gpg | sha256sum --check
RUN echo "deb [arch=amd64 signed-by=/usr/share/keyrings/getenvoy-keyring.gpg] https://deb.dl.getenvoy.io/public/deb/ubuntu $(lsb_release -cs) main" | tee /etc/apt/sources.list.d/getenvoy.list
RUN apt-get update
RUN apt-get install -y getenvoy-envoy

## Front End part

## Copy SolARServiceMappingAndRelocalizationFrontend app files
RUN mkdir SolARServiceMappingAndRelocalizationFrontend

## Libraries and modules
RUN mkdir SolARServiceMappingAndRelocalizationFrontend/modules
ADD modules/* /SolARServiceMappingAndRelocalizationFrontend/modules/
ADD modules_common/* /SolARServiceMappingAndRelocalizationFrontend/modules/
ADD modules_no_cuda/* /SolARServiceMappingAndRelocalizationFrontend/modules/

## Project files
ADD SolARService_MappingAndRelocalizationFrontend /SolARServiceMappingAndRelocalizationFrontend/
RUN chmod +x /SolARServiceMappingAndRelocalizationFrontend/SolARService_MappingAndRelocalizationFrontend
RUN mkdir .xpcf
ADD *.xml /.xpcf/
ADD docker/start_server.sh .
RUN chmod +x start_server.sh

## Proxy part
ADD SolARService_MappingAndRelocalizationProxy /SolARServiceMappingAndRelocalizationFrontend/
RUN chmod +x /SolARServiceMappingAndRelocalizationFrontend/SolARService_MappingAndRelocalizationProxy
## Envoy configuration files
ADD envoy_config.yaml /SolARServiceMappingAndRelocalizationFrontend/

## Set application gRPC server url
ENV XPCF_GRPC_SERVER_URL=0.0.0.0:8080
## Set application gRPC max receive message size
ENV XPCF_GRPC_MAX_RECV_MSG_SIZE=7000000
## Set application gRPC max send message size
ENV XPCF_GRPC_MAX_SEND_MSG_SIZE=20000

## Set url to Map Update Service
ENV MAPUPDATE_SERVICE_URL=mapupdate-service
## Set url to Relocalization Service
ENV RELOCALIZATION_SERVICE_URL=relocalization-service
## Set url to Relocalization Markers Service
ENV RELOCALIZATION_MARKERS_SERVICE_URL=relocalization-markers-service
## Set url to Mapping Service
ENV MAPPING_SERVICE_URL=mapping-service

## Set application log level
## Log level expected: DEBUG, CRITICAL, ERROR, INFO, TRACE, WARNING
ENV SOLAR_LOG_LEVEL=INFO

## Run Server
CMD [ "./start_server.sh"  ]
