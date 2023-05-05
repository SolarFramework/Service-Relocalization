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
#include <boost/log/core.hpp>

#include <xpcf/api/IComponentManager.h>
#include <xpcf/core/helpers.h>
#include "GrpcServerManager.h"
#include <cstdlib>
#include <boost/filesystem.hpp>
#include <boost/filesystem/detail/utf8_codecvt_facet.hpp>
#include "core/Log.h"

#include "api/pipeline/IServiceManagerPipeline.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif // _WIN32

using namespace SolAR;

namespace fs = boost::filesystem;

namespace xpcf = org::bcom::xpcf;

// print help options
void print_help(const cxxopts::Options& options)
{
    std::cout << options.help({""}) << std::endl;
}

// print error message
void print_error(const std::string& msg)
{
    std::cerr << msg << std::endl;
}

void tryConfigureServer(SRef<xpcf::IGrpcServerManager> server, const std::string & propName, const std::string & envVarName)
{
    char * envValue = getenv(envVarName.c_str());
    if (envValue != nullptr) {
        LOG_DEBUG("Environment variable {}: {}", envVarName, envValue);
        xpcf::IProperty::PropertyType type = server->bindTo<xpcf::IConfigurable>()->getProperty(propName.c_str())->getType();
        switch (type) {
        case xpcf::IProperty::PropertyType::IProperty_CHARSTR:
            server->bindTo<xpcf::IConfigurable>()->getProperty(propName.c_str())->setStringValue(envValue);
            break;

        case xpcf::IProperty::PropertyType::IProperty_UINTEGER:
            server->bindTo<xpcf::IConfigurable>()->getProperty(propName.c_str())->setUnsignedIntegerValue(std::atoi(envValue));
            break;

        case xpcf::IProperty::PropertyType::IProperty_LONG:
            server->bindTo<xpcf::IConfigurable>()->getProperty(propName.c_str())->setLongValue(std::atol(envValue));
        break;
        default:
            LOG_DEBUG("GrpcServerManager Property type not handled");
            break;
        }
    }
}

