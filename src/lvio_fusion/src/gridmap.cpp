#include "lvio_fusion/navigation/gridmap.h"

#include <pcl/filters/extract_indices.h>
#include <pcl/filters/radius_outlier_removal.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/kdtree/kdtree_flann.h>
#include <pcl/sample_consensus/method_types.h>
#include <pcl/sample_consensus/model_types.h>
#include <pcl/segmentation/sac_segmentation.h>

namespace lvio_fusion
{

void Gridmap::AddScan( PointICloud scan_msg)
{
    CartesianCoordinates(scan_msg,current_frame);
    curr_scan_msg=scan_msg;
    start=true;
}

void Gridmap::AddFrame(Frame::Ptr& frame)
{
    current_frame=frame;
}
  
cv::Mat Gridmap::GetLocalGridmap()    
{
    LOG(INFO)<<"BuildLocalmap";
    cv::Mat local_visual_counter;
    cv::Mat local_occupied_counter;
    local_visual_counter.create(10/resolution, 10/resolution, CV_32SC1);
    local_occupied_counter.create(10/resolution, 10/resolution, CV_32SC1);
    local_visual_counter.setTo(0);
    local_occupied_counter.setTo(0);

    for(int i = 0; i < curr_scan_msg.size(); ++i) {
        std::vector<Eigen::Vector2i> points;
        Bresenhamline(5/resolution,  5/resolution,  curr_scan_msg[i].x/resolution+5/resolution,  curr_scan_msg[i].y/resolution+5/resolution, points);
        int n = points.size();
        //LOG(INFO)<<"n: "<<n;
        if(n == 0) {
            continue;
         }
        for(int j = 0; j < n - 1; ++j) 
        {
            Vector2i index(points[j][0], points[j][1]);
            if((index[0]>=10/resolution||index[0]<0||index[1]>=10/resolution||index[1]<0))
                continue;
            local_visual_counter.at<int>(index[0],index[1])++; 
        }
        Vector2i index(points[n - 1][0], points[n - 1][1]);
        if(index[0]>=10/resolution||index[0]<0||index[1]>=10/resolution||index[1]<0)
            continue;
        local_occupied_counter.at<int>(index[0],index[1])++;
    }
    local_grid_map_int.setTo(-1);
    for (int row = 0; row < 10/resolution; ++row)
	{
		for (int col = 0; col < 10/resolution; ++col)
        {
            int visits = local_visual_counter.at<int>(row, col);
			int occupieds =  local_occupied_counter.at<int>(row, col);
            if((occupieds+ visits)>0)
            {
                if(occupieds>0)
                    local_grid_map_int.at<char>(row, col) =  100;
                else
                    local_grid_map_int.at<char>(row, col) =  0;
            }
        }
    }
    Vector3d eulerAngle = current_frame->pose.rotationMatrix().eulerAngles(2,1,0);
    AngleAxisd rollAngle(AngleAxisd(0,Vector3d::UnitX()));
    AngleAxisd pitchAngle(AngleAxisd(0,Vector3d::UnitY()));
    AngleAxisd yawAngle(AngleAxisd(eulerAngle(0),Vector3d::UnitZ()));
    Quaterniond quaternion;
    quaternion=yawAngle*pitchAngle*rollAngle;
    orientation=quaternion;
    current_pose=Vector2d(current_frame->pose.translation()[0],current_frame->pose.translation()[1]);

    return local_grid_map_int;
}


void Gridmap::CartesianCoordinates(PointICloud scan_msg,Frame::Ptr& frame)
{
    std::vector<Vector2d> scan_points;
    for(int i = 0; i < scan_msg.size(); ++i) {
        Vector3d point( scan_msg[i].x,scan_msg[i].y,scan_msg[i].z);
        Vector3d trans_point=frame->pose.rotationMatrix()* point+frame->pose.translation();
        scan_points.emplace_back(Vector2d(trans_point[0]/resolution,trans_point[1]/resolution));
    }
    LaserScan::Ptr laser_scan=LaserScan::Ptr(new LaserScan(frame,scan_points));
    laser_scans_2d.emplace_back(laser_scan);
    LOG(INFO)<<"laser_scans_2d "<<laser_scans_2d.size();
}

cv::Mat Gridmap::GetGridmap()    
{
    //ClearMap();
    int max_x=0,max_y=0,min_x=height,min_y=width;
    for(const LaserScan::Ptr&  scan : laser_scans_2d) {
        Vector3d trans_pose =scan->GetFrame()->pose.translation();
        Vector2d start =Vector2d(trans_pose[0]/resolution,trans_pose[1]/resolution);
        for(Vector2d end : scan->GetPoints()) {
            std::vector<Eigen::Vector2i> points;
            Bresenhamline(start[0],  start[1], end[0], end[1], points);

            int n = points.size();
            if(n == 0) {
                continue;
            }
    
            for(int j = 0; j < n - 1; ++j) {
                Vector2i index = GetIndex(points[j][0], points[j][1]);
                visual_counter.at<int>(index[0],index[1])++;
            }
            Vector2i index = GetIndex(points[n - 1][0], points[n - 1][1]);
            occupied_counter.at<int>(index[0],index[1])++;
            if(index[0]>max_x)max_x=index[0]+1;
            if(index[0]<min_x)min_x=index[0];
            if(index[1]>max_y)max_y=index[1]+1;
            if(index[1]<min_y)min_y=index[1];
        }
    }
    laser_scans_2d.clear();
	for (int row = min_x; row < max_x; ++row)
	{
		for (int col = min_y; col < max_y; ++col)
        {
            int visits = visual_counter.at<int>(row, col);
			int occupieds = occupied_counter.at<int>(row, col);
            if((occupieds+ visits)<=0)
            {
                grid_map.at<float>(row, col) =-1.0;
            }
            else
            {
                grid_map.at<float>(row, col) = 1.0 - ((1.0 * occupieds )/(occupieds+ visits));
                grid_map_int.at<char>(row, col) = (1 - grid_map.at<float>(row, col)) * 100;
            }
        }
    }
    border={double(max_x),double(min_x),double(max_y),double(min_y)};
    return grid_map_int;
}

Vector2i  Gridmap::GetIndex(int x, int y)
{
    Vector2i index(x+height/2,y+width/2);
    return index;
}


void Gridmap::Bresenhamline (double x1,double y1,double x2,double y2, std::vector<Eigen::Vector2i>& points)
{
    double  dx, dy,  p, temp;
    int x, y, s1, s2, interchange=0, i;
    x=(x1+0.5)/1;
    y=(y1+0.5)/1;
 
    dx=abs(x2-x1);
    dy=abs(y2-y1);

    s1=x2>x1?1:-1;
    s2=y2>y1?1:-1;

    if(dy>dx)
    {
        temp=dx;
        dx=dy;
        dy=temp;
        interchange=1;
    }

    p=2*dy-dx;
    for(i=1;i<=dx;i++)
    {
        points.push_back(Vector2i(y,x));
        if(p>=0)
        {
            if(interchange==0)
                y=y+s2;
            else
                x=x+s1;
            p=p-2*dx;
        }   
        if(interchange==0)
            x=x+s1; 
        else
            y=y+s2;
        p=p+2*dy;
    }
}

} // namespace lvio_fusion