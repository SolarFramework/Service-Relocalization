/**
 * @copyright Copyright (c) 2021 B-com http://www.b-com.com/
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <iostream>

#include <cxxopts.hpp>

#include <xpcf/xpcf.h>
#include <xpcf/api/IComponentManager.h>
#include <xpcf/core/helpers.h>
#include <boost/log/core.hpp>
#include <signal.h>

#include "core/Log.h"
#include "api/pipeline/IAsyncRelocalizationPipeline.h"
#include "api/input/devices/IARDevice.h"
#include "api/display/IImageViewer.h"
#include "api/display/I3DOverlay.h"
#include "api/display/I3DPointsViewer.h"

using namespace std;
using namespace SolAR;
using namespace SolAR::api;
using namespace SolAR::datastructure;
namespace xpcf=org::bcom::xpcf;

// index of using cameras
// 1 camera for mono mode
// 2 cameras for stereo mode
const std::vector<int> INDEX_USE_CAMERA{1, 2};

// Global relocalization and mapping front end Service instance
SRef<pipeline::IAsyncRelocalizationPipeline> gRelocalizationAndMappingFrontendService = 0;

// print help options
void print_help(const cxxopts::Options& options)
{
    cout << options.help({""}) << '\n';
}

// print error message
void print_error(const string& msg)
{
    cerr << msg << '\n';
}

// Function called when interruption signal is triggered
static void SigInt(int signo) {

    LOG_INFO("\n\n===> Program interruption\n");

    LOG_INFO("Stop mapping and relocalization front end service");

    if (gRelocalizationAndMappingFrontendService != 0)
        gRelocalizationAndMappingFrontendService->stop();

    LOG_INFO("End of test");

    exit(0);
}

int main(int argc, char* argv[])
{
    #if NDEBUG
        boost::log::core::get()->set_logging_enabled(false);
    #endif

    LOG_ADD_LOG_TO_CONSOLE();
//    LOG_SET_DEBUG_LEVEL();


    bool relocOnly = false; // Indicate if only relocalization has to be done

    // Signal interruption function (Ctrl + C)
    signal(SIGINT, SigInt);

    cxxopts::Options option_list("SolARServiceTest_MappingAndRelocalizationFrontend",
                                 "SolARServiceTest_MappingAndRelocalizationFrontend - The commandline interface to the xpcf grpc client test application.\n");
    option_list.add_options()
            ("h,help", "display this help and exit")
            ("v,version", "display version information and exit")
            ("f,file", "xpcf grpc client configuration file",
             cxxopts::value<string>())
            ("reloc-only", "do only relocalization (no mapping)");

    auto options = option_list.parse(argc, argv);
    if (options.count("help")) {
        print_help(option_list);
        return 0;
    }
    else if (options.count("version"))
    {
        cout << "SolARServiceTest_MappingAndRelocalizationFrontend version " << MYVERSION << std::endl << std::endl;
        return 0;
    }
    else {
        if (!options.count("file") || options["file"].as<string>().empty()) {
            print_error("missing configuration file argument");
            return 1;
        }

        if (options.count("reloc-only")) {
            LOG_INFO("Relocalization only option specified");
            relocOnly = true;
        }
    }

    try {
        LOG_INFO("Get component manager instance");
        SRef<xpcf::IComponentManager> componentMgr = xpcf::getComponentManagerInstance();

        // Components used by test app
        SRef<input::devices::IARDevice> arDevice = 0;
        SRef<display::IImageViewer> imageViewer = 0;

        string file = options["file"].as<string>();
        LOG_INFO("Load Client Remote Service configuration file: {}", file);

        if (componentMgr->load(file.c_str()) == org::bcom::xpcf::_SUCCESS)
        {
            // Create Service component
            gRelocalizationAndMappingFrontendService = componentMgr->resolve<pipeline::IAsyncRelocalizationPipeline>();

            LOG_INFO("Mapping and relocalization front end service component created");
        }
        else {
            LOG_INFO("Failed to load Client Remote Service configuration file: {}", file);
            return -1;
        }

        LOG_INFO("Initialize the service");

        if (relocOnly) {
            LOG_INFO("Set \'Relocalization only\' mode");

            if (gRelocalizationAndMappingFrontendService->init(api::pipeline::RELOCALIZATION_ONLY)
                    != FrameworkReturnCode::_SUCCESS) {
                LOG_ERROR("Error while initializing the mode for mapping and relocalization front end service");
                return -1;
            }
        }
        else if (gRelocalizationAndMappingFrontendService->init() != FrameworkReturnCode::_SUCCESS) {
            LOG_ERROR("Error while initializing the mapping and relocalization front end service");
            return -1;
        }

        arDevice = componentMgr->resolve<input::devices::IARDevice>();
        LOG_INFO("Producer client: AR device component created");

        imageViewer = componentMgr->resolve<SolAR::api::display::IImageViewer>();
        auto overlay3D = componentMgr->resolve<display::I3DOverlay>();
        LOG_INFO("Remote producer client: AR device component created");

        if (arDevice->start() == FrameworkReturnCode::_SUCCESS) {

            // Load camera intrinsics parameters
            CameraRigParameters camRigParams = arDevice->getCameraParameters();
            CameraParameters camParams = camRigParams.cameraParams[INDEX_USE_CAMERA[0]];

            LOG_INFO("Set camera paremeters for the service");

            if (gRelocalizationAndMappingFrontendService->setCameraParameters(camParams) != FrameworkReturnCode::_SUCCESS) {
                LOG_ERROR("Error while setting camera parameters for the mapping and relocalization front end service");
                return -1;
            }

            // for stereo mode
            if (INDEX_USE_CAMERA.size() == 2) {
                RectificationParameters rectParams1 = camRigParams.rectificationParams[std::make_pair(INDEX_USE_CAMERA[0], INDEX_USE_CAMERA[1])].first;
                RectificationParameters rectParams2 = camRigParams.rectificationParams[std::make_pair(INDEX_USE_CAMERA[0], INDEX_USE_CAMERA[1])].second;
                if (gRelocalizationAndMappingFrontendService->setRectificationParameters(rectParams1, rectParams2) != FrameworkReturnCode::_SUCCESS) {
                    LOG_ERROR("Error while setting rectification parameters for the mapping and relocalization front end service");
                    return -1;
                }
            }

            if (!relocOnly) {
                LOG_INFO("Reset the global map stored in the Map Update service");
                if (gRelocalizationAndMappingFrontendService->resetMap() == FrameworkReturnCode::_SUCCESS) {
                    LOG_INFO("Global map reset!");
                }
            }

            LOG_INFO("Start the service");

            if (gRelocalizationAndMappingFrontendService->start() != FrameworkReturnCode::_SUCCESS) {
                LOG_ERROR("Error while initializing the mapping and relocalization front end service");
                return -1;
            }

            LOG_INFO("Read images and poses from hololens files");
            LOG_INFO("\n\n***** Control+C to stop *****\n");          

            // Wait for interruption or and of images
            while (true) {
                std::vector<SRef<Image>> images;
                std::vector<Transform3Df> poses;
                std::chrono::system_clock::time_point timestamp;

                // Read next image and pose
                if (arDevice->getData(images, poses, timestamp) == FrameworkReturnCode::_SUCCESS) {
                    std::vector<SRef<Image>> imagesToProcess;
                    std::vector<Transform3Df> posesToProcess;
                    for (const auto & i : INDEX_USE_CAMERA){
                        images[i]->setImageEncoding(Image::ENCODING_JPEG);
                        images[i]->setImageEncodingQuality(80);
                        imagesToProcess.push_back(images[i]);
                        posesToProcess.push_back(poses[i]);
                    }

                    api::pipeline::TransformStatus transform3DStatus;
                    Transform3Df transform3D;
                    float_t confidence;
                    api::pipeline::MappingStatus mappingStatus;

                    LOG_INFO("Send image and pose to service");                    

                    // Send data to mapping and relocalization front end service
                    gRelocalizationAndMappingFrontendService->relocalizeProcessRequest(
                                imagesToProcess, posesToProcess, timestamp, transform3DStatus, transform3D, confidence, mappingStatus);

                    if (transform3DStatus == api::pipeline::NEW_3DTRANSFORM) {
                        LOG_DEBUG("New 3D transformation = {}", transform3D.matrix());
                        // draw cube
                        if (!relocOnly)
                            overlay3D->draw(transform3D * posesToProcess[0], camParams, imagesToProcess[0]);
                    }
                    else if (transform3DStatus == api::pipeline::PREVIOUS_3DTRANSFORM) {
                        LOG_DEBUG("Previous 3D transformation = {}", transform3D.matrix());

                        // draw cube
                        if (!relocOnly)
                            overlay3D->draw(transform3D * posesToProcess[0], camParams, imagesToProcess[0]);
                    }
                    else if (transform3DStatus == api::pipeline::NO_3DTRANSFORM) {
                        LOG_DEBUG("No 3D transformation");
                    }

                    if (!relocOnly) {
                        switch (mappingStatus) {
                            case api::pipeline::BOOTSTRAP:
                                LOG_DEBUG("Mapping status: Bootstrap");
                                break;
                            case api::pipeline::MAPPING:
                                LOG_DEBUG("Mapping status: Mapping");
                                break;
                            case api::pipeline::TRACKING_LOST:
                                LOG_DEBUG("Mapping status: Tracking Lost");
                                break;
                            case api::pipeline::LOOP_CLOSURE:
                                LOG_DEBUG("Mapping status: Loop Closure");
                                break;
                            default:
                                LOG_DEBUG("Mapping status: unknown");
                                break;
                        }
                    }

                    // Display image sent
                    imageViewer->display(imagesToProcess[0]);
                }
                else {
                    LOG_INFO("No more images to send");

                    SRef<PointCloud> pointCloud;

                    // Get global point cloud
                    if (gRelocalizationAndMappingFrontendService->getPointCloudRequest(pointCloud) == FrameworkReturnCode::_SUCCESS) {
                        std::vector<SRef<CloudPoint>> globalPointCloud;
                        pointCloud->getAllPoints(globalPointCloud);

                        auto gViewer3D = componentMgr->resolve<display::I3DPointsViewer>();

                        LOG_INFO("==> Display current global point cloud: press ESC on the display window to end test");

                        while (true)
                        {
                            if (gViewer3D->display(globalPointCloud, {}, {}, {}, {}, {}) == FrameworkReturnCode::_STOP)
                                break;
                        }
                    }

                    LOG_INFO("Stop relocalization and mapping front end service");

                    if (gRelocalizationAndMappingFrontendService != 0)
                        gRelocalizationAndMappingFrontendService->stop();

                    LOG_INFO("End of test");

                    exit(0);

                }
            }
        }
        else {
            LOG_INFO("Cannot start AR device loader");
            return -1;
        }
    }
    catch (xpcf::Exception & e) {
        LOG_INFO("The following exception has been caught: {}", e.what());
        return -1;
    }

    return 0;
}
