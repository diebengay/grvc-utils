//----------------------------------------------------------------------------------------------------------------------
// Fixed wing lib
//----------------------------------------------------------------------------------------------------------------------
// The MIT License (MIT)
// 
// Copyright (c) 2020 GRVC University of Seville
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
// documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
// Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
// OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//----------------------------------------------------------------------------------------------------------------------
#ifndef FIXED_WING_LIB_H
#define FIXED_WING_LIB_H

#include <ros/ros.h>
#include <thread>
#include <atomic>
#include <vector>
#include <utility>
#include <Eigen/Core>

#include <mavros_msgs/CommandBool.h>
#include <mavros_msgs/SetMode.h>
#include <mavros_msgs/State.h>
#include <mavros_msgs/ExtendedState.h>
#include <mavros_msgs/GlobalPositionTarget.h>
#include <mavros_msgs/WaypointList.h>
#include <mavros_msgs/Waypoint.h>
#include <geographic_msgs/GeoPoint.h>
#include <geographic_msgs/GeoPoseStamped.h>
#include <sensor_msgs/NavSatFix.h>
#include <geometry_msgs/PoseStamped.h>
#include <geometry_msgs/TwistStamped.h>
#include <geometry_msgs/TransformStamped.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_ros/static_transform_broadcaster.h>

namespace grvc { namespace fw_ns {

typedef geometry_msgs::PoseStamped      Pose;
typedef geometry_msgs::PoseStamped      Waypoint;
typedef sensor_msgs::NavSatFix          WaypointGeo;
typedef geometry_msgs::TwistStamped     Velocity;

enum State {
    UNINITIALIZED,
    LANDED_DISARMED,
    LANDED_ARMED,
    TAKING_OFF,
    FLYING_AUTO,
    FLYING_MANUAL,
    LANDING,
};

enum MissionElementType {       // Possible mission waypoint types
    TAKEOFF_POSE,
    TAKEOFF_AUX,
    PASS,
    LOITER_UNLIMITED,
    LOITER_TURNS,
    LOITER_TIME,
    LOITER_HEIGHT,
    LAND_POSE,
    LAND_AUX,
};

struct MissionElement {
    std::vector<geometry_msgs::PoseStamped> waypoints;
    std::vector< std::pair<std::string,float> > params;     // name - value
    MissionElementType type;
};

class FixedWing {

public:
    FixedWing();
    ~FixedWing();

    /// Library is initialized and ready to run tasks?
    bool isReady() const;
    /// Latest pose estimation of the robot
    Pose pose();
    /// Latest velocity estimation of the robot
    Velocity velocity() const;

    /// Go to the specified waypoint, following a straight line
    /// \param _wp goal waypoint
    void	goToWaypoint(const Waypoint& _wp);

    /// Go to the specified waypoint in geographic coordinates, following a straight line
    /// \param _wp goal waypoint in geographic coordinates
    void	goToWaypointGeo(const WaypointGeo& _wp);

    /// Perform a take off maneuver
    /// \param _height target height that must be reached to consider the take off complete
    void    takeOff(double _height);
    /// Land on the current position.
    void	land();
    /// Execute mission of a sequence of waypoints
    void	setMission(const std::vector<MissionElement>& _waypoint_element_list);
    /// Set velocities
    /// Recover from manual flight mode
    /// Use it when FLYING uav is switched to manual mode and want to go BACK to auto.
    void    recoverFromManual();
    /// Set home position
    void    setHome(bool _set_z);

private:
    void missionThreadLoop();
    void getAutopilotInformation();
    void initHomeFrame();
    void setFlightMode(const std::string& _flight_mode);
    double updateParam(const std::string& _param_id);
    State guessState();

    // FW specifics
    void arm(const bool& _arm);
    void setParam(const std::string& _param_id,const int& _param_value);
    bool pushMission(const mavros_msgs::WaypointList& _wp_list);
    void clearMission();
    void addTakeOffWp(mavros_msgs::WaypointList& _wp_list, const MissionElement& _waypoint_element, const int& _wp_set_index);
    void addPassWpList(mavros_msgs::WaypointList& _wp_list, const MissionElement& _waypoint_element, const int& _wp_set_index);
    void addLoiterWpList(mavros_msgs::WaypointList& _wp_list, const MissionElement& _waypoint_element, const int& _wp_set_index);
    void addLandWpList(mavros_msgs::WaypointList& _wp_list, const MissionElement& _waypoint_element, const int& _wp_set_index);
    void addSpeedWpList(mavros_msgs::WaypointList& _wp_list, const MissionElement& _waypoint_element, const int& _wp_set_index);
    std::vector<geographic_msgs::GeoPoseStamped> uniformizeSpatialField( const MissionElement& _waypoint_element);
    geographic_msgs::GeoPoseStamped poseStampedtoGeoPoseStamped(const geometry_msgs::PoseStamped& _posestamped );
    geometry_msgs::PoseStamped geoPoseStampedtoPoseStamped(const geographic_msgs::GeoPoseStamped _geoposestamped );
    mavros_msgs::Waypoint geoPoseStampedtoGlobalWaypoint(const geographic_msgs::GeoPoseStamped& _geoposestamped );
    float getMissionYaw(const geometry_msgs::Quaternion& _quat);
    void checkMissionParams(const std::map<std::string, float>& _existing_params_map, const std::vector<std::string>& _required_params, const int& _wp_set_index);
    void initMission();

    //WaypointList path_;
    geometry_msgs::PoseStamped  ref_pose_;
    geometry_msgs::PoseStamped  cur_pose_;
    sensor_msgs::NavSatFix      cur_geo_pose_;
    geometry_msgs::TwistStamped cur_vel_;
    geometry_msgs::TwistStamped cur_vel_body_;
    mavros_msgs::State          mavros_state_;
    mavros_msgs::ExtendedState  mavros_extended_state_;

    //Mission
    int  mavros_reached_wp_;
    mavros_msgs::WaypointList  mavros_cur_mission_;
    geographic_msgs::GeoPoint origin_geo_;
    std::vector<int> takeoff_wps_on_mission_;
    std::vector<int> land_wps_on_mission_;
    float mission_aux_height_;
    float mission_aux_distance_;
    float mission_takeoff_minimum_pitch_;
    float mission_loit_heading_;
    float mission_loit_radius_;
    float mission_loit_forward_moving_;
    float mission_land_precision_mode_;
    float mission_land_abort_alt_;
    float mission_pass_orbit_distance_;
    float mission_pass_acceptance_radius_;

    //Control
    enum class eControlMode {LOCAL_VEL, LOCAL_POSE, GLOBAL_POSE, NONE};
    eControlMode control_mode_ = eControlMode::NONE;
    bool mavros_has_pose_ = false;
    bool mavros_has_geo_pose_ = false;

    /// Ros Communication
    ros::ServiceClient flight_mode_client_;
    ros::ServiceClient arming_client_;
    ros::ServiceClient get_param_client_;
    ros::ServiceClient set_param_client_;
    ros::ServiceClient push_mission_client_;
    ros::ServiceClient clear_mission_client_;
    ros::Publisher mavros_ref_pose_pub_;        // Not publishing right now!!
    ros::Publisher mavros_ref_pose_global_pub_; // Not publishing right now!!
    ros::Publisher mavros_ref_vel_pub_;         // Not publishing right now!!
    ros::Subscriber mavros_cur_pose_sub_;
    ros::Subscriber mavros_cur_geo_pose_sub_;
    ros::Subscriber mavros_cur_vel_sub_;
    ros::Subscriber mavros_cur_vel_body_sub_;
    ros::Subscriber mavros_cur_state_sub_;
    ros::Subscriber mavros_cur_extended_state_sub_;
    tf2_ros::Buffer tf_buffer_;
    tf2_ros::TransformListener tf_listener_;
    ros::Subscriber mavros_reached_wp_sub_;
    ros::Subscriber mavros_cur_mission_sub_;

    int robot_id_;
    enum struct AutopilotType {PX4, APM, UNKNOWN};
    AutopilotType autopilot_type_ = AutopilotType::UNKNOWN;
    std::string pose_frame_id_;
    std::string uav_home_frame_id_;
    std::string uav_frame_id_;
    tf2_ros::StaticTransformBroadcaster * static_tf_broadcaster_;
    std::map <std::string, geometry_msgs::TransformStamped> cached_transforms_;
    std::map<std::string, double> mavros_params_;
    Eigen::Vector3d local_start_pos_;

    std::thread mission_thread_;
    double mission_thread_frequency_;

    bool calling_takeoff_ = false;
    bool calling_land_ = false;

    std::atomic<State> state_ = {UNINITIALIZED};

    int mission_state_ = 0;
};

}}	// namespace grvc::fw_ns

#endif // FIXED_WING_LIB_H