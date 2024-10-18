#include "traject_recorder/traject_recorder_auto.hpp"
#include <cmath>

namespace traject_recorder
{
TrajectRecorderAuto::TrajectRecorderAuto(const rclcpp::NodeOptions & options)
: Node("traject_recorder_auto", options),
{
    declare_parameter("sampling_time",10000);               //[s]
    declare_parameter("distance_interval",10000);           //[m]
    get_parameter("sampling_time", sampling_time_);
    get_parameter("distance_interval", distance_interval_);
    
    subscription_ = this->create_subscription<nav_msgs::msg::Odometry>(
        "/odom", 10, std::bind(&PositionMonitor::odom_callback, this, std::placeholders::_1));
    last_position_ = std::nullopt;
    start_time_ = this->now();

    std::String csv_file_path = "saved_points.csv" 
    csv_file_.open(csv_file_path);
    if (!csv_file_) {
        RCLCPP_ERROR(this->get_logger(), "CSVファイルを開けませんでした。");
    } else {
        // ヘッダを書き込む
        csv_file_ << "x,y,z\n";
    }
}

void TrajectRecorderAuto::odom_callback(const nav_msgs::msg::Odometry::SharedPtr msg)
{
    auto current_position = msg->pose.pose.position;
    auto current_time = this->now();
    if (!last_position_) {
        // 初期位置を保存
        last_position_ = current_position;
        start_time_ = current_time;
        return;
    }
    // 移動距離を計算
    double distance = calculate_distance(*last_position_, current_position);
    // 経過時間を計算
    double elapsed_time = (current_time - start_time_).seconds();
    if (distance >= distance_interval_ && elapsed_time >= time_threshold_) {
        // 条件を満たした場合、ポイントを保存
        save_point(current_position);
        // 位置と時間をリセット
        last_position_ = current_position;
        start_time_ = current_time;
        publish_marker();
    }
}

double TrajectRecorderAuto::calculate_distance(const geometry_msgs::msg::Point &pos1, const geometry_msgs::msg::Point &pos2)
{
    return std::sqrt(std::pow(pos2.x - pos1.x, 2) + std::pow(pos2.y - pos1.y, 2));
}

void TrajectRecorderAuto::save_point(const geometry_msgs::msg::Point &position)
{
    saved_points_.push_back(position);
    RCLCPP_INFO(this->get_logger(), "ポイント保存: x=%.2f, y=%.2f, z=%.2f", position.x, position.y, position.z);
}

void TrajectRecorderAuto::publish_marker()
{
    visualization_msgs::msg::Marker marker;
    marker.header.frame_id = "map";
    marker.header.stamp = this->now();
    marker.ns = "points";
    marker.id = 0;
    marker.type = visualization_msgs::msg::Marker::POINTS;
    marker.action = visualization_msgs::msg::Marker::ADD;
    marker.scale.x = marker.scale.y = marker.scale.z = 0.02;
    marker.color.a = 1.0;  
    marker.color.r = 1.0;  
    marker.color.g = 0.0;  
    marker.color.b = 0.0;  
    marker.lifetime = rclcpp::Duration(0.0);
    for (const auto &point : saved_points_) {
        geometry_msgs::msg::Point p;
        p.x = point.x;
        p.y = point.y;
        p.z = point.z;
        marker.points.push_back(p);
    }
    marker_pub_->publish(marker);
}

void TrajectRecorderAuto::write_point_to_csv(const geometry_msgs::msg::Point &point)
{
    if (csv_file_.is_open()) {
        csv_file_ << point.x << "," << point.y << "," << point.z << "\n";
        RCLCPP_INFO(this->get_logger(), "CSVに書き込み: x=%.2f, y=%.2f, z=%.2f", point.x, point.y, point.z);
    }
}

}

#include <rclcpp_components/register_node_macro.hpp>
RCLCPP_COMPONENTS_REGISTER_NODE(traject_recorder::TrajectRecorderAuto)