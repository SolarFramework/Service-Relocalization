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

#include <chrono>
#include <string>

#include <xpcf/xpcf.h>
#include <xpcf/api/IComponentManager.h>
#include <xpcf/core/helpers.h>
#include <boost/log/core.hpp>
#include <boost/filesystem.hpp>
#include <core/Log.h>

using grpc::Status;
using grpc::StatusCode;

using SolAR::Log;

using SolARImage = SolAR::datastructure::Image;

namespace com::bcom::solar::gprc
{

class Fps
{
    typedef std::chrono::system_clock Time;
    typedef std::chrono::milliseconds ms;

public:

    Fps(){}
    Fps(unsigned int computePeriodMs, unsigned int windowSize)
        :m_computePeriodMs{computePeriodMs},
          m_windowSize{windowSize}
    {}

  float update()
  {
    auto now = Time::now();
    auto timeElapsed = now - m_lastTime;
    m_lastTime = now;

    m_lastTenDeltas.push_back(std::chrono::duration_cast<ms>(timeElapsed).count());

    if (m_lastTenDeltas.size() > m_windowSize)
    {
      m_lastTenDeltas.erase(m_lastTenDeltas.begin());
    }

    if (now - m_lastTimeComputed > m_computePeriodMs)
    {
        m_currentFps = 1000.f / (std::accumulate(m_lastTenDeltas.begin(), m_lastTenDeltas.end(), 0.f) / m_lastTenDeltas.size());
        m_lastTimeComputed = now;
    }

    return m_currentFps;
  }

private:
  std::chrono::milliseconds m_computePeriodMs{1000};
  unsigned int m_windowSize{10};
  float m_currentFps{0};

