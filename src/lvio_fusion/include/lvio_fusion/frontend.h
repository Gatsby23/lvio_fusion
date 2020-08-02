#ifndef lvio_fusion_FRONTEND_H
#define lvio_fusion_FRONTEND_H

#include "lvio_fusion/common.h"
#include "lvio_fusion/frame.h"
#include "lvio_fusion/map.h"
#include "lvio_fusion/sensors/camera.hpp"

namespace lvio_fusion
{

class Backend;

enum class FrontendStatus
{
    INITING,
    TRACKING_GOOD,
    TRACKING_BAD,
    TRACKING_TRY,
    LOST
};

enum Flag
{
    None = 0,
    Mono = 1,
    Stereo = 1 << 1,
    RGBD = 1 << 2,
    IMU = 1 << 3,
    Lidar = 1 << 4,
    GNSS = 1 << 5,
    RTK = 1 << 6,
    Semantic = 1 << 7,
};

class Frontend
{
public:
    typedef std::shared_ptr<Frontend> Ptr;

    Frontend(int num_features, int init, int tracking, int tracking_bad, int need_for_keyframe);

    bool AddFrame(Frame::Ptr frame);

    void SetMap(Map::Ptr map) { map_ = map; }

    void SetBackend(std::shared_ptr<Backend> backend) { backend_ = backend; }

    void SetCameras(Camera::Ptr left, Camera::Ptr right)
    {
        camera_left_ = left;
        camera_right_ = right;
    }

    void UpdateCache();

    std::unordered_map<unsigned long, Vector3d> GetPositionCache();

    int flags = Flag::None;
    FrontendStatus status = FrontendStatus::INITING;
    Frame::Ptr current_frame = nullptr;
    Frame::Ptr last_frame = nullptr;
    SE3d relative_motion;
    std::mutex last_frame_mutex;

private:
    bool Track();

    bool Reset();

    int TrackLastFrame();

    bool InitFramePoseByPnP();

    void CreateKeyframe();

    bool StereoInit();

    int DetectNewFeatures();

    int TriangulateNewPoints();

    // data
    Map::Ptr map_ = nullptr;
    std::shared_ptr<Backend> backend_ = nullptr;
    Camera::Ptr camera_left_ = nullptr;
    Camera::Ptr camera_right_ = nullptr;
    std::unordered_map<unsigned long, Vector3d> position_cache_;
    SE3d last_frame_pose_cache_;

    // params
    int num_features_;
    int num_features_init_;
    int num_features_tracking_;
    int num_features_tracking_bad_;
    int num_features_needed_for_keyframe_;
};

} // namespace lvio_fusion

#endif // lvio_fusion_FRONTEND_H
