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

## Copy SolARServiceRelocalizationClient app files
RUN mkdir SolARServiceRelocalizationClient

## Data files (maps, hololens configuration)
RUN mkdir SolARServiceRelocalizationClient/data
RUN mkdir SolARServiceRelocalizationClient/data/data_hololens
ADD data/data_hololens/* /SolARServiceRelocalizationClient/data/data_hololens/
RUN mkdir SolARServiceRelocalizationClient/data/data_hololens/loop_desktop_A
ADD data/data_hololens/loop_desktop_A/* /SolARServiceRelocalizationClient/data/data_hololens/loop_desktop_A/
RUN mkdir SolARServiceRelocalizationClient/data/data_hololens/loop_desktop_A/000
ADD data/data_hololens/loop_desktop_A/000/* /SolARServiceRelocalizationClient/data/data_hololens/loop_desktop_A/000/
RUN mkdir SolARServiceRelocalizationClient/data/data_hololens/loop_desktop_A/001
ADD data/data_hololens/loop_desktop_A/001/* /SolARServiceRelocalizationClient/data/data_hololens/loop_desktop_A/001/

## Libraries and modules
RUN mkdir SolARServiceRelocalizationClient/modules
ADD modules/* /SolARServiceRelocalizationClient/modules/

## Project files
ADD SolARServiceTest_Relocalization /SolARServiceRelocalizationClient/
RUN chmod +x /SolARServiceRelocalizationClient/SolARServiceTest_Relocalization
RUN mkdir .xpcf
ADD *.xml /.xpcf/
ADD docker/start_client.sh .
RUN chmod +x start_client.sh

## Set application log level
## Log level expected: DEBUG, CRITICAL, ERROR, INFO, TRACE, WARNING
ENV SOLAR_LOG_LEVEL=INFO

## Run Server
CMD [ "./start_client.sh"  ]
