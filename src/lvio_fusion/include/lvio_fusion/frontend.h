#ifndef lvio_fusion_FRONTEND_H
#define lvio_fusion_FRONTEND_H

#include "lvio_fusion/common.h"
#include "lvio_fusion/visual/local_map.h"
#include "lvio_fusion/visual/matcher.h"
#include "lvio_fusion/navigation/global_planner.h"//NAVI
namespace lvio_fusion
{

class Backend;

enum class FrontendStatus
{
    BUILDING,
    INITIALIZING,
    TRACKING_GOOD,
    TRACKING_TRY,
    LOST
};

class Frontend
{
public:
    typedef std::shared_ptr<Frontend> Ptr;

    Frontend(int num_features, int init, int tracking, int tracking_bad, int need_for_keyframe);

    bool AddFrame(Frame::Ptr frame);

    void AddImu(double time, Vector3d acc, Vector3d gyr);

    void SetBackend(std::shared_ptr<Backend> backend) { backend_ = backend; }

    void SetGlobalPlanner(Global_planner::Ptr globalplanner ) { globalplanner_ = globalplanner; }//NAVI

    void UpdateCache();

    void UpdateIMU(const Bias &bias_);

    FrontendStatus status = FrontendStatus::BUILDING;
    Frame::Ptr current_frame;
    Frame::Ptr last_frame;
    Frame::Ptr last_keyframe;
    SE3d relative_i_j;
    LocalMap local_map;
    std::mutex mutex;
    double valid_imu_time = 0;
    bool last_keyframe_updated = false;

private:
    bool Track();

    bool Reset();

    void InitFrame();

    int TrackLastFrame();

    int Relocate(Frame::Ptr base_frame);

    void CreateKeyframe();

    bool InitMap();

    int DetectNewFeatures();

    int TriangulateNewPoints();

    void PreintegrateIMU();

    void PredictStateIMU();

    // data
    std::weak_ptr<Backend> backend_;
    Global_planner::Ptr globalplanner_;//NAVI
    SE3d last_frame_pose_cache_;
    std::queue<ImuData> imu_buf_;
    imu::Preintegration::Ptr imu_preintegrated_from_last_kf_;

    // params
    int num_features_init_;
    int num_features_tracking_bad_;
    int num_features_needed_for_keyframe_;
};

} // namespace lvio_fusion

#endif // lvio_fusion_FRONTEND_H
