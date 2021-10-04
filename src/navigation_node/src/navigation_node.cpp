#include "navigation_node/navigation_node.h"

void nav_goal_callback(const geometry_msgs::PoseStamped  &nav_goal_msg)
{
    //LOG(INFO)<<nav_goal_msg.pose.position.x<<" "<<nav_goal_msg.pose.position.y;
    global_planner->SetGoalPose(Vector2d(nav_goal_msg.pose.position.x/GRID_RESOLUTION,nav_goal_msg.pose.position.y/GRID_RESOLUTION));
    if(use_obstacle_avoidance)
        local_planner->first=true;
}

void pose_callback(const geometry_msgs::PoseStamped  &pose_msg)
{
    Vector3d pose3d(pose_msg.pose.position.x,pose_msg.pose.position.y,pose_msg.pose.position.z);
    Vector2d pose2d(pose3d[0],pose3d[1]);
    global_planner->SetRobotPose(pose2d);
    Quaterniond q;
    q.w()=pose_msg.pose.orientation.w;
    q.x()=pose_msg.pose.orientation.x;
    q.y()=pose_msg.pose.orientation.y;
    q.z()=pose_msg.pose.orientation.z;
    double yaw = q.toRotationMatrix().eulerAngles(2,1,0)(0);
    if(use_obstacle_avoidance)
        local_planner->SetRobotPose(pose2d,yaw);
}

void border_callback(const std_msgs::Float64MultiArray::ConstPtr& border)
{
    max_x=int(border->data.at(0));
    max_y=int(border->data.at(2));
    min_x=int(border->data.at(1));
    min_y=int(border->data.at(3));
    //LOG(INFO)<<max_x<<" "<< max_y<<" "<<min_x<<" "<<min_y;
}

void gridmap_callback(const nav_msgs::OccupancyGrid  &gridmap_msg)
{
    cv::Mat map(gridmap_msg.info.width+5,gridmap_msg.info.height+5,CV_32FC1);
    map.setTo(-1);
    for(int row = 0; row<gridmap_msg.info.width;row++)
    {
        for(int col =0; col<gridmap_msg.info.height; col++)
        {
            float val=float(gridmap_msg.data[row*gridmap_msg.info.width+col]);
            if(val!=-1)
            {
                map.at<float>(row,col)=1-val/100;
            }
        }
    }

    global_planner->SetNewMap(map, max_x, max_y, min_x, min_y);
}

void localmap_callbcak(const nav_msgs::OccupancyGrid  &localmap_msg)
{
    nav_msgs::OccupancyGridConstPtr msg(new nav_msgs::OccupancyGrid(localmap_msg));
    local_planner->SetMap(msg);
}

void odom_callbcak(const nav_msgs::Odometry& odom_msg)
{
    nav_msgs::OdometryConstPtr msg(new nav_msgs::Odometry(odom_msg));
    local_planner->SetOdom(msg);
}

void plan_path_timer_callback(const ros::TimerEvent &timer_event)
{
    if(global_planner->pathupdated)
    {
        plan_path.poses.clear();
        list<Vector2d> plan_path_ = global_planner->GetPath();
        if(use_obstacle_avoidance)
            local_planner->SetPlanPath(plan_path_);
        for( auto point: plan_path_)
        {
            geometry_msgs::PoseStamped pose_stamped;
            pose_stamped.header.stamp = ros::Time(timer_event.current_real.toSec());
            pose_stamped.header.frame_id = "navigation";
            pose_stamped.pose.position.x = point.x()*GRID_RESOLUTION;
            pose_stamped.pose.position.y = point.y()*GRID_RESOLUTION;
            pose_stamped.pose.position.z = 0;
            plan_path.poses.push_back(pose_stamped);
        }
        plan_path.header.stamp = ros::Time(timer_event.current_real.toSec());
        plan_path.header.frame_id = "navigation";
        //LOG(INFO)<<"plan_path_: "<<plan_path_.size();
        pub_plan_path.publish(plan_path);
    }
}

void control_vel_timer_callback(const ros::TimerEvent &timer_event)
{
    if(local_planner->local_goal_updated)
    {
        geometry_msgs::PoseStamped local_goal;
        local_goal.pose=local_planner->local_goal_msg.pose;
        local_goal.header.frame_id="navigation";
        local_goal.header.stamp = ros::Time(timer_event.current_real.toSec());
        pub_local_goal.publish(local_goal);
        local_planner->local_goal_updated=false;
        //LOG(INFO)<<local_goal.pose.position.x<<" "<<local_goal.pose.position.y<<" "<<local_goal.pose.position.z;
    }
}

void local_goal_process()
{
    LOG(INFO)<<"process222";
    local_planner->process();
}

void dwa_process()
{
    local_planner->dwa->process();
}

// void navi_process()
// {
    
// }

