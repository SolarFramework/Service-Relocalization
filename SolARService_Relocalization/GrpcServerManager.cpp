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

#include "GrpcServerManager.h"

namespace org { namespace bcom { namespace xpcf {

GrpcServerManager::GrpcServerManager():ConfigurableBase(toMap<GrpcServerManager>())
{
    LOG_DEBUG("ConfigurableBase");

    declareInterface<IGrpcServerManager>(this);
    declareProperty("server_address",m_serverAddress);
    declareProperty("server_credentials",m_serverCredentials);
    declareProperty("max_receive_message_size", m_receiveMessageMaxSize);
    declareProperty("max_send_message_size", m_sendMessageMaxSize);
    declareProperty("external_url", m_externalURL);
    declareInjectable<IGrpcService>(m_services);

    declareInjectable<api::pipeline::IServiceManagerPipeline>(m_serviceManager, true);

    LOG_DEBUG("this->getNbInterfaces() = {}", this->getNbInterfaces());
}

GrpcServerManager::~GrpcServerManager()
{
    LOG_DEBUG("~GrpcServerManager");

    LOG_DEBUG("Unregister the service to the Service Manager");

    m_serviceManager->unregisterService(api::pipeline::ServiceType::RELOCALIZATION_SERVICE, m_externalURL);
}

void GrpcServerManager::unloadComponent ()
{
    LOG_DEBUG("~unloadComponent");

    // provide component cleanup strategy
    // default strategy is to delete self, uncomment following line in this case :
    delete this;
    return;
}

XPCFErrorCode GrpcServerManager::onConfigured()
{
    return XPCFErrorCode::_SUCCESS;
}

void GrpcServerManager::registerService(grpc::Service * service)
{
    m_builder.RegisterService(service);
}

void GrpcServerManager::registerService(const std::string & host, grpc::Service * service)
{
    m_builder.RegisterService(host, service);
}

void GrpcServerManager::registerService(SRef<IGrpcService> service)
{
    registerService(service->getService());
}

void GrpcServerManager::registerService(const std::string & host, SRef<IGrpcService> service)
{
    registerService(host, service->getService());
}

void GrpcServerManager::runServer()
{
    if (m_receiveMessageMaxSize > 0) {
        m_builder.SetMaxReceiveMessageSize(m_receiveMessageMaxSize);
        LOG_DEBUG("SetMaxReceiveMessageSize: {}", m_receiveMessageMaxSize);
    }
    else if (m_receiveMessageMaxSize < 0) {
        // Set message size to max value
        m_builder.SetMaxReceiveMessageSize(LONG_MAX);
        LOG_DEBUG("SetMaxReceiveMessageSize: {}", LONG_MAX);
    }

    if (m_sendMessageMaxSize > 0) {
        m_builder.SetMaxSendMessageSize(m_sendMessageMaxSize);
        LOG_DEBUG("SetMaxSendMessageSize: {}", m_sendMessageMaxSize);
    }
    else if (m_sendMessageMaxSize < 0) {
        // Set message size to max value
        m_builder.SetMaxSendMessageSize(LONG_MAX);
        LOG_DEBUG("SetMaxSendMessageSize: {}", LONG_MAX);
    }

    m_builder.AddListeningPort(m_serverAddress, GrpcHelper::getServerCredentials(static_cast<grpcCredentials>(m_serverCredentials)));
    for (auto service: *m_services) {
        LOG_DEBUG("Registering IGrpcService #  {}", service->getServiceName());
        registerService(service);
    }

    LOG_DEBUG("Register the new service to the Service Manager with URL: {}", m_externalURL);

    m_serviceManager->registerService(api::pipeline::ServiceType::RELOCALIZATION_SERVICE, m_externalURL);

    std::unique_ptr<grpc::Server> server(m_builder.BuildAndStart());
    LOG_DEBUG("Server listening on  {}", m_serverAddress);
    server->Wait();
}

}}}