  std::vector<float> m_lastTenDeltas;
  std::chrono::time_point<std::chrono::system_clock> m_lastTime;
  std::chrono::time_point<std::chrono::system_clock> m_lastTimeComputed;
};

Fps relocAndMapFps;

RelocalizationAndMappingGrpcServiceImpl::RelocalizationAndMappingGrpcServiceImpl(
        SolAR::api::pipeline::IAsyncRelocalizationPipeline* pipeline): m_pipeline{ pipeline }
{}

RelocalizationAndMappingGrpcServiceImpl::RelocalizationAndMappingGrpcServiceImpl(
        SolAR::api::pipeline::IAsyncRelocalizationPipeline* pipeline,
        std::string saveFolder): m_pipeline{ pipeline }, m_file_path { saveFolder }
{
    // Save images and poses processing function
    if (saveFolder != "") {
        auto fnSaveImagesProcessing = [&]() {
            saveImages();
        };

        m_saveImagesTask = new xpcf::DelegateTask(fnSaveImagesProcessing, true);
    }
}

RelocalizationAndMappingGrpcServiceImpl::RelocalizationAndMappingGrpcServiceImpl(
        SolAR::api::pipeline::IAsyncRelocalizationPipeline* pipeline,
        std::string saveFolder,
        SRef<SolAR::api::display::IImageViewer> image_viewer_left,
        SRef<SolAR::api::display::IImageViewer> image_viewer_right):
            m_pipeline{ pipeline }, m_file_path { saveFolder },
            m_image_viewer_left {image_viewer_left}, m_image_viewer_right{image_viewer_right}
{
    // Display images processing function
    if (image_viewer_left) {
        auto fnDisplayImagesProcessing = [&]() {
            displayImages();
        };

        m_displayImagesTask = new xpcf::DelegateTask(fnDisplayImagesProcessing, true);
    }

    // Save images and poses processing function
    if (saveFolder != "") {
        auto fnSaveImagesProcessing = [&]() {
            saveImages();
        };

        m_saveImagesTask = new xpcf::DelegateTask(fnSaveImagesProcessing, true);
    }
}

RelocalizationAndMappingGrpcServiceImpl::~RelocalizationAndMappingGrpcServiceImpl()
{
    LOG_DEBUG("RelocalizationAndMappingGrpcServiceImpl destructor");

    if (m_displayImagesTask != nullptr)
        delete m_displayImagesTask;

    if (m_saveImagesTask != nullptr)
        delete m_saveImagesTask;
}

grpc::Status
RelocalizationAndMappingGrpcServiceImpl::Init(grpc::ServerContext* context,
                                              const PipelineModeValue* request,
                                              Empty* response)
{
    LOG_INFO("Init mapping and relocalization service");

    if (m_pipeline->init(toSolAR(request->pipeline_mode())) != SolAR::FrameworkReturnCode::_SUCCESS) {
        LOG_ERROR("Error while initializing the mapping and relocalization front end service");
        return gRpcError("Error while initializing the mapping and relocalization front end service");
    }

    m_started = false;

    LOG_DEBUG("Init mapping and relocalization service OK");

    return Status::OK;
}

grpc::Status
RelocalizationAndMappingGrpcServiceImpl::Start(grpc::ServerContext* context,
                                               const Empty* request,
                                               Empty* response)
{
    if (m_started) {
        LOG_INFO("Proxy is already started");
        return Status::OK;
    }

    LOG_INFO("Start mapping and relocalization service");

    if (m_pipeline->start() != SolAR::FrameworkReturnCode::_SUCCESS) {
        LOG_ERROR("Error while starting the mapping and relocalization front end service");
        return gRpcError("Error while starting the mapping and relocalization front end service");
    }

    m_cameraMode = UNKNOWN_CAMERA_MODE;
    m_ordered_images.clear();
    m_last_image_timestamp = 0;

    m_index_image = 0;
    if (m_file_path != "") {
        m_image1_path = m_file_path + "/000/";
        boost::filesystem::create_directories(boost::filesystem::path(m_image1_path.c_str()));
        m_poseFile1.open(m_file_path + "/pose_000.txt");
        m_image2_path = m_file_path + "/001/";
        boost::filesystem::create_directories(boost::filesystem::path(m_image2_path.c_str()));
        m_poseFile2.open(m_file_path + "/pose_001.txt");
        m_timestampFile.open(m_file_path + "/timestamps.txt");
    }

    if (m_displayImagesTask != nullptr)
        m_displayImagesTask->start();

    if (m_saveImagesTask != nullptr)
        m_saveImagesTask->start();

    m_started = true;

    LOG_DEBUG("Start mapping and relocalization service OK");

    return Status::OK;
}

grpc::Status
RelocalizationAndMappingGrpcServiceImpl::Stop(grpc::ServerContext* context,
                                              const Empty* request,
                                              Empty* response)
{
    if (!m_started) {
        LOG_INFO("Proxy is not started");
        return Status::OK;
    }

    m_started = false;

    LOG_INFO("Stop mapping and relocalization service");

    if (m_pipeline->stop() != SolAR::FrameworkReturnCode::_SUCCESS)
    {
        return gRpcError("Error while stopping the mapping and relocalization front end service");
    }

    if (m_file_path != "") {
        // Wait to let all data be saved in files
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        m_poseFile1.close();
        m_poseFile2.close();
        m_timestampFile.close();
    }

    if (m_displayImagesTask != nullptr)
        m_displayImagesTask->stop();

    if (m_saveImagesTask != nullptr)
        m_saveImagesTask->stop();

    LOG_DEBUG("Stop mapping and relocalization service OK");

    return Status::OK;
}

grpc::Status
RelocalizationAndMappingGrpcServiceImpl::SetCameraParameters(grpc::ServerContext* context,
                                                             const CameraParameters* request,
                                                             Empty* response)
{
    LOG_INFO("Set camera parameters for relocalization and mapping");
    LOG_DEBUG("   name: {}", request->name());
    LOG_DEBUG("   id: {}", request->id());
    LOG_DEBUG("   type: {}", to_string(request->camera_type()));
    LOG_DEBUG("   resolution: {}x{}", request->width(), request->height());
    LOG_DEBUG("   intrinsics:");
    LOG_DEBUG("{}", to_string(request->intrinsics()));
    LOG_DEBUG("   distortion: K_1={}, K_2={}, P_1={}, P_2={}, K_3={}",
              request->distortion().k_1(),
              request->distortion().k_2(),
              request->distortion().p_1(),
              request->distortion().p_2(),
              request->distortion().k_3());

    SolAR::datastructure::CameraParameters solarCamParams;
    solarCamParams.name = request->name();
    solarCamParams.id = request->id();
    solarCamParams.type = toSolAR(request->camera_type());
    solarCamParams.resolution.width = request->width();
    solarCamParams.resolution.height = request->height();

    solarCamParams.intrinsic(0,0) = request->intrinsics().m11();
    solarCamParams.intrinsic(0,1) = request->intrinsics().m12();
    solarCamParams.intrinsic(0,2) = request->intrinsics().m13();
    solarCamParams.intrinsic(1,0) = request->intrinsics().m21();
    solarCamParams.intrinsic(1,1) = request->intrinsics().m22();
    solarCamParams.intrinsic(1,2) = request->intrinsics().m23();
    solarCamParams.intrinsic(2,0) = request->intrinsics().m31();
    solarCamParams.intrinsic(2,1) = request->intrinsics().m32();
    solarCamParams.intrinsic(2,2) = request->intrinsics().m33();

    solarCamParams.distortion(0,0) = request->distortion().k_1();
    solarCamParams.distortion(1,0) = request->distortion().k_2();
    solarCamParams.distortion(2,0) = request->distortion().p_1();
    solarCamParams.distortion(3,0) = request->distortion().p_1();
    solarCamParams.distortion(4,0) = request->distortion().k_3();

    if (m_pipeline->setCameraParameters(solarCamParams) != SolAR::FrameworkReturnCode::_SUCCESS)
    {
        return gRpcError("Error while setting camera parameters for the mapping and relocalization front end service");
    }

    LOG_DEBUG("Set camera parameters for relocalization and mapping OK");

    return Status::OK;
}

grpc::Status
RelocalizationAndMappingGrpcServiceImpl::setRectificationParameters(grpc::ServerContext* context,
                                                                    const RectificationParameters* request,
                                                                    Empty* response)
{
    LOG_INFO("Set camera rectification parameters for relocalization and mapping");

    LOG_DEBUG("Camera 1:");
    LOG_DEBUG("   rotation:");
    LOG_DEBUG("{}", to_string(request->cam1_rotation()));
    LOG_DEBUG("   projection:");
    LOG_DEBUG("{}", to_string(request->cam1_projection()));
    LOG_DEBUG("   stereo type: {}", to_string(request->cam1_stereo_type()));
    LOG_DEBUG("   baseline: {}", request->cam1_baseline());
    LOG_DEBUG("Camera 2:");
    LOG_DEBUG("   rotation:");
    LOG_DEBUG("{}", to_string(request->cam2_rotation()));
    LOG_DEBUG("   projection:");
    LOG_DEBUG("{}", to_string(request->cam2_projection()));
    LOG_DEBUG("   stereo type: {}", to_string(request->cam2_stereo_type()));
    LOG_DEBUG("   baseline: {}", request->cam2_baseline());

    SolAR::datastructure::RectificationParameters solarCam1RectParams, solarCam2RectParams;
    solarCam1RectParams.rotation(0,0) = request->cam1_rotation().m11();
    solarCam1RectParams.rotation(0,1) = request->cam1_rotation().m12();
    solarCam1RectParams.rotation(0,2) = request->cam1_rotation().m13();
    solarCam1RectParams.rotation(1,0) = request->cam1_rotation().m21();
    solarCam1RectParams.rotation(1,1) = request->cam1_rotation().m22();
    solarCam1RectParams.rotation(1,2) = request->cam1_rotation().m23();
    solarCam1RectParams.rotation(2,0) = request->cam1_rotation().m31();
    solarCam1RectParams.rotation(2,1) = request->cam1_rotation().m32();
    solarCam1RectParams.rotation(2,2) = request->cam1_rotation().m33();
    solarCam1RectParams.projection(0,0) = request->cam1_projection().m11();
    solarCam1RectParams.projection(0,1) = request->cam1_projection().m12();
    solarCam1RectParams.projection(0,2) = request->cam1_projection().m13();
    solarCam1RectParams.projection(0,3) = request->cam1_projection().m14();
    solarCam1RectParams.projection(1,0) = request->cam1_projection().m21();
    solarCam1RectParams.projection(1,1) = request->cam1_projection().m22();
    solarCam1RectParams.projection(1,2) = request->cam1_projection().m23();
    solarCam1RectParams.projection(1,3) = request->cam1_projection().m24();
    solarCam1RectParams.projection(2,0) = request->cam1_projection().m31();
    solarCam1RectParams.projection(2,1) = request->cam1_projection().m32();
    solarCam1RectParams.projection(2,2) = request->cam1_projection().m33();
    solarCam1RectParams.projection(2,3) = request->cam1_projection().m34();
    solarCam1RectParams.type = toSolAR(request->cam1_stereo_type());
    solarCam1RectParams.baseline = request->cam1_baseline();
    solarCam2RectParams.rotation(0,0) = request->cam2_rotation().m11();
    solarCam2RectParams.rotation(0,1) = request->cam2_rotation().m12();
    solarCam2RectParams.rotation(0,2) = request->cam2_rotation().m13();
    solarCam2RectParams.rotation(1,0) = request->cam2_rotation().m21();
    solarCam2RectParams.rotation(1,1) = request->cam2_rotation().m22();
    solarCam2RectParams.rotation(1,2) = request->cam2_rotation().m23();
    solarCam2RectParams.rotation(2,0) = request->cam2_rotation().m31();
    solarCam2RectParams.rotation(2,1) = request->cam2_rotation().m32();
    solarCam2RectParams.rotation(2,2) = request->cam2_rotation().m33();
    solarCam2RectParams.projection(0,0) = request->cam2_projection().m11();
    solarCam2RectParams.projection(0,1) = request->cam2_projection().m12();
    solarCam2RectParams.projection(0,2) = request->cam2_projection().m13();
    solarCam2RectParams.projection(0,3) = request->cam2_projection().m14();
    solarCam2RectParams.projection(1,0) = request->cam2_projection().m21();
    solarCam2RectParams.projection(1,1) = request->cam2_projection().m22();
    solarCam2RectParams.projection(1,2) = request->cam2_projection().m23();
    solarCam2RectParams.projection(1,3) = request->cam2_projection().m24();
    solarCam2RectParams.projection(2,0) = request->cam2_projection().m31();
    solarCam2RectParams.projection(2,1) = request->cam2_projection().m32();
    solarCam2RectParams.projection(2,2) = request->cam2_projection().m33();
    solarCam2RectParams.projection(2,3) = request->cam2_projection().m34();
    solarCam2RectParams.type = toSolAR(request->cam2_stereo_type());
    solarCam2RectParams.baseline = request->cam2_baseline();

    if (m_pipeline->setRectificationParameters(solarCam1RectParams, solarCam2RectParams) != SolAR::FrameworkReturnCode::_SUCCESS)
    {
        return gRpcError("Error while setting camera rectification parameters for the mapping and relocalization front end service");
    }

    LOG_DEBUG("Set camera rectification parameters for relocalization and mapping OK");

    return Status::OK;
}

bool RelocalizationAndMappingGrpcServiceImpl::sortbythird (
        const std::tuple<std::vector<SRef<SolAR::datastructure::Image>>,
                         std::vector<SolAR::datastructure::Transform3Df>, long> a,
        const std::tuple<std::vector<SRef<SolAR::datastructure::Image>>,
                         std::vector<SolAR::datastructure::Transform3Df>, long> b)
{
    return (std::get<2>(a) < std::get<2>(b));
}

grpc::Status
RelocalizationAndMappingGrpcServiceImpl::RelocalizeAndMap(grpc::ServerContext* context,
                                                          const Frames* request,
                                                          RelocalizationResult* response)
{
    response->set_confidence(0);
    response->set_mapping_status(MappingStatus::BOOTSTRAP);
    response->set_pose_status(RelocalizationPoseStatus::NO_POSE);

    if (!m_started) {
        LOG_INFO("Proxy is not started");
        return Status::OK;
    }

    m_images_vector_mutex.lock();

    if ((request->frames_size() == 1) && (m_cameraMode != CAMERA_MONO)) {
        if (m_cameraMode == UNKNOWN_CAMERA_MODE) {
            m_cameraMode = CAMERA_MONO;
            LOG_INFO("Camera mode = MONO");
        }
        else {
            LOG_WARNING("Only 1 image received in stereo mode: drop image");
            m_images_vector_mutex.unlock();
            return Status::OK;
        }
    }
    else if ((request->frames_size() == 2) && (m_cameraMode != CAMERA_STEREO)) {
        if (m_cameraMode == UNKNOWN_CAMERA_MODE) {
            m_cameraMode = CAMERA_STEREO;
            LOG_INFO("Camera mode = STEREO");
        }
        else {
            LOG_WARNING("2 images received in mono mode: switch to stereo mode");
            m_cameraMode = CAMERA_STEREO;
        }
    }
    else if ((request->frames_size() == 0) || (request->frames_size() > 2)) {
        LOG_ERROR("Unexpected number of images: {}", request->frames_size());
        m_images_vector_mutex.unlock();
        return Status::CANCELLED;
    }

    m_images_vector_mutex.unlock();

    auto fps = relocAndMapFps.update();

    LOG_INFO("{:03.2f} FPS", fps);

    long timestamp = request->frames(0).timestamp();

    // Drop image if too old (older than last processed image)
    if (timestamp < m_last_image_timestamp) {
        LOG_INFO("Image too old: drop it!");
        return Status::OK;
    }

    // Get data from request
    SRef<SolARImage> image1 = nullptr, image2 = nullptr;

    if (m_cameraMode == CAMERA_MONO) {
        LOG_DEBUG("Get image 1 from request");
        auto status  = buildSolARImage(request->frames(0), toSolAR(request->frames(0).pose()), image1);
        if (!status.ok())
        {
            LOG_ERROR("Error while converting received image 1 to SolAR datastructure");
            return status;
        }
    }
    else if (m_cameraMode == CAMERA_STEREO) {
        LOG_DEBUG("Get images from request");

        for (uint8_t i = 0; i < 2; i++) {
            if (request->frames(i).sensor_id() == 0) {
                // Left image
                auto status  = buildSolARImage(request->frames(i), toSolAR(request->frames(i).pose()), image1);
                if (!status.ok())
                {
                    LOG_ERROR("Error while converting received image 1 to SolAR datastructure");
                    return status;
                }
            }
            else if (request->frames(i).sensor_id() == 1) {
                // Right image
                auto status  = buildSolARImage(request->frames(i), toSolAR(request->frames(i).pose()), image2);
                if (!status.ok())
                {
                    LOG_ERROR("Error while converting received image 2 to SolAR datastructure");
                    return status;
                }
                // Rotate image from camera right front 180 degrees
                image2->rotate180();
            }
        }

        if ((image1 == nullptr) || (image2 == nullptr)) {
            LOG_ERROR("Error: can not found left and/or right image for stereo processing");
            return Status::CANCELLED;
        }
    }

    SolAR::datastructure::Transform3Df pose1, pose2;
    pose1 = toSolAR(request->frames(0).pose());
    if (m_cameraMode == CAMERA_STEREO)
        pose2 = toSolAR(request->frames(1).pose());

    SolAR::api::pipeline::TransformStatus transform3DStatus;
    SolAR::datastructure::Transform3Df transform3D;
    float_t confidence;
    SolAR::api::pipeline::MappingStatus mappingStatus;

    // Add data to vector of tuple (timestamp, image(s), pose(s))
    std::vector<SRef<SolAR::datastructure::Image>> images;
    std::vector<SolAR::datastructure::Transform3Df> poses;

    images.push_back(image1);
    poses.push_back(pose1);

    if (m_cameraMode == CAMERA_STEREO) {
        images.push_back(image2);
        poses.push_back(pose2);
    }

    m_images_vector_mutex.lock();

    m_ordered_images.push_back(std::make_tuple(images, poses, timestamp));

    // If enough tuples, send the older one to Front End
    if (m_ordered_images.size() >= 5) {

        // Sort vector based on timestamps
        std::sort(m_ordered_images.begin(), m_ordered_images.end(), sortbythird);

        std::vector<SRef<SolARImage>> imagesToSend = std::get<0>(m_ordered_images[0]);
        std::vector<SolAR::datastructure::Transform3Df> posesToSend = std::get<1>(m_ordered_images[0]);
        m_last_image_timestamp = std::get<2>(m_ordered_images[0]);

        // Remove the older tuple from vector
        m_ordered_images.erase(m_ordered_images.begin());

        m_images_vector_mutex.unlock();

        if (m_file_path == "") {

            LOG_DEBUG("Do mapping and relocalization");

            try {
                m_pipeline->relocalizeProcessRequest(
                            imagesToSend,
                            posesToSend,
                            std::chrono::time_point<std::chrono::system_clock>(
                                std::chrono::milliseconds(m_last_image_timestamp)),
                            transform3DStatus,
                            transform3D,
                            confidence,
                            mappingStatus);
            }
            catch (const std::exception& e)
            {
                return gRpcError("Error: exception thrown by relocation and mapping pipeline: "
                                 + std::string(e.what()));
            }

            // Display images if specified
            if (m_displayImagesTask != nullptr) {
                m_sharedBufferImageToDisplay.push(imagesToSend);
            }

            RelocalizationPoseStatus gRpcPoseStatus;
            auto status = toGrpc(transform3DStatus, gRpcPoseStatus);
            if (!status.ok())
            {
                LOG_ERROR("RelocalizeAndMap(): error while converting received image to SolAR datastructure");
                return status;
            }

            MappingStatus gRpcMappingStatus;
            status = toGrpc(mappingStatus, gRpcMappingStatus);
            if (!status.ok())
            {
                LOG_ERROR("RelocalizeAndMap(): error while converting received image to SoLAR datastructure");
                return status;
            }

            response->set_confidence(confidence);
            response->set_mapping_status(gRpcMappingStatus);
            if (gRpcMappingStatus == MappingStatus::BOOTSTRAP)
                response->set_pose_status(RelocalizationPoseStatus::NO_POSE);
            else
                response->set_pose_status(gRpcPoseStatus);
            toGrpc(transform3D, *response->mutable_pose());

            LOG_DEBUG("Output");
            LOG_DEBUG("  confidence: {}", confidence);
            LOG_DEBUG("  transform status: {}", to_string(transform3DStatus));
            LOG_DEBUG("  transform:\n{}", transform3D.matrix());
            LOG_DEBUG("  mapping status:\n{}", to_string(mappingStatus));

            return Status::OK;
        }
        else {

            LOG_DEBUG("Save images and poses on file");

            if (m_saveImagesTask != nullptr) {
                m_sharedBufferImagePoseToSave.push(std::make_pair(imagesToSend, posesToSend));
            }

            // Display images if specified
            if (m_displayImagesTask != nullptr) {
                m_sharedBufferImageToDisplay.push(imagesToSend);
            }

            return Status::OK;
        }
    }
    else {
        LOG_INFO("Not enough images to process");

        m_images_vector_mutex.unlock();

        return Status::OK;
    }
}

grpc::Status
RelocalizationAndMappingGrpcServiceImpl::Get3DTransform(grpc::ServerContext* context,
                                                        const Empty* request,
                                                        RelocalizationResult* response)
{
    LOG_INFO("Get3DTransform");

    return Status(grpc::StatusCode::UNIMPLEMENTED,
                  "Get3DTransform() is not yet implemented: \
                    request relocalization to get the latest pose");
}

grpc::Status
RelocalizationAndMappingGrpcServiceImpl::Reset(grpc::ServerContext *context,
                                               const Empty *request,
                                               Empty *response)
{
    LOG_INFO("Reset");


    if (m_pipeline->resetMap() != SolAR::FrameworkReturnCode::_SUCCESS)
    {
        return gRpcError("Error while resetting the global map for the map update service");
    }

    LOG_DEBUG("Reset global map OK");

    return Status::OK;
}

grpc::Status
RelocalizationAndMappingGrpcServiceImpl::SendMessage(grpc::ServerContext* context,
                                                     const Message* request,
                                                     Empty* response)
{
    LOG_INFO("[RelocAndMapping] message: '{}'", request->message());
    return Status::OK;
}

std::string
RelocalizationAndMappingGrpcServiceImpl::to_string(CameraType type)
{
    switch(type)
    {
    case CameraType::RGB: return "RGB";
    case CameraType::GRAY: return "GRAY";
    default: throw std::runtime_error("Unkown camera type");
    }
}

std::string
RelocalizationAndMappingGrpcServiceImpl::to_string(StereoType type)
{
    switch(type)
    {
    case StereoType::Horizontal: return "Horizontal";
    case StereoType::Vertical: return "Vertical";
    default: throw std::runtime_error("Unkown camera stereo type");
    }
}

std::string
RelocalizationAndMappingGrpcServiceImpl::to_string(Matrix3x3 mat)
{
    std::stringstream ss;
    ss << mat.m11() << ", "  << mat.m12() << ", "  << mat.m13() << std::endl
       << mat.m21() << ", "  << mat.m22() << ", "  << mat.m23() << std::endl
       << mat.m31() << ", "  << mat.m32() << ", "  << mat.m33() << std::endl;
    return ss.str();
}

std::string
RelocalizationAndMappingGrpcServiceImpl::to_string(Matrix4x4 mat)
{
    std::stringstream ss;
    ss << mat.m11() << ", " << mat.m12() << ", " <<  mat.m13() << ", " <<  mat.m14() <<std::endl
       << mat.m21() << ", " <<  mat.m22() << ", " <<  mat.m23() << ", " <<  mat.m24() <<std::endl
       << mat.m31() << ", " <<  mat.m32() << ", " <<  mat.m33() << ", " <<  mat.m34() <<std::endl
       << mat.m41() << ", " <<  mat.m42() << ", " <<  mat.m43() << ", " <<  mat.m44() <<std::endl;
    return ss.str();
}

std::string
RelocalizationAndMappingGrpcServiceImpl::to_string(Matrix3x4 mat)
{
    std::stringstream ss;
    ss << mat.m11() << ", " << mat.m12() << ", " <<  mat.m13() << ", " <<  mat.m14() <<std::endl
       << mat.m21() << ", " <<  mat.m22() << ", " <<  mat.m23() << ", " <<  mat.m24() <<std::endl
       << mat.m31() << ", " <<  mat.m32() << ", " <<  mat.m33() << ", " <<  mat.m34() <<std::endl;
    return ss.str();
}

std::string
RelocalizationAndMappingGrpcServiceImpl::to_string(ImageLayout layout)
{
    switch(layout)
    {
    case ImageLayout::RGB_24: return "RGB24";
    case ImageLayout::GREY_8: return "GREY_8";
    case ImageLayout::GREY_16: return "GREY_16";
    default: throw std::runtime_error("Unkown layout type");
    };
}

std::string
RelocalizationAndMappingGrpcServiceImpl::to_string(ImageCompression compression)
{
    switch(compression)
    {
    case ImageCompression::PNG: return "PNG";
    case ImageCompression::JPG: return "JPG";
    case ImageCompression::NONE: return "NONE";
    default: throw std::runtime_error("Unkown compression type");
    };
}

std::string
RelocalizationAndMappingGrpcServiceImpl::to_string(SolAR::api::pipeline::TransformStatus transformStatus)
{
    switch(transformStatus)
    {
    case SolAR::api::pipeline::TransformStatus::NO_3DTRANSFORM: return "NO_3DTRANSFORM";
    case SolAR::api::pipeline::TransformStatus::NEW_3DTRANSFORM: return "NEW_3DTRANSFORM";
    case SolAR::api::pipeline::TransformStatus::PREVIOUS_3DTRANSFORM: return "PREVIOUS_3DTRANSFORM";
    default: throw std::runtime_error("Unkown transform status");
    };
}

std::string
RelocalizationAndMappingGrpcServiceImpl::to_string(SolAR::api::pipeline::MappingStatus mappingStatus)
{
    switch(mappingStatus)
    {
    case SolAR::api::pipeline::MappingStatus::BOOTSTRAP: return "BOOTSTRAP";
    case SolAR::api::pipeline::MappingStatus::MAPPING: return "MAPPING";
    case SolAR::api::pipeline::MappingStatus::TRACKING_LOST: return "TRACKING_LOST";
    case SolAR::api::pipeline::MappingStatus::LOOP_CLOSURE: return "LOOP_CLOSURE";
    default: throw std::runtime_error("Unkown mapping status");
    };
}

SolAR::datastructure::CameraType
RelocalizationAndMappingGrpcServiceImpl::toSolAR(CameraType type)
{
    switch(type)
    {
    case CameraType::RGB: return SolAR::datastructure::CameraType::RGB;
    case CameraType::GRAY: return SolAR::datastructure::CameraType::Gray;
    default: throw std::runtime_error("Unkown camera type");
    }
}

SolAR::datastructure::StereoType
RelocalizationAndMappingGrpcServiceImpl::toSolAR(StereoType type)
{
    switch(type)
    {
    case StereoType::Horizontal: return SolAR::datastructure::StereoType::Horizontal;
    case StereoType::Vertical: return SolAR::datastructure::StereoType::Vertical;
    default: throw std::runtime_error("Unkown camera stereo type");
    }
}

SolAR::datastructure::Transform3Df
RelocalizationAndMappingGrpcServiceImpl::toSolAR(const Matrix4x4& gRpcPose)
{
    SolAR::datastructure::Transform3Df solARPose;
    solARPose(0,0) = gRpcPose.m11();
    solARPose(0,1) = gRpcPose.m12();
    solARPose(0,2) = gRpcPose.m13();
    solARPose(0,3) = gRpcPose.m14();
    solARPose(1,0) = gRpcPose.m21();
    solARPose(1,1) = gRpcPose.m22();
    solARPose(1,2) = gRpcPose.m23();
    solARPose(1,3) = gRpcPose.m24();
    solARPose(2,0) = gRpcPose.m31();
    solARPose(2,1) = gRpcPose.m32();
    solARPose(2,2) = gRpcPose.m33();
    solARPose(2,3) = gRpcPose.m34();
    solARPose(3,0) = gRpcPose.m41();
    solARPose(3,1) = gRpcPose.m42();
    solARPose(3,2) = gRpcPose.m43();
    solARPose(3,3) = gRpcPose.m44();

    LOG_DEBUG("==> Hololens pose = {}", solARPose.matrix());
    return solARPose;
}

void
RelocalizationAndMappingGrpcServiceImpl::toGrpc(const SolAR::datastructure::Transform3Df& solARPose,
                                                Matrix4x4& gRpcPose)
{
    gRpcPose.set_m11(solARPose(0,0));
    gRpcPose.set_m12(solARPose(0,1));
    gRpcPose.set_m13(solARPose(0,2));
    gRpcPose.set_m14(solARPose(0,3));
    gRpcPose.set_m21(solARPose(1,0));
    gRpcPose.set_m22(solARPose(1,1));
    gRpcPose.set_m23(solARPose(1,2));
    gRpcPose.set_m24(solARPose(1,3));
    gRpcPose.set_m31(solARPose(2,0));
    gRpcPose.set_m32(solARPose(2,1));
    gRpcPose.set_m33(solARPose(2,2));
    gRpcPose.set_m34(solARPose(2,3));
    gRpcPose.set_m41(solARPose(3,0));
    gRpcPose.set_m42(solARPose(3,1));
    gRpcPose.set_m43(solARPose(3,2));
    gRpcPose.set_m44(solARPose(3,3));
}

grpc::Status
RelocalizationAndMappingGrpcServiceImpl::buildSolARImage(const Frame frame,
                                                         const SolAR::datastructure::Transform3Df& solARPose,
                                                         SRef<SolAR::datastructure::Image>& image)
{
    SolAR::datastructure::Image::ImageEncoding encoding;

    // Decode image before use if needed
    switch (frame.image().imagecompression())
    {
        case ImageCompression::NONE:
        {
            encoding = SolAR::datastructure::Image::ENCODING_NONE;
            break;
        }
        case ImageCompression::JPG:
        {
            encoding = SolAR::datastructure::Image::ENCODING_JPEG;
            break;
        }
        case ImageCompression::PNG:
        {
            encoding = SolAR::datastructure::Image::ENCODING_PNG;
            break;
        }
        default:
        {
            return gRpcError("Error: unkown image compression format");
        }
    }

    switch(frame.image().layout())
    {
        case ImageLayout::RGB_24:
        {
            LOG_DEBUG("Create Image: RGB_24");

            if (encoding == SolAR::datastructure::Image::ENCODING_NONE)
            {
                // Convert to CV_8UC3 because otherwise convertToSolar() will fail
                const char* image_buffer = frame.image().data().c_str();
                std::vector<char> bgr_buffer;
                for (long j = 0; j < frame.image().data().size(); j += 4)
                {
                  bgr_buffer.push_back(image_buffer[j]);
                  bgr_buffer.push_back(image_buffer[j + 1]);
                  bgr_buffer.push_back(image_buffer[j + 2]);
                }

                image = org::bcom::xpcf::utils::make_shared<SolARImage>(
                            &bgr_buffer[0],
                            frame.image().width(),
                            frame.image().height(),
                            SolARImage::ImageLayout::LAYOUT_BGR,
                            SolARImage::PixelOrder::INTERLEAVED,
                            SolARImage::DataType::TYPE_8U);
            }
            else if (encoding == SolAR::datastructure::Image::ENCODING_PNG) {

                // Use temporary image to decode data buffer
                SRef<SolAR::datastructure::Image> temp_image =
                        org::bcom::xpcf::utils::make_shared<SolARImage>(
                            (char*)frame.image().data().c_str(),
                            frame.image().width(),
                            frame.image().height(),
                            SolARImage::ImageLayout::LAYOUT_BGR,
                            SolARImage::PixelOrder::INTERLEAVED,
                            SolARImage::DataType::TYPE_8U,
                            encoding);

                // Convert to CV_8UC3 because otherwise convertToSolar() will fail
                char* image_buffer = (char*)temp_image->data();
                std::vector<char> bgr_buffer;
                for (long j = 0; j < temp_image->getBufferSize(); j += 4)
                {
                  bgr_buffer.push_back(image_buffer[j]);
                  bgr_buffer.push_back(image_buffer[j + 1]);
                  bgr_buffer.push_back(image_buffer[j + 2]);
                }

                image = org::bcom::xpcf::utils::make_shared<SolARImage>(
                            &bgr_buffer[0],
                            temp_image->getWidth(),
                            temp_image->getHeight(),
                            SolARImage::ImageLayout::LAYOUT_BGR,
                            SolARImage::PixelOrder::INTERLEAVED,
                            SolARImage::DataType::TYPE_8U);
            }
            else if (encoding == SolAR::datastructure::Image::ENCODING_JPEG) {

                image = org::bcom::xpcf::utils::make_shared<SolARImage>(
                            (char*)frame.image().data().c_str(),
                            frame.image().width(),
                            frame.image().height(),
                            SolARImage::ImageLayout::LAYOUT_BGR,
                            SolARImage::PixelOrder::INTERLEAVED,
                            SolARImage::DataType::TYPE_8U,
                            encoding);
            }
            else {
                return gRpcError("Unkown encoding format");
            }

            break;
        }
        case ImageLayout::GREY_8:
        {
            LOG_DEBUG("Create Image: GREY_8");

            image = org::bcom::xpcf::utils::make_shared<SolARImage>(
                        (char*)frame.image().data().c_str(),
                        frame.image().width(),
                        frame.image().height(),
                        SolARImage::ImageLayout::LAYOUT_GREY,
                        SolARImage::PixelOrder::INTERLEAVED,
                        SolARImage::DataType::TYPE_8U,
                        encoding);

            break;
        }
        case ImageLayout::GREY_16:
        {
            LOG_DEBUG("Create Image: GREY_16");

            image = org::bcom::xpcf::utils::make_shared<SolARImage>(
                        (char*)frame.image().data().c_str(),
                        frame.image().width(),
                        frame.image().height(),
                        SolARImage::ImageLayout::LAYOUT_GREY,
                        SolARImage::PixelOrder::INTERLEAVED,
                        SolARImage::DataType::TYPE_16U,
                        encoding);

            break;
        }
        default:
        {
            return gRpcError("Unkown image layout");
        }
    }

    return Status::OK;
}

grpc::Status
RelocalizationAndMappingGrpcServiceImpl::toGrpc(SolAR::api::pipeline::TransformStatus solARPoseStatus,
                                                RelocalizationPoseStatus& gRpcPoseStatus)
{
    switch(solARPoseStatus)
    {
    case SolAR::api::pipeline::TransformStatus::NO_3DTRANSFORM:
    {
        gRpcPoseStatus = RelocalizationPoseStatus::NO_POSE; break;
    }
    case SolAR::api::pipeline::TransformStatus::NEW_3DTRANSFORM:
    {
        gRpcPoseStatus = RelocalizationPoseStatus::NEW_POSE; break;
    }
    case SolAR::api::pipeline::TransformStatus::PREVIOUS_3DTRANSFORM:
    {
        gRpcPoseStatus = RelocalizationPoseStatus::LATEST_POSE; break;
    }
    default:
    {
        return gRpcError("Cannot convert gRPC pose status to SolAR TransformStatus");
    }
    };

    return Status::OK;
}

grpc::Status
RelocalizationAndMappingGrpcServiceImpl::toGrpc(SolAR::api::pipeline::MappingStatus mappingStatus,
                                                MappingStatus& gRpcMappingStatus)
{
    switch(mappingStatus)
    {
    case SolAR::api::pipeline::MappingStatus::BOOTSTRAP:
    {
        gRpcMappingStatus = MappingStatus::BOOTSTRAP; break;
    }
    case SolAR::api::pipeline::MappingStatus::MAPPING:
    {
        gRpcMappingStatus = MappingStatus::MAPPING; break;
    }
    case SolAR::api::pipeline::MappingStatus::TRACKING_LOST:
    {
        gRpcMappingStatus = MappingStatus::TRACKING_LOST; break;
    }
    case SolAR::api::pipeline::MappingStatus::LOOP_CLOSURE:
    {
        gRpcMappingStatus = MappingStatus::LOOP_CLOSURE; break;
    }
    default:
    {
        return gRpcError("Cannot convert gRPC mapping status to SolAR MappingStatus");
    }
    };

    return Status::OK;
}

SolAR::api::pipeline::PipelineMode
RelocalizationAndMappingGrpcServiceImpl::toSolAR(PipelineMode pipelineMode)
{
    switch(pipelineMode)
    {
        case RELOCALIZATION_AND_MAPPING: return SolAR::api::pipeline::PipelineMode::RELOCALIZATION_AND_MAPPING;
        case RELOCALIZATION_ONLY: return SolAR::api::pipeline::PipelineMode::RELOCALIZATION_ONLY;
        default: throw std::runtime_error("Unkown pipeline mode");
    }
}

grpc::Status
RelocalizationAndMappingGrpcServiceImpl::gRpcError(std::string message,
                                                   grpc::StatusCode gRpcStatus)
{
    LOG_ERROR("{}", message);
    return Status(gRpcStatus, message);
}

void RelocalizationAndMappingGrpcServiceImpl::displayImages()
{
    std::vector<SRef<SolAR::datastructure::Image>> images;

    // Try to get next images to display
    if (!m_started || !m_sharedBufferImageToDisplay.tryPop(images)) {
        xpcf::DelegateTask::yield();
        return;
    }

    // Display images if specified
    if ((images.size() >= 1) && (m_image_viewer_left))
        m_image_viewer_left->display(images[0]);
    if ((images.size() == 2) && (m_image_viewer_right))
        m_image_viewer_right->display(images[1]);

}

void RelocalizationAndMappingGrpcServiceImpl::saveImages()
{
    std::pair<std::vector<SRef<SolARImage>>, std::vector<SolAR::datastructure::Transform3Df>> imagesPoses;
    if (!m_sharedBufferImagePoseToSave.tryPop(imagesPoses)) {
        xpcf::DelegateTask::yield();
        return;
    }

    std::vector<SRef<SolARImage>> images = imagesPoses.first;
    std::vector<SolAR::datastructure::Transform3Df> poses  = imagesPoses.second;

    char imageName[9];
    sprintf(imageName, "%0.8d", m_index_image);

    if (images.size() >= 1)
    {
        if (images.at(0)->save(m_image1_path + imageName + std::string(".jpg"))
                == SolAR::FrameworkReturnCode::_SUCCESS) {
            for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; j++)
                m_poseFile1 << poses.at(0)(i, j) << " ";
            m_poseFile1 << "\n";

            if (images.size() == 2) {
                if (images.at(1)->save(m_image2_path + imageName + std::string(".jpg"))
                        == SolAR::FrameworkReturnCode::_SUCCESS) {
                    for (int i = 0; i < 4; ++i)
                    for (int j = 0; j < 4; j++)
                        m_poseFile2 << poses.at(1)(i, j) << " ";
                    m_poseFile2 << "\n";
                }
            }
            m_timestampFile << m_last_image_timestamp << "\n";
            m_index_image++;
        }
    }
}

} // namespace com::bcom::solar::gprc
