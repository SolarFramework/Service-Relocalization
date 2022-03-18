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
    Fps(uint computePeriodMs, uint windowSize)
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
  uint m_windowSize{10};
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
{}

RelocalizationAndMappingGrpcServiceImpl::RelocalizationAndMappingGrpcServiceImpl(
        SolAR::api::pipeline::IAsyncRelocalizationPipeline* pipeline,
        std::string saveFolder, SRef<SolAR::api::display::IImageViewer> image_viewer):
            m_pipeline{ pipeline }, m_file_path { saveFolder }, m_image_viewer {image_viewer}
{}

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

    m_ordered_images.clear();
    m_last_image_timestamp = 0;

    m_index_image = 0;
    if (m_file_path != "") {
        m_image_path = m_file_path + "/000/";
        boost::filesystem::create_directories(boost::filesystem::path(m_image_path.c_str()));
        m_poseFile.open(m_file_path + "/pose_000.txt");
        m_timestampFile.open(m_file_path + "/timestamps.txt");
    }

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
        m_poseFile.close();
        m_timestampFile.close();
    }

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

bool RelocalizationAndMappingGrpcServiceImpl::sortbythird (
        const std::tuple<SRef<SolAR::datastructure::Image>, SolAR::datastructure::Transform3Df, long> a,
        const std::tuple<SRef<SolAR::datastructure::Image>, SolAR::datastructure::Transform3Df, long> b)
{
    return (std::get<2>(a) < std::get<2>(b));
}