int main(int argc, char* argv[])
{
#if NDEBUG
    boost::log::core::get()->set_logging_enabled(false);
#endif

    LOG_ADD_LOG_TO_CONSOLE();

    fs::detail::utf8_codecvt_facet utf8;
    SRef<xpcf::IComponentManager> cmpMgr = xpcf::getComponentManagerInstance();
    cmpMgr->bindLocal<xpcf::IGrpcServerManager,xpcf::GrpcServerManager>();
    std::string configSrc;
    fs::path currentPath(boost::filesystem::initial_path().generic_string(utf8));
    configSrc = currentPath.generic_string(utf8);

    cxxopts::Options option_list("SolARService_RelocalizationMarkers",
                                 "SolARService_RelocalizationMarkers - The commandline interface to the xpcf grpc server application.\n");
    option_list.add_options()
            ("h,help", "display this help and exit")
            ("v,version", "display version information and exit")
            ("m,modules", "XPCF modules configuration file",
             cxxopts::value<std::string>())
            ("p,properties", "XPCF properties configuration file",
             cxxopts::value<std::string>());

    auto options = option_list.parse(argc, argv);
    if (options.count("help")) {
        print_help(option_list);
        return 0;
    }
    else if (options.count("version"))
    {
        std::cout << "SolARService_RelocalizationMarkers version " << MYVERSION << std::endl << std::endl;
        return 0;
    }
    else if ((!options.count("modules") || options["modules"].as<std::string>().empty())
          || (!options.count("properties") || options["properties"].as<std::string>().empty())) {
        LOG_ERROR("missing one of modules (-m) or properties (-p) argument");
        return -1;
    }

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

        LOG_DEBUG("Environment variable SOLAR_LOG_LEVEL={}", str_log_level);
    }

    configSrc = options["modules"].as<std::string>();

    LOG_INFO("Load modules configuration file: {}", configSrc);

    if (cmpMgr->load(configSrc.c_str()) != org::bcom::xpcf::_SUCCESS) {
        LOG_ERROR("Failed to load modules configuration file: {}", configSrc);
        return -1;
    }

    configSrc = options["properties"].as<std::string>();

    LOG_INFO("Load properties configuration file: {}", configSrc);

    if (cmpMgr->load(configSrc.c_str()) != org::bcom::xpcf::_SUCCESS) {
        LOG_ERROR("Failed to load properties configuration file: {}", configSrc);
        return -1;
    }

    // Get the external URL of the service
    char * externalURL = getenv("SERVER_EXTERNAL_URL");
    if (externalURL == nullptr) {
     LOG_ERROR("The external URL of the service must be defined using the SERVER_EXTERNAL_URL env var!");
     return -1;
    }

    LOG_DEBUG("Environment variable SERVER_EXTERNAL_URL: {}", externalURL);

    // Get Service Manager proxy
    auto serviceManager = cmpMgr->resolve<api::pipeline::IServiceManagerPipeline>();

    LOG_DEBUG("Register the new service to the Service Manager with URL: {}", externalURL);

    bool isRegistered = false;

    while(!isRegistered) {
        try {
            if (serviceManager->registerService(api::pipeline::ServiceType::RELOCALIZATION_MARKERS_SERVICE,
                                                std::string(externalURL)) == FrameworkReturnCode::_SUCCESS) {
                isRegistered = true;
            }
            else {
                LOG_ERROR("Fail to register the service to the Service Manager!");
                return -1;
            }
        }
        catch (const std::exception &e) {
            LOG_WARNING("Waiting for the Service Manager...");
#ifdef _WIN32
            Sleep(1);
#else
            sleep(1);
#endif
        }
    }

    auto serverMgr = cmpMgr->resolve<xpcf::IGrpcServerManager>();

    // Check environment variables
    tryConfigureServer(serverMgr, "server_address", "XPCF_GRPC_SERVER_URL");
    tryConfigureServer(serverMgr, "server_credentials", "XPCF_GRPC_CREDENTIALS");
    tryConfigureServer(serverMgr, "max_receive_message_size", "XPCF_GRPC_MAX_RECV_MSG_SIZE");
    tryConfigureServer(serverMgr, "max_send_message_size", "XPCF_GRPC_MAX_SEND_MSG_SIZE");

    LOG_INFO ("LOG LEVEL: {}", str_log_level);
    LOG_INFO ("GRPC SERVER ADDRESS: {}",
              serverMgr->bindTo<xpcf::IConfigurable>()->getProperty("server_address")->getStringValue());
    LOG_INFO ("GRPC SERVER CREDENTIALS: {}",
              serverMgr->bindTo<xpcf::IConfigurable>()->getProperty("server_credentials")->getUnsignedIntegerValue());
    uint64_t max_msg_size = serverMgr->bindTo<xpcf::IConfigurable>()->getProperty("max_receive_message_size")->getLongValue();
    if (max_msg_size == 0) {
        LOG_INFO ("GRPC MAX RECEIVED MESSAGE SIZE: 4000000 (default) {}", max_msg_size);
    }
    else {
        LOG_INFO ("GRPC MAX RECEIVED MESSAGE SIZE: {}", max_msg_size);
    }
    max_msg_size = serverMgr->bindTo<xpcf::IConfigurable>()->getProperty("max_send_message_size")->getLongValue();
    if (max_msg_size == 0) {
        LOG_INFO ("GRPC MAX SENT MESSAGE SIZE: 4000000 (default)");
    }
    else {
        LOG_INFO ("GRPC MAX SENT MESSAGE SIZE: {}", max_msg_size);
    }

    LOG_INFO ("XPCF gRPC server listens on: {}",
              serverMgr->bindTo<xpcf::IConfigurable>()->getProperty("server_address")->getStringValue())

    serverMgr->runServer();

    LOG_DEBUG("Unregister the service to the Service Manager");

    serviceManager->unregisterService(api::pipeline::ServiceType::RELOCALIZATION_MARKERS_SERVICE,
                                      std::string(externalURL));

    return 0;
}
