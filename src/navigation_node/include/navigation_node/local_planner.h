#define FMT_HEADER_ONLY
#ifndef navigation_node_LOCALPLANNER_H
#define navigation_node_LOCALPLANNER_H
#include "navigation_node/common.h"
#include "navigation_node/DWA.h"
namespace navigation_node
{
 class Local_planner
    {
    public:
        typedef std::shared_ptr<Local_planner> Ptr;
        Local_planner(DWA::Ptr dwa_);
        void SetPlanPath(std::list<Vector2d> plan_path_);
        void SetRobotPose(Vector2d robot_position_,  double yaw_);
        void SetOdom(const nav_msgs::OdometryConstPtr& odom_msg);
        void SetMap(const nav_msgs::OccupancyGridConstPtr& newmap);
        void process();
        geometry_msgs::PoseStamped local_goal_msg;
        bool local_goal_updated;
        bool first=true;
        DWA::Ptr dwa;
    private:
        std::list<Vector2d> plan_path;
        SE3d robot_pose;
        double final_yaw;
        sensor_msgs::LaserScan scan;
        geometry_msgs::Twist current_velocity;
        bool robot_position_changed=false;
        double PREDICT_TIME;

    };
}// namespace navigation_node
#endif // navigation_node_LOCALPLANNER_H