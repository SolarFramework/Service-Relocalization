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

#include "RelocalizationAndMappingGrpcServiceImpl.h"

#include <iostream>
#include <signal.h>

#include <unistd.h>

#include <cxxopts.hpp>

// #include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <xpcf/xpcf.h>
#include <xpcf/api/IComponentManager.h>
#include <xpcf/core/helpers.h>
#include <boost/log/core.hpp>

#include <core/Log.h>
#include <api/pipeline/IAsyncRelocalizationPipeline.h>

#include <api/display/IImageViewer.h>
SRef<SolAR::api::display::IImageViewer> gImageViewer;

using namespace std;
using namespace SolAR;
using namespace SolAR::api;
using namespace SolAR::datastructure;
namespace xpcf=org::bcom::xpcf;

using com::bcom::solar::gprc::RelocalizationAndMappingGrpcServiceImpl;

const string DEFAULT_GRPC_LISTENING_PORT = "5002";

SRef<pipeline::IAsyncRelocalizationPipeline> resolvePipeline(const string& configFile);
void startService(pipeline::IAsyncRelocalizationPipeline* pipeline, string serverAddress);
void print_help(const cxxopts::Options& options);

int main(int argc, char* argv[])
{
#if NDEBUG
    boost::log::core::get()->set_logging_enabled(false);
#endif

LOG_ADD_LOG_TO_CONSOLE();
LOG_SET_DEBUG_LEVEL();

    cxxopts::Options option_list("SolARService_MappingAndRelocalizationProxy",
                                 "SolARService_MappingAndRelocalizationProxy - The CLI of the mapping and relocalization gRPC proxy.\n");
    option_list.add_options()
            ("h,help", "display this help and exit")
            ("v,version", "display version information and exit")
            ("f,file", "configuration file (mandatory)", cxxopts::value<string>())
            ("p,port", "port to which the gRPC service will listen to \
                (default: " + DEFAULT_GRPC_LISTENING_PORT + ")", cxxopts::value<int>());

    auto options = option_list.parse(argc, argv);
    if (options.count("help")) {
        print_help(option_list);
        return 0;
    }

    if (options.count("version"))
    {
        cout << "SolARService_MappingAndRelocalizationProxy version " << MYVERSION << std::endl << std::endl;
        return 0;
    }

    string port = DEFAULT_GRPC_LISTENING_PORT;
    if (options.count("port"))
    {
        port = options["port"].as<int>();
    }
    LOG_DEBUG("Port set to '{}'");

    if (!options.count("file") || options["file"].as<string>().empty()) {
           LOG_ERROR("Configuration file is missing");
           print_help(option_list);
           return 1;
    }

    string configFile = options["file"].as<string>();

    try
    {
        auto pipeline = resolvePipeline(configFile);
        startService(pipeline.get(), "0.0.0.0:" + port);
    }
    catch (const xpcf::Exception& e)
    {
        LOG_ERROR("XPCF exception: #{}: {}", e.getErrorCode(), e.what());
        return -1;
    }
    catch (const exception& e)
    {
        LOG_ERROR("Exception: {}", e.what());
        return -1;
    }
    catch(...)
    {
        LOG_ERROR("Unkown error");
        return -1;
    }

    return 0;
}

SRef<pipeline::IAsyncRelocalizationPipeline> resolvePipeline(const string& configFile)
{
    auto componentMgr = xpcf::getComponentManagerInstance();

    if (componentMgr->load(configFile.c_str()) != xpcf::_SUCCESS)
    {
        LOG_INFO("Failed to load Client Remote Service configuration file: {}", configFile);
        return nullptr;
    }

    gImageViewer = componentMgr->resolve<api::display::IImageViewer>();

    return componentMgr->resolve<pipeline::IAsyncRelocalizationPipeline>();
}

void startService(pipeline::IAsyncRelocalizationPipeline* pipeline, string serverAddress)
{
    grpc::EnableDefaultHealthCheckService(true);
    // grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    grpc::ServerBuilder builder;

    builder.AddListeningPort(serverAddress, grpc::InsecureServerCredentials());

    RelocalizationAndMappingGrpcServiceImpl grpcServices(pipeline, gImageViewer);
    builder.RegisterService(&grpcServices);

    LOG_INFO("Starting proxy gRPC service");
    unique_ptr<grpc::Server> grpcServer = builder.BuildAndStart();

    cout << "SolARDeviceGrpcService listening on " << serverAddress << std::endl;

    grpcServer->Wait();

}

void print_help(const cxxopts::Options& options)
{
    cout << options.help({""}) << '\n';
}
