/**
 * @copyright Copyright (c) 2022 B-com http://www.b-com.com/
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

#include "core/Log.h"
#include "api/pipeline/IMapUpdatePipeline.h"
#include "api/pipeline/IAsyncRelocalizationPipeline.h"
#include "api/storage/IMapManager.h"
#include "api/display/I3DPointsViewer.h"

using namespace std;
using namespace SolAR;
using namespace SolAR::api;
using namespace SolAR::datastructure;
namespace xpcf=org::bcom::xpcf;

// Delay between two requests to front end in ms
#define FRONT_END_REQUEST_DELAY 500

// print help options
void print_help(const cxxopts::Options& options)
{
    cout << options.help({""}) << std::endl;
}

// print error message
void print_error(const string& msg)
{
    cerr << msg << std::endl;
}

int main(int argc, char* argv[])
{
    #if NDEBUG
        boost::log::core::get()->set_logging_enabled(false);
    #endif

    LOG_ADD_LOG_TO_CONSOLE();
    LOG_SET_DEBUG_LEVEL();

    cxxopts::Options option_list("SolARServiceTest_MappingAndRelocalizationFrontend_Multi_Viewer",
                                 "SolARServiceTest_MappingAndRelocalizationFrontend_Multi_Viewer - The commandline interface to the xpcf grpc client test application.\n");
    option_list.add_options()
            ("h,help", "display this help and exit")
            ("v,version", "display version information and exit")
            ("f,file", "xpcf grpc client configuration file", cxxopts::value<string>());

    auto options = option_list.parse(argc, argv);
    if (options.count("help")) {
        print_help(option_list);
        return 0;
    }
    else if (options.count("version"))
    {
        std::cout << "SolARServiceTest_MappingAndRelocalizationFrontend_Multi_Viewer version " << MYVERSION << std::endl << std::endl;
        return 0;
    }
    else {
        if (!options.count("file") || options["file"].as<string>().empty()) {
            print_error("missing configuration file argument");
            return -1;
        }
    }

    try {

        // Check if log level is defined in environment variable SOLAR_LOG_LEVEL
        char * log_level = getenv("SOLAR_LOG_LEVEL");
        std::string str_log_level = "INFO(default)";

        if (log_level != nullptr) {
            str_log_level = std::string(log_level);

            if (str_log_level == "DEBUG"){
                LOG_SET_DEBUG_LEVEL();
            }
            else if (str_log_level == "CRITICAL"){
                LOG_SET_CRITICAL_LEVEL();
            }
            else if (str_log_level == "ERROR"){
                LOG_SET_ERROR_LEVEL();
            }
            else if (str_log_level == "INFO"){
                LOG_SET_INFO_LEVEL();
            }
            else if (str_log_level == "TRACE"){
                LOG_SET_TRACE_LEVEL();
            }
            else if (str_log_level == "WARNING"){
                LOG_SET_WARNING_LEVEL();
            }
            else {
                LOG_ERROR ("'SOLAR_LOG_LEVEL' environment variable: invalid value");
                LOG_ERROR ("Expected values are: DEBUG, CRITICAL, ERROR, INFO, TRACE or WARNING");
            }

            LOG_DEBUG("Environment varMap Updateiable SOLAR_LOG_LEVEL={}", str_log_level);
        }

        LOG_INFO("Get component manager instance");
        SRef<xpcf::IComponentManager> componentManager = xpcf::getComponentManagerInstance();

        string file = options["file"].as<string>();
        LOG_INFO("Load Client configuration file: {}", file);

        if (componentManager->load(file.c_str()) != org::bcom::xpcf::_SUCCESS)
        {
            LOG_ERROR("Failed to load Client configuration file: {}", file);
            return -1;
        }

        LOG_INFO("Resolve IMapUpdatePipeline interface");
        SRef<pipeline::IMapUpdatePipeline> mapUpdateService =
                componentManager->resolve<SolAR::api::pipeline::IMapUpdatePipeline>();
        LOG_INFO("Resolve IAsyncRelocalisationPipeline interface");
        SRef<pipeline::IAsyncRelocalizationPipeline> frontEndService =
                componentManager->resolve<SolAR::api::pipeline::IAsyncRelocalizationPipeline>();

        LOG_INFO("Client components loaded");

        LOG_INFO("Initialize Map Update service");

        if (mapUpdateService->init() != FrameworkReturnCode::_SUCCESS)
        {
            LOG_ERROR("Failed to initialize Map Update service");
            return -1;
        }

        LOG_INFO("Start Map Update service");

        if (mapUpdateService->start() != FrameworkReturnCode::_SUCCESS)
        {
            LOG_ERROR("Failed to start Map Update service");
            return -1;
        }

        // Display the current global map

        auto gViewer3D = componentManager->resolve<display::I3DPointsViewer>();

        LOG_INFO("Try to get current global map from Map Update remote service");

        SRef<Map> globalMap;
        std::vector<SRef<Keyframe>> globalKeyframes;
        std::vector<SRef<CloudPoint>> globalPointCloud;

        auto last_request = std::chrono::high_resolution_clock::now();

        if (mapUpdateService->getMapRequest(globalMap) == FrameworkReturnCode::_SUCCESS) {

            LOG_INFO("Map Update request terminated");

            // Stop Map Update service
            if (mapUpdateService->stop() != FrameworkReturnCode::_SUCCESS)
            {
                LOG_ERROR("Failed to stop Map Update service");
                return -1;
            }

            // Retrieve point cloud and keyFrames from global map
            globalMap->getConstPointCloud()->getAllPoints(globalPointCloud);
            LOG_INFO("Number of cloud points: {}", globalPointCloud.size());

            if (globalPointCloud.size() > 0) {

                LOG_INFO("==> Display current global map: press ESC on the map display window to end test");

                datastructure::Transform3Df poseNotUsed;
                std::vector<datastructure::Transform3Df> poseReloc = {};
                std::vector<datastructure::Transform3Df> poseNoReloc = {};
                api::pipeline::TransformStatus transform3DStatus;
                datastructure::Transform3Df transform3D;
                float_t confidence;

                uint16_t nb_clients_registered = 1;

                while (true)
                {
                    // Calculate duration between two requests
                    auto current_time = std::chrono::high_resolution_clock::now();
                    std::chrono::duration<double, std::milli> elapsed_time = current_time - last_request;

                    // If delay is reached
                    if (elapsed_time.count() > FRONT_END_REQUEST_DELAY) {

                        datastructure::Transform3Df poseClient;
                        poseReloc.clear();
                        poseNoReloc.clear();

                        // Get clients UUID from Front End
                        std::vector<std::string> clients_UUID;

                        if (frontEndService->getAllClientsUUID(clients_UUID) != FrameworkReturnCode::_SUCCESS) {
                            LOG_ERROR("Failed to get all clients UUID");
                        }
                        else {

                            if (clients_UUID.size() > 0) {
                                if (nb_clients_registered < clients_UUID.size()) {
                                    LOG_INFO("New client(s) registered: display poses...");
                                    nb_clients_registered = clients_UUID.size();
                                }
                                for (auto const & uuid : clients_UUID) {
                                    if (frontEndService->getLastPose(uuid, poseClient) == FrameworkReturnCode::_SUCCESS) {

                                        // Check if a new transformation matrix has been found
                                        if (frontEndService->get3DTransformRequest(uuid, transform3DStatus, transform3D, confidence)
                                                == FrameworkReturnCode::_SUCCESS) {
                                            if (transform3DStatus == api::pipeline::NEW_3DTRANSFORM) {
                                                poseReloc.push_back(poseClient);
                                            }
                                            else {
                                                poseNoReloc.push_back(poseClient);
                                            }
                                        }
                                    }
                                }
                            }
                            else {
                                if (nb_clients_registered > 0) {
                                    LOG_INFO("No clients currently registered: waiting for client connections...");
                                    nb_clients_registered = 0;
                                }
                            }
                        }

                        if (gViewer3D->display(globalPointCloud, poseNotUsed, poseNoReloc, {}, {}, poseReloc) == FrameworkReturnCode::_STOP)
                            break;

                        last_request = std::chrono::high_resolution_clock::now();
                    }
                }
            }
            else {
                LOG_INFO("Current global map is empty!");
            }
        }
        else {
            LOG_INFO("No current global map!");
        }
    }
    catch (xpcf::Exception & e) {
        LOG_INFO("The following exception has been caught: {}", e.what());
        return -1;
    }

    return 0;
}