int main(int argc, char **argv)
{
    ros::init(argc, argv, "navigation_node");
    ros::NodeHandle n("~");
    ros::console::set_logger_level(ROSCONSOLE_DEFAULT_NAME, ros::console::levels::Info);
    //read_parameters
    string config_file = "read_config_path_is_not_correct";
    if (argc > 1)
    {
        if (argc != 2)
        {
            printf("Please intput: rosrun navigation_node navigation_node [config file] \n"
                   "for example: rosrun navigation_node navigation_node "
                   "~/Projects/lvio-fusion/src/navigation_node/config/kitti.yaml \n");
            return 1;
        }

        config_file = argv[1];
    }
    else
    {
        if (!n.getParam("config_file", config_file))
        {
            ROS_ERROR("Error: %s", config_file.c_str());
            return 1;
        }
        ROS_INFO("Load config_file: %s", config_file.c_str());
    }
    FILE *f = fopen(config_file.c_str(), "r");
    if (f == NULL)
    {
        ROS_WARN("config_file dosen't exist; wrong config_file path");
        ROS_BREAK();
        return 0;
    }
    fclose(f);

    cv::FileStorage settings(config_file, cv::FileStorage::READ);
    if (!settings.isOpened())
    {
        cerr << "ERROR: Wrong path to settings" << endl;
    }
    settings["use_navigation"] >> use_navigation;
    if(use_navigation)
    {
        settings["grid_width"] >> GRID_WIDTH;
        settings["grid_height"] >> GRID_HEIGHT;
        settings["grid_resolution"]>> GRID_RESOLUTION;

        settings["nav_goal_topic"] >> NAV_GOAL_TOPIC;
        settings["pose_topic"] >> POSE_TOPIC;
        settings["border_topic"]>> BORDER_TOPIC;
        settings["gridmap_topic"] >> GRIDMAP_TOPIC;
        settings["use_obstacle_avoidance"] >> use_obstacle_avoidance;
        if(use_obstacle_avoidance)
        {
            settings["MAX_VELOCITY"] >> MAX_VELOCITY;
            settings["MIN_VELOCITY"] >> MIN_VELOCITY;            
            settings["MAX_YAWRATE"] >> MAX_YAWRATE;
            settings["MAX_ACCELERATION"] >> MAX_ACCELERATION;
            settings["MAX_D_YAWRATE"] >> MAX_D_YAWRATE;
            settings["TARGET_VELOCITY"] >> TARGET_VELOCITY;
            settings["MAX_DIST"] >> MAX_DIST;
            settings["VELOCITY_RESOLUTION"] >> VELOCITY_RESOLUTION;
            settings["YAWRATE_RESOLUTION"] >> YAWRATE_RESOLUTION;
            settings["ANGLE_RESOLUTION"] >> ANGLE_RESOLUTION;
            settings["PREDICT_TIME"] >> PREDICT_TIME;
            settings["TO_GOAL_COST_GAIN"] >> TO_GOAL_COST_GAIN;
            settings["SPEED_COST_GAIN"] >> SPEED_COST_GAIN;
            settings["OBSTACLE_COST_GAIN"] >> OBSTACLE_COST_GAIN;
            settings["HZ"] >> HZ;
            settings["GOAL_THRESHOLD"] >> GOAL_THRESHOLD;
            settings["TURN_DIRECTION_THRESHOLD"] >> TURN_DIRECTION_THRESHOLD;  

            settings["odom_topic"] >> ODOM_TOPIC;
            settings["localmap_topic"] >> LOCALMAP_TOPIC;
        }
    }
    settings.release();
    ros::Timer plan_path_timer;
    ros::Timer control_vel_timer;

    //set sub and pub
    if(use_navigation)
    {
        global_planner = navigation_node::Global_planner::Ptr(new navigation_node::Global_planner(
            GRID_WIDTH,
            GRID_HEIGHT,
            GRID_RESOLUTION));
        cout << "nav_goal:" << NAV_GOAL_TOPIC << endl;
        sub_nav_goal = n.subscribe(NAV_GOAL_TOPIC, 100, nav_goal_callback);
        cout << "robot_pose:" << POSE_TOPIC << endl;
        sub_pose = n.subscribe(POSE_TOPIC,1,pose_callback);
        cout << "border_pose:" << BORDER_TOPIC << endl;
        sub_border = n.subscribe(BORDER_TOPIC,1,border_callback);
        cout << "gridmap:" << GRIDMAP_TOPIC << endl;
        sub_gridmap = n.subscribe(GRIDMAP_TOPIC,1,gridmap_callback);
        pub_plan_path = n.advertise<nav_msgs::Path>("plan_path", 10);
        plan_path_timer = n.createTimer(ros::Duration(1),plan_path_timer_callback);
        if(use_obstacle_avoidance)
        {
            pub_control_vel = n.advertise<geometry_msgs::Twist>("control_vel", 100);
            pub_candidate_trajectories = n.advertise<visualization_msgs::MarkerArray>("candidate_trajectories", 100);
            pub_selected_trajectory = n.advertise<visualization_msgs::Marker>("selected_trajectory", 100);

            navigation_node::DWA::Ptr dwa=navigation_node::DWA::Ptr(new navigation_node::DWA(0.0,0.0,0.0,0.0,
                            0.0,0.0,0.0,0.0,
                            0.0,0.0,0.0,0.0,
                            0.0,0.0,0.0,0.0,0.0,pub_control_vel,pub_candidate_trajectories,pub_selected_trajectory));
            local_planner = navigation_node::Local_planner::Ptr(new navigation_node::Local_planner(dwa));
            cout << "odom:" << ODOM_TOPIC << endl;
            sub_odom = n.subscribe(ODOM_TOPIC,10,odom_callbcak);
            cout << "localmap:" << LOCALMAP_TOPIC << endl;
            sub_localmap = n.subscribe(LOCALMAP_TOPIC,10,localmap_callbcak);
            pub_local_goal = n.advertise<geometry_msgs::PoseStamped>("local_goal", 100);
            
            control_vel_timer = n.createTimer(ros::Duration(0.2),control_vel_timer_callback);
        }
    }
    
    // thread navi_thread{navi_process};
    ros::spin();

    if(use_navigation)
    {
        if(use_obstacle_avoidance)
        {
            thread local_goal_thread{local_goal_process};
            thread dwa_thread{dwa_process};

            local_goal_thread.join();
            dwa_thread.join();
        }
    }
    return 0;
}
