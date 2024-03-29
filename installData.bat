@echo off
:: Download bag of words vocabulary
echo Download bag of word dictionnaries
curl https://github.com/SolarFramework/SolARModuleFBOW/releases/download/fbowVocabulary/fbow_voc.zip -L -o fbow_voc.zip
echo Unzip bag of word dictionnaries
powershell Expand-Archive fbow_voc.zip -DestinationPath .\data\fbow_voc -F
del fbow_voc.zip
curl https://repository.solarframework.org/generic/FbowVoc/popsift_uint8.fbow -L -o ./data/fbow_voc/popsift_uint8.fbow

curl https://repository.solarframework.org/generic/FbowVoc/popsift_uint8_indoor.fbow -L -o data/fbow_voc/popsift_uint8_indoor.fbow

:: Download TUM camera calibration
echo Download TUM camera calibration
curl https://repository.solarframework.org/generic/captures/singleRGB/TUM/tum_camera_calibration.json -L -o data/tum_camera_calibration.json

:: Download TUM video for testing relocalization
echo Download TUM video for testing relocalization
curl https://repository.solarframework.org/generic/captures/singleRGB/TUM/rgbd_dataset_freiburg3_long_office_household_relocalization.avi -L -o data/rgbd_dataset_freiburg3_long_office_household_relocalization.avi

:: Download AR device captures for testing relocalization service
echo Download and install AR device captures
curl https://repository.solarframework.org/generic/captures/hololens/bcomLab/loopDesktopA.zip -L -o loopDesktopA.zip
powershell Expand-Archive loopDesktopA.zip -DestinationPath .\data\data_hololens -F
del loopDesktopA.zip

curl https://repository.solarframework.org/generic/captures/hololens/bcomLab/gtposeFidDesktopA.zip -L -o gtposeFidDesktopA.zip
powershell Expand-Archive gtposeFidDesktopA.zip -DestinationPath .\data\data_hololens -F
del gtposeFidDesktopA.zip

curl https://repository.solarframework.org/generic/captures/hololens/hololens_calibration.json -L -o .\data\data_hololens\hololens_calibration.json

:: Download TUM map for testing relocalization
echo Download TUM map for testing relocalization
curl https://repository.solarframework.org/generic/maps/TUM/freiburg3_long_office_household/map_win_0_10_0.zip -L -o map.zip
echo Unzip Map
powershell Expand-Archive map.zip -DestinationPath .\data -F
del map.zip

:: Download AR device captures
echo Download and install AR device captures
curl https://repository.solarframework.org/generic/captures/hololens/bcomLab/loopDesktopA.zip -L -o loopDesktopA.zip
powershell Expand-Archive loopDesktopA.zip -DestinationPath .\data\data_hololens -F
del loopDesktopA.zip

curl https://repository.solarframework.org/generic/captures/hololens/bcomLab/loopDesktopB.zip -L -o loopDesktopB.zip
powershell Expand-Archive loopDesktopB.zip -DestinationPath .\data\data_hololens -F
del loopDesktopB.zip

curl https://repository.solarframework.org/generic/captures/hololens/hololens_calibration.json -L -o .\data\data_hololens\hololens_calibration.json