grpc::Status
RelocalizationAndMappingGrpcServiceImpl::RelocalizeAndMap(grpc::ServerContext* context,
                                                          const Frame* request,
                                                          RelocalizationResult* response)
{

    if (!m_started) {
        LOG_INFO("Proxy is not started");
        return Status::OK;
    }

    auto fps = relocAndMapFps.update();

    LOG_DEBUG("{:03.2f} FPS", fps);

//    LOG_DEBUG("  image: {}x{}, {}",
//              request->image().width(),
//              request->image().height(),
//              to_string(request->image().layout()));
//    LOG_DEBUG("  pose:\n{}", to_string(request->pose()));
//    LOG_DEBUG("  timestamp: {}", request->timestamp());

    // Get data from request
    SRef<SolARImage> image;
    auto status  = buildSolARImage(request, toSolAR(request->pose()), image);
    if (!status.ok())
    {
        LOG_ERROR("Error while converting received image to SolAR datastructure");
        return status;
    }

    SolAR::datastructure::Transform3Df pose = toSolAR(request->pose());
    long timestamp = request->timestamp();

    SolAR::api::pipeline::TransformStatus transform3DStatus;
    SolAR::datastructure::Transform3Df transform3D;
    float_t confidence;

    // Drop image if too old (older than last processed image)
    if (timestamp < m_last_image_timestamp) {
        LOG_INFO("Image too old: drop it!");

        response->set_confidence(0);
        response->set_pose_status(RelocalizationPoseStatus::NO_POSE);

        return Status::OK;
    }

    m_images_vector_mutex.lock();

    // Add data to vector of tuple (timestamp, image, pose)
    m_ordered_images.push_back(std::make_tuple(image, pose, timestamp));

    // Sort vector based on timestamps
    std::sort(m_ordered_images.begin(), m_ordered_images.end(), sortbythird);

    // If enough tuples, send the older one to Front End
    if (m_ordered_images.size() >= 5) {

        SRef<SolARImage> imageToSend = std::get<0>(m_ordered_images[0]);
        SolAR::datastructure::Transform3Df poseToSend = std::get<1>(m_ordered_images[0]);
        m_last_image_timestamp = std::get<2>(m_ordered_images[0]);

        try {
            m_pipeline->relocalizeProcessRequest(
                        imageToSend,
                        poseToSend,
                        std::chrono::time_point<std::chrono::system_clock>(
                            std::chrono::milliseconds(m_last_image_timestamp)),
                        transform3DStatus,
                        transform3D,
                        confidence);            
        }
        catch (const std::exception& e)
        {
            m_images_vector_mutex.unlock();

            return gRpcError("Error: exception thrown by relocation and mapping pipeline: "
                             + std::string(e.what()));
        }

        // Display image if specified
        if (m_image_viewer)
            m_image_viewer->display(imageToSend);

        // Save images and poses on files if specified
        if (m_file_path != "") {
            char imageName[9];
            sprintf(imageName, "%0.8d", m_index_image);
            cv::Mat imageToSave;
            imageToOpenCV(imageToSend, imageToSave);
            if (cv::imwrite(m_image_path + imageName + std::string(".jpg"), imageToSave)) {
                m_index_image++;
                for (int i = 0; i < 4; ++i)
                for (int j = 0; j < 4; j++)
                    m_poseFile << poseToSend(i, j) << " ";
                m_poseFile << "\n";
                m_timestampFile << m_last_image_timestamp << "\n";
            }
        }

        // Remove the older tuple from vector
        m_ordered_images.erase(m_ordered_images.begin());

        m_images_vector_mutex.unlock();

        RelocalizationPoseStatus gRpcPoseStatus;
        status = toGrpc(transform3DStatus, gRpcPoseStatus);
        if (!status.ok())
        {
            LOG_ERROR("RelocalizeAndMap(): error while converting received image to SoLAR datastructure");
            return status;
        }

        response->set_confidence(confidence);
        response->set_pose_status(gRpcPoseStatus);
        toGrpc(transform3D, *response->mutable_pose());

        LOG_DEBUG("Output");
        LOG_DEBUG("  confidence: {}", confidence);
        LOG_DEBUG("  transform status: {}", to_string(transform3DStatus));
        LOG_DEBUG("  transform:\n{}", transform3D.matrix());

        return Status::OK;
    }
    else {
        LOG_INFO("Not enough images to process");

        m_images_vector_mutex.unlock();

        response->set_confidence(0);
        response->set_pose_status(RelocalizationPoseStatus::NO_POSE);

        return Status::OK;
    }

    return Status::OK;
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
RelocalizationAndMappingGrpcServiceImpl::to_string(Matrix3x3 mat)
{
    std::stringstream ss;
    ss << mat.m11() << mat.m12() << mat.m13() << std::endl
       << mat.m21() << mat.m22() << mat.m23() << std::endl
       << mat.m31() << mat.m32() << mat.m33() << std::endl;
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
RelocalizationAndMappingGrpcServiceImpl::buildSolARImage(const Frame* frame,
                                                         const SolAR::datastructure::Transform3Df& solARPose,
                                                         SRef<SolAR::datastructure::Image>& image)
{
    switch(frame->image().imagecompression())
    {
        case ImageCompression::NONE:
        {
            int image_layout = -1;
            std::string image_layout_string;
            switch(frame->image().layout())
            {
                case ImageLayout::RGB_24:
                {
                    image_layout = CV_8UC4;
                    image_layout_string = "RGB_24";
                    break;
                }
                case ImageLayout::GREY_8:
                {
                    image_layout = CV_8UC1;
                    image_layout_string = "GREY_8";
                    break;
                }
                case ImageLayout::GREY_16:
                {
                    image_layout = CV_16UC1;
                    image_layout_string = "GREY_16";
                    break;
                }
                default:
                {
                    return gRpcError("Unkown image layout");
                }
            }

            cv::Mat ocvImg;

            if ( image_layout == CV_8UC4 )
            {
                // Convert to CV_8UC3 because otherwise convertToSolar() will fail
                const char* bgra_buffer = frame->image().data().c_str();
                long destBufferSize = frame->image().data().size() - (frame->image().data().size() / 4);
                char bgr_buffer[destBufferSize];
                for (long j = 0, k = 0; j < frame->image().data().size(); j += 4, k += 3)
                {
                  bgr_buffer[k] = bgra_buffer[j];
                  bgr_buffer[k + 1] = bgra_buffer[j + 1];
                  bgr_buffer[k + 2] = bgra_buffer[j + 2];
                }
                ocvImg.create(
                            static_cast<int>(frame->image().height()),
                            static_cast<int>(frame->image().width()),
                            CV_8UC3);
                memcpy(ocvImg.data, bgr_buffer, ocvImg.rows * ocvImg.cols * 3);
            }
            else
            {
                ocvImg.create(
                  static_cast<int>(frame->image().height()),
                  static_cast<int>(frame->image().width()),
                  image_layout
                  );
                memcpy(ocvImg.data,
                       const_cast<void*>(static_cast<const void*>(frame->image().data().c_str())),
                       ocvImg.rows * ocvImg.cols * (image_layout == CV_8UC1 ? 1 : 2 ));
            }

            return toSolAR(ocvImg, image);
        }

        case ImageCompression::PNG:
        {
            // Decode PNG image

            // Copy PNG image buffer
            std::vector<uchar> decodingBuffer(frame->image().data().c_str(),
                                              frame->image().data().c_str() + frame->image().data().size());
            cv::Mat imageDecoded;

            // Decode PNG image
            switch(frame->image().layout())
            {
                case ImageLayout::RGB_24:
                {
                    imageDecoded = cv::imdecode(decodingBuffer, cv::IMREAD_COLOR);
                    break;
                }
                case ImageLayout::GREY_8:
                {
                    imageDecoded = cv::imdecode(decodingBuffer, cv::IMREAD_GRAYSCALE);
                    break;
                }
                case ImageLayout::GREY_16:
                {
                    imageDecoded = cv::imdecode(decodingBuffer, cv::IMREAD_GRAYSCALE);
                    break;
                }
                default:
                {
                    return gRpcError("Unkown image layout");
                }
            }

            return toSolAR(imageDecoded, image);
        }

        default:
        {
            return gRpcError("Error: unkown image compression format");
        }
    }
}

grpc::Status
RelocalizationAndMappingGrpcServiceImpl::toSolAR(/* const */ cv::Mat& imgSrc,
                                                 SRef<SolARImage>& image)
{
    assert(imgSrc.type() == CV_8UC3 || imgSrc.type() == CV_8UC1 || imgSrc.type() == CV_16UC1);

    SolARImage::ImageLayout layout;
    SolARImage::DataType dataType;
    switch(imgSrc.type())
    {
    case CV_8UC3:
    {
        layout = SolARImage::ImageLayout::LAYOUT_BGR;
        dataType = SolARImage::DataType::TYPE_8U;
        break;
    }
    case CV_8UC1:
    {
        layout = SolARImage::ImageLayout::LAYOUT_GREY;
        dataType = SolARImage::DataType::TYPE_8U;
        break;
    }
    case CV_16UC1:
    {
        layout = SolARImage::ImageLayout::LAYOUT_GREY;
        dataType = SolARImage::DataType::TYPE_16U;
        break;
    }
    default:
    {
        return gRpcError("Cannot convert OpenCV image type to SolAR");
    }
    };

    image = org::bcom::xpcf::utils::make_shared<SolARImage>(
                imgSrc.ptr(),
                imgSrc.cols,
                imgSrc.rows,
                layout,
                SolARImage::PixelOrder::INTERLEAVED,
                dataType);

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

void RelocalizationAndMappingGrpcServiceImpl::imageToOpenCV(SRef<SolAR::datastructure::Image> imgSrc, cv::Mat& imgDest)
{
    static std::map<std::tuple<uint32_t,std::size_t,uint32_t>,int> solar2cvTypeConvertMap = {
        {std::make_tuple(8,1,3),CV_8UC3},
        {std::make_tuple(8,1,1),CV_8UC1},
        {std::make_tuple(16,1,1), CV_16UC1}
    };
    int type = solar2cvTypeConvertMap.at(std::forward_as_tuple(imgSrc->getNbBitsPerComponent(),1,imgSrc->getNbChannels()));
    cv::Mat imgCV(imgSrc->getHeight(),imgSrc->getWidth(),type, imgSrc->data());
    imgDest = imgCV;
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

} // namespace com::bcom::solar::gprc
