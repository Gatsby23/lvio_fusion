#ifndef lvio_fusion_LOOP_CONSTRAINT_H
#define lvio_fusion_LOOP_CONSTRAINT_H

#include "lvio_fusion/common.h"

namespace lvio_fusion
{

class Frame;

namespace loop
{

class LoopConstraint
{
public:
    typedef std::shared_ptr<LoopConstraint> Ptr;

    std::shared_ptr<Frame> frame_old;
    SE3d relative_pose;
};

} // namespace loop
} // namespace lvio_fusion

#endif // lvio_fusion_LOOP_CONSTRAINT_H