#ifndef lvio_fusion_INITIALIZER_H
#define lvio_fusion_INITIALIZER_H

#include "lvio_fusion/common.h"
#include "lvio_fusion/imu/imu.h"
#include "lvio_fusion/map.h"
#include "lvio_fusion/navigation/gridmap.h"//NAVI

namespace lvio_fusion
{

class Initializer
{
public:
    typedef std::shared_ptr<Initializer> Ptr;

    bool EstimateVelAndRwg(std::vector<Frame::Ptr> keyframes);

    bool Initialize(Frames frames, double priorA = 1e6, double priorG = 1e2);
    //NAVI
    void SetGridmap(Gridmap::Ptr gridmap){gridmap_ = gridmap;}

    bool first_init = false;      // finished first init?
    bool need_reinit = false;    // need re-init?
    const int num_frames = 10;
private:
    Gridmap::Ptr gridmap_;//NAVI
    Matrix3d Rwg_; //重力方向
};

} // namespace lvio_fusion
#endif // lvio_fusion_INITIALIZER_H
