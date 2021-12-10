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

#ifndef RELOCALIZATION_AND_MAPPING_GRPC_SERVICE_IMPL
#define RELOCALIZATION_AND_MAPPING_GRPC_SERVICE_IMPL

#include "grpc/solar_mapping_and_relocalization_proxy.grpc.pb.h"

#include <opencv2/opencv.hpp>

#include <api/pipeline/IAsyncRelocalizationPipeline.h>

namespace com::bcom::solar::gprc
{

class RelocalizationAndMappingGrpcServiceImpl
    final : public SolARMappingAndRelocalizationProxy::Service
{

public:
    RelocalizationAndMappingGrpcServiceImpl(SolAR::api::pipeline::IAsyncRelocalizationPipeline* pipeline);
    ~RelocalizationAndMappingGrpcServiceImpl() = default;

public:
    grpc::Status Init(grpc::ServerContext* context,
                      const Empty* request,
                      Empty* response)  override;

    grpc::Status Start(grpc::ServerContext* context,
                       const Empty* request,
                       Empty* response)  override;

    grpc::Status Stop(grpc::ServerContext* context,
                      const Empty* request,
                      Empty* response)  override;

    grpc::Status SetCameraParameters(grpc::ServerContext* context,
                                     const CameraParameters* request,
                                     Empty* response)  override;

    grpc::Status RelocalizeAndMap(grpc::ServerContext* context,
                                  const Frame* request,
                                  RelocalizationResult* response)  override;

    grpc::Status Get3DTransform(grpc::ServerContext* context,
                                const Empty* request,
                                RelocalizationResult* response)  override;

private:
    SolAR::api::pipeline::IAsyncRelocalizationPipeline* m_pipeline;

private:
    static std::string to_string(CameraType type);
    static std::string to_string(Matrix3x3 mat);
    static std::string to_string(Matrix4x4 mat);

    static SolAR::datastructure::CameraType toSolAR(CameraType type);
    static SolAR::datastructure::Transform3Df toSolAR(const Matrix4x4& gRpcPose);
    static void toGrpc(const SolAR::datastructure::Transform3Df& solARPose, Matrix4x4& gRpcPose);
    static grpc::Status buildSolARImage(const Frame*, SRef<SolAR::datastructure::Image>& image);
    static grpc::Status toSolAR(/* const */ cv::Mat& imgSrc, SRef<SolAR::datastructure::Image>& image);
    static grpc::Status toGrpc(SolAR::api::pipeline::TransformStatus solARPoseStatus, RelocalizationPoseStatus& gRpcPoseStatus);

    static grpc::Status gRpcError(std::string message, grpc::StatusCode gRpcStatus = grpc::StatusCode::INTERNAL);

};

} // namespace com::bcom::solar::gprc

#endif // RELOCALIZATION_AND_MAPPING_GRPC_SERVICE_IMPL
