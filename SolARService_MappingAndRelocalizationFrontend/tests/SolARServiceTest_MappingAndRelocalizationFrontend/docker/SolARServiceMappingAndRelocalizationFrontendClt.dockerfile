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

## Copy SolARServiceMappingAndRelocalizationFrontendClient app files
RUN mkdir SolARServiceMappingAndRelocalizationFrontendClient

## Data files (maps, hololens configuration)
RUN mkdir SolARServiceMappingAndRelocalizationFrontendClient/data
RUN mkdir SolARServiceMappingAndRelocalizationFrontendClient/data/data_hololens
ADD data/data_hololens/* /SolARServiceMappingAndRelocalizationFrontendClient/data/data_hololens/
RUN mkdir SolARServiceMappingAndRelocalizationFrontendClient/data/data_hololens/HoloLensRecording__2021_08_02__11_23_59_MUCLab_1
ADD data/data_hololens/HoloLensRecording__2021_08_02__11_23_59_MUCLab_1/* /SolARServiceMappingAndRelocalizationFrontendClient/data/data_hololens/HoloLensRecording__2021_08_02__11_23_59_MUCLab_1/
RUN mkdir SolARServiceMappingAndRelocalizationFrontendClient/data/data_hololens/HoloLensRecording__2021_08_02__11_23_59_MUCLab_1/000
ADD data/data_hololens/HoloLensRecording__2021_08_02__11_23_59_MUCLab_1/000/* /SolARServiceMappingAndRelocalizationFrontendClient/data/data_hololens/HoloLensRecording__2021_08_02__11_23_59_MUCLab_1/000/
RUN mkdir SolARServiceMappingAndRelocalizationFrontendClient/data/data_hololens/HoloLensRecording__2021_08_02__11_23_59_MUCLab_1/001
ADD data/data_hololens/HoloLensRecording__2021_08_02__11_23_59_MUCLab_1/001/* /SolARServiceMappingAndRelocalizationFrontendClient/data/data_hololens/HoloLensRecording__2021_08_02__11_23_59_MUCLab_1/001/
RUN mkdir SolARServiceMappingAndRelocalizationFrontendClient/data/data_hololens/HoloLensRecording__2021_08_02__11_23_59_MUCLab_1/002
ADD data/data_hololens/HoloLensRecording__2021_08_02__11_23_59_MUCLab_1/002/* /SolARServiceMappingAndRelocalizationFrontendClient/data/data_hololens/HoloLensRecording__2021_08_02__11_23_59_MUCLab_1/002/

## Libraries and modules
RUN mkdir SolARServiceMappingAndRelocalizationFrontendClient/modules
ADD modules/* /SolARServiceMappingAndRelocalizationFrontendClient/modules/

## Project files
ADD SolARServiceTest_MappingAndRelocalizationFrontend /SolARServiceMappingAndRelocalizationFrontendClient/
RUN chmod +x /SolARServiceMappingAndRelocalizationFrontendClient/SolARServiceTest_MappingAndRelocalizationFrontend
RUN mkdir .xpcf
ADD *.xml /.xpcf/
ADD docker/start_client.sh .
RUN chmod +x start_client.sh

## Set application log level
## Log level expected: DEBUG, CRITICAL, ERROR, INFO, TRACE, WARNING
ENV SOLAR_LOG_LEVEL=INFO

## Run Server
CMD [ "./start_client.sh"  ]
