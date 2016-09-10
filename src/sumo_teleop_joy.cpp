/*!
  \file        sumo_teleop_joy->cpp
  \author      Arnaud Ramey <arnaud.a.ramey@gmail.com>
                -- Robotics Lab, University Carlos III of Madrid
  \date        2016/1/18

________________________________________________________________________________

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
________________________________________________________________________________

A simple node for teleoperating the Sumo
 */
#include <std_msgs/Empty.h>
#include <std_msgs/Float32.h>
#include <std_msgs/String.h>
#include <sensor_msgs/Joy.h>
#include <geometry_msgs/Twist.h>
#include <ros/ros.h>

int axis_linear = 1, axis_angular = 2, axis_90turn = 3, axis_180turn = 4, maxaxis = -1;
int button_high_jump = 1, button_posture = 2, button_anim = 3, maxbutton = -1;
double scale_linear = 1.0, scale_angular = 1.0;
std::string posture = "jumper";
bool high_jump_before = false, posture_before = false, anim_before = false;
bool axis_90before = false, axis_180before = false;
std_msgs::String string_msg;
ros::Publisher posture_pub, cmd_vel_pub, high_jump_pub, sharp_turn_pub, anim_pub;

////////////////////////////////////////////////////////////////////////////////

void joy_cb(const sensor_msgs::Joy::ConstPtr& joy) {
  int naxes = joy->axes.size(), nbuttons = joy->buttons.size();
  if (naxes <= maxaxis) {
    ROS_WARN("joy_cb(): expected at least %i axes, got %i!", maxaxis+1, naxes);
    return;
  }
  if (nbuttons <= maxbutton) {
    ROS_WARN("joy_cb(): expected at least %i buttons, got %i!", maxbutton+1, naxes);
    return;
  }
  bool command_sent = false;
  // sharp turns at 90°
  bool axis_90now = fabs(joy->axes[axis_90turn]) > 0.9;
  if (axis_90now && !axis_90before) {
    std_msgs::Float32 msg;
    msg.data = (joy->axes[axis_90turn] < 0 ? M_PI_2 : -M_PI_2);
    sharp_turn_pub.publish(msg);
    command_sent = true;
  }
  axis_90before = axis_90now;
  // sharp turns at 180°
  bool axis_180now = fabs(joy->axes[axis_180turn]) > 0.9;
  if (axis_180now && !axis_180before) {
    std_msgs::Float32 msg;
    msg.data = (joy->axes[axis_180turn] > 0 ? 2 * M_PI : -M_PI);
    sharp_turn_pub.publish(msg);
    command_sent = true;
  }
  axis_180before = axis_180now;

  // jumps
  if (joy->buttons[button_high_jump] && !high_jump_before) {
    ROS_INFO("Starting high jump!");
    high_jump_pub.publish(std_msgs::Empty());
    command_sent = true;
  }
  high_jump_before = joy->buttons[button_high_jump];

  // postures
  if (joy->buttons[button_posture] && !posture_before) {
    if (posture == "standing")    posture = "jumper";
    else if (posture == "jumper") posture = "kicker";
    else                          posture = "standing";
    string_msg.data = posture;
    posture_pub.publish(string_msg);
    command_sent = true;
  }
  posture_before = joy->buttons[button_posture];

  // anims
  if (joy->buttons[button_anim] && !anim_before) {
    string_msg.data = "tap";
    anim_pub.publish(string_msg);
    command_sent = true;
  }
  anim_before = joy->buttons[button_anim];

  // if no command was sent till here: move robot with directions of axes
  if (command_sent)
    return;
  geometry_msgs::Twist vel;
  vel.linear.x = (joy->axes[axis_linear] * scale_linear);
  vel.angular.z = (joy->axes[axis_angular] * scale_angular);
  cmd_vel_pub.publish(vel);
} // end joy_cb();

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
  ros::init(argc, argv, "sumo_teleop_joy");
  ros::NodeHandle nh_public, nh_private("~");
  // params
  nh_private.param("axis_180turn", axis_180turn, axis_180turn);
  nh_private.param("axis_90turn", axis_90turn, axis_90turn);
  nh_private.param("axis_angular", axis_angular, axis_angular);
  nh_private.param("axis_linear", axis_linear, axis_linear);
  nh_private.param("button_anim", button_anim, button_anim);
  nh_private.param("button_high_jump", button_high_jump, button_high_jump);
  nh_private.param("button_posture", button_posture, button_posture);
  nh_private.param("scale_angular", scale_angular, scale_angular);
  nh_private.param("scale_linear", scale_linear, scale_linear);
  maxaxis = std::max(axis_linear, std::max(axis_angular, std::max(axis_90turn, axis_180turn)));
  maxbutton = std::max(button_anim, std::max(button_high_jump, button_posture));
  // subscribers
  ros::Subscriber joy_sub = nh_public.subscribe<sensor_msgs::Joy>("joy", 1,  joy_cb);
  // publishers
  anim_pub = nh_public.advertise<std_msgs::String>("anim", 1);
  cmd_vel_pub = nh_public.advertise<geometry_msgs::Twist>("cmd_vel", 1);
  high_jump_pub = nh_public.advertise<std_msgs::Empty>("high_jump", 1);
  posture_pub = nh_public.advertise<std_msgs::String>("set_posture", 1);
  sharp_turn_pub = nh_public.advertise<std_msgs::Float32>("sharp_turn", 1);
  ros::spin();
  return 0;
}


