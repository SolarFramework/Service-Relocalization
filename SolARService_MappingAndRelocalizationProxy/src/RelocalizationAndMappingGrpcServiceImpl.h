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
#include <api/display/IImageViewer.h>

namespace com::bcom::solar::gprc
{

class RelocalizationAndMappingGrpcServiceImpl
    final : public SolARMappingAndRelocalizationProxy::Service
{

public:
    RelocalizationAndMappingGrpcServiceImpl() = default;

    RelocalizationAndMappingGrpcServiceImpl(SolAR::api::pipeline::IAsyncRelocalizationPipeline* pipeline);

    RelocalizationAndMappingGrpcServiceImpl(SolAR::api::pipeline::IAsyncRelocalizationPipeline* pipeline,
                                            std::string saveFolder);

    RelocalizationAndMappingGrpcServiceImpl(SolAR::api::pipeline::IAsyncRelocalizationPipeline* pipeline,
                                            std::string saveFolder,
                                            SRef<SolAR::api::display::IImageViewer> image_viewer);

    ~RelocalizationAndMappingGrpcServiceImpl() = default;

public:
    grpc::Status Init(grpc::ServerContext* context,
                      const PipelineModeValue* request,
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

    grpc::Status Reset(grpc::ServerContext *context,
                       const Empty *request,
                       Empty *response) override;

    grpc::Status SendMessage(grpc::ServerContext* context,
                             const Message* request,
                             Empty* response) override;


private:
    SolAR::api::pipeline::IAsyncRelocalizationPipeline* m_pipeline;

    bool m_started; // Indicates if the proxy is started or not

    SRef<SolAR::api::display::IImageViewer> m_image_viewer;

    // Variables used to save images on disk
    long m_index_image;
    std::ofstream m_poseFile;
    std::ofstream m_timestampFile;
    std::string m_file_path, m_image_path;

    // Vector of ordered tuple(image, pose, timestamp)
    std::vector<std::tuple<SRef<SolAR::datastructure::Image>, SolAR::datastructure::Transform3Df, long>> m_ordered_images;
    std::mutex m_images_vector_mutex;   // Mutex used to control vector access
    long m_last_image_timestamp;        // Timestamp of the last image processed

    // Sort vector of tuples according to the third element of tuple
    static bool sortbythird (
            const std::tuple<SRef<SolAR::datastructure::Image>, SolAR::datastructure::Transform3Df, long> a ,
            const std::tuple<SRef<SolAR::datastructure::Image>, SolAR::datastructure::Transform3Df, long> b);

private:
    static std::string to_string(CameraType type);
    static std::string to_string(Matrix3x3 mat);
    static std::string to_string(Matrix4x4 mat);
    static std::string to_string(ImageLayout layout);
    static std::string to_string(SolAR::api::pipeline::TransformStatus transformStatus);
    static std::string to_string(ImageCompression compression);

    static SolAR::datastructure::CameraType toSolAR(CameraType type);
    static SolAR::datastructure::Transform3Df toSolAR(const Matrix4x4& gRpcPose);
    static void toGrpc(const SolAR::datastructure::Transform3Df& solARPose, Matrix4x4& gRpcPose);
    static grpc::Status buildSolARImage(const Frame*, const SolAR::datastructure::Transform3Df& solARPose, SRef<SolAR::datastructure::Image>& image);
    static grpc::Status toSolAR(/* const */ cv::Mat& imgSrc, SRef<SolAR::datastructure::Image>& image);
    static grpc::Status toGrpc(SolAR::api::pipeline::TransformStatus solARPoseStatus, RelocalizationPoseStatus& gRpcPoseStatus);
    static void imageToOpenCV(SRef<SolAR::datastructure::Image> imgSrc, cv::Mat& imgDest);
    static SolAR::api::pipeline::PipelineMode toSolAR(PipelineMode pipelineMode);

    static grpc::Status gRpcError(std::string message, grpc::StatusCode gRpcStatus = grpc::StatusCode::INTERNAL);

};

} // namespace com::bcom::solar::gprc

#endif // RELOCALIZATION_AND_MAPPING_GRPC_SERVICE_IMPL
