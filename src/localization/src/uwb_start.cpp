//these are all header file declaration
//#define _GLIBCXX_USE_CXX11_ABI 0
//sensor_msgs::msgs::Range
#include <ros/ros.h>
#include <ros/package.h>
#include <ros/console.h> 
#include <nodelet/nodelet.h>
#include <fstream>
#include <iostream>
#include <geometry_msgs/PoseStamped.h>
#include <geometry_msgs/PointStamped.h>
#include <geometry_msgs/Pose.h>
#include <geometry_msgs/Point.h>
#include <geometry_msgs/PoseArray.h>
#include <nav_msgs/Odometry.h>
#include <sensor_msgs/Imu.h>
#include <gtec_msgs/Ranging.h>
#include <sensor_msgs/Range.h>
//#include <mrs_msgs/TrackerPoint.h>
#include <mavros_msgs/CommandTOL.h>
#include <mavros_msgs/CommandBool.h>
#include <mavros_msgs/SetMode.h>
#include <std_srvs/SetBool.h>
#include <mrs_msgs/ControlManagerDiagnostics.h>
#include <mrs_msgs/Vec4.h>
#include <mrs_msgs/MpcTrackerDiagnostics.h>
#include <mrs_msgs/Float64Stamped.h>
#include <mrs_msgs/ReferenceStamped.h>
#include <mrs_msgs/RtkGps.h>
#include <mrs_msgs/UavState.h>
#include <mrs_lib/param_loader.h>
#include <pluginlib/class_list_macros.h>
#include <std_srvs/Trigger.h>
#include <math.h>
#include <string.h>
#include <map> 
#include <iterator> 
#include <std_msgs/String.h> 
#include <eigen3/Eigen/Dense>

namespace localization
{

  class uwb_start : public nodelet::Nodelet
  {
  public:
	virtual void onInit();
	ros::NodeHandle nh;
	struct locate{
	//bool localization_enable;
	//geometry_msgs::Point anchor[4];
	//float dist[3];
	std::map<std::string, float>		tag;
	};

  	void callbackOdomGt(const nav_msgs::OdometryConstPtr& msg);
	void callbackOdomUav(const nav_msgs::OdometryConstPtr& msg);
	void callbackTrackerDiag(const mrs_msgs::ControlManagerDiagnosticsConstPtr msg, const std::string& topic);

	void collisionavoidance(std::string uav_name);
  	void activate(void);
	void goal(std::string uav_name, float x, float y, float z, float yaw);
	void callbackOtheruavcoordinates(const mrs_msgs::RtkGpsConstPtr msg, const std::string& topic);
	void callbackuwbranging(const gtec_msgs::RangingConstPtr msg, const std::string& topic);
	void callbacksonar(const sensor_msgs::RangeConstPtr msg, const std::string& topic);
	void callbackimudata(const mrs_msgs::UavStateConstPtr msg, const std::string& topic);
	double dist3d(const double ax, const double ay, const double az, const double bx, const double by, const double bz);
	int neighbourtimer(void);
	void callbackTimerPublishDistToWaypoint(const ros::TimerEvent& te);
	void callbackTimerUwbLocate(const ros::TimerEvent& te);
	void takeoff(int client_id);
	void movement(void);
	//void uwblocate();
	//vector for subcription 
	std::vector<ros::Subscriber>                            sub_uav_rtk_gps;
  	std::vector<ros::Subscriber>                            sub_uav_diag;
  	std::vector<ros::Subscriber>                            sub_uav_sonar;
  	std::vector<ros::Subscriber>                            sub_uav_uwb_range;
  	std::vector<ros::Subscriber>                            sub_uav_imu;
  	std::vector<ros::ServiceClient>                         arm_client;
  	std::vector<ros::ServiceClient>                         motor_client;
  	std::vector<ros::ServiceClient>                         land_client;
  	std::vector<ros::ServiceClient>                         set_mode_client;
  	std::vector<ros::ServiceClient>                         goto_client;
  	std::vector<ros::ServiceClient>                         takeoff_client;
	ros::ServiceClient					takeoff_client1;

	//drones name
  	std::vector<std::string>                                other_drone_names_;
  	std::map<std::string, bool>  				other_drones_diagnostics;
	//drone localization
  	std::map<std::string, mrs_msgs::RtkGps> 		drones_gps_locate;
  	std::map<std::string, float>	 			drones_sonar_locate;
  	std::map<std::string, geometry_msgs::Point> 	        drones_imu_locate;
  	std::map<std::string, geometry_msgs::Point> 		drones_uwb_locate;
  	std::map<std::string, geometry_msgs::Point> 		drones_uwb_data;
  	std::map<std::string, struct locate> 	        	anchor;
  	std::map<std::string, geometry_msgs::Point> 	        drones_final_locate;

	std::string 						rtk_gps;
	std::string 						global;
	std::string						control_manager;
	std::string						mpc_tracker;
	std::string						diagnostics;
	//nav_msgs::Odometry					odom_gt_;
  	//nav_msgs::Odometry 					odom_uav_;
	std::string						_frame_id_;
	bool 							_simulation_;
	std::vector<ros::Publisher>				pub_reference_;
	ros::Timer 						timer_publish_dist_to_waypoint_;
	ros::Timer 						timer_publish_uwb_locate;
	std::map<std::string, boost::array<double, 4>>		new_waypoints;
	std::map<std::string, boost::array<double, 4>>::iterator itr;
	bool 							path_set=false;
	bool 							goal_set=false;
	mavros_msgs::CommandBool 				arm_request;
	mavros_msgs::SetMode 					srv_setMode;
	std_srvs::SetBool					motor_request;
	std_srvs::Trigger					srv_takeoff;
	mrs_msgs::Vec4						goto_request;
	bool							got_imu_data=false;
	bool							got_sonar_data=false;
	bool							got_uwb_data=false;
	bool							got_tracker_responce=false;
  };
}




//init function
namespace localization
{	
	//ros::NodeHandle nh("~");
	//ros::NodeHandle nh = getMTPrivateNodeHandle();
	void uwb_start::onInit()
	{
	ros::NodeHandle nh = getMTPrivateNodeHandle();
	//load parameter rom launch file
	mrs_lib::ParamLoader param_loader(nh, "uwb_start");
	//param_loader.load_param("uav_name", _uav_name_);
	param_loader.loadParam("simulation", _simulation_);
	param_loader.loadParam("frame_id", _frame_id_);
	//param_loader.load_param("network/robot_names", other_drone_names_);
	other_drone_names_ = {"uav1", "uav2", "uav3"};
	//subscrbing and publishing to respective topic 
	for (unsigned long i = 0; i < other_drone_names_.size(); i++) {
	//subscribe gps topic
	std::string prediction_topic_name=std::string("/")+other_drone_names_[i]+std::string("/")+"rtk_gps"+std::string("/")+"global";
	sub_uav_rtk_gps.push_back(nh.subscribe <mrs_msgs::RtkGps> (prediction_topic_name, 10, 						boost::bind(&uwb_start::callbackOtheruavcoordinates, this, _1, prediction_topic_name)));
 	ROS_INFO("[uwb_start]: subscribing to %s", prediction_topic_name.c_str());
	//subscribe diagnostics topic
	std::string diag_topic_name = std::string("/") + other_drone_names_[i] + std::string("/") +"control_manager"+std::string("/")+"diagnostics";    
	sub_uav_diag.push_back(nh.subscribe <mrs_msgs::ControlManagerDiagnostics> (diag_topic_name, 10, boost::bind(&uwb_start::callbackTrackerDiag, this, _1, diag_topic_name)));
	ROS_INFO("[uwb_start]: subscribing to %s", diag_topic_name.c_str());				
	//advertise reference topic
	std::string neha_uav = "/"+other_drone_names_[i]+"/control_manager/referance";
	pub_reference_.push_back(nh.advertise<mrs_msgs::ReferenceStamped>(neha_uav,1));
	ROS_INFO("[uwb_start]:publishing to %s",neha_uav.c_str());
	//subscribe sonar topic
	std::string sonar_topic_name=std::string("/")+other_drone_names_[i]+std::string("/")+"sensor"+std::string("/")+"sonar_front";
	sub_uav_sonar.push_back(nh.subscribe <sensor_msgs::Range> (sonar_topic_name, 1, boost::bind(&uwb_start::callbacksonar, this, _1, sonar_topic_name)));
 	ROS_INFO("[uwb_start]: subscribing to %s", sonar_topic_name.c_str());

	//suscribe imu topic  
	std::string imu_topic_name=std::string("/")+other_drone_names_[i]+std::string("/")+"odometry"+std::string("/")+"uav_state";
	sub_uav_imu.push_back(nh.subscribe <mrs_msgs::UavState> (imu_topic_name, 1, boost::bind(&uwb_start::callbackimudata, this, _1, imu_topic_name)));
 	ROS_INFO("[uwb_start]: subscribing to %s", imu_topic_name.c_str());

	//service call
std::string motor_service_name = std::string("/")+other_drone_names_[i]+ "/control_manager/motors";
	motor_client.push_back(nh.serviceClient<std_srvs::SetBool>(motor_service_name));

	std::string arv_service_name = std::string("/")+other_drone_names_[i]+ "/mavros/cmd/arming";
	arm_client.push_back(nh.serviceClient<mavros_msgs::CommandBool>(arv_service_name));
	//std::string land_service_name = std::string("/")+other_drone_names_[i]+ "/mavros/cmd/land";
	//land_client.push_back(nh.serviceClient<mavros_msgs::CommandTOL>(land_service_name));
	std::string mode_service_name = std::string("/")+other_drone_names_[i]+ "/mavros/set_mode";
	set_mode_client.push_back(nh.serviceClient<mavros_msgs::SetMode>(mode_service_name));

	std::string goto_service_name = std::string("/")+other_drone_names_[i]+ "/control_manager/goto";
	goto_client.push_back(nh.serviceClient<mrs_msgs::Vec4>(goto_service_name));

	std::string takeoff_service_name = std::string("/")+other_drone_names_[i]+ "/uav_manager/takeoff";
	takeoff_client.push_back(nh.serviceClient<std_srvs::Trigger>(takeoff_service_name));
	}
	//subscribe uwb topic  
	std::string uwb_topic_name=std::string("/")+"gtec"+std::string("/")+"toa"+std::string("/")+"ranging";
		sub_uav_uwb_range.push_back(nh.subscribe <gtec_msgs::Ranging> (uwb_topic_name, 1, boost::bind(&uwb_start::callbackuwbranging, this, _1, uwb_topic_name)));
	 	ROS_INFO("[uwb_start]: subscribing to %s", uwb_topic_name.c_str());
	//subscrobe to pose topic 


//------------subsriber---------------
	//ros::Subscriber sub_odom_gt_=nh.subscribe("odom_gt_in",1,&uwb_start::callbackOdomGt,this,ros::TransportHints().tcpNoDelay());
	//ros::Subscriber sub_odom_uav_=nh.subscribe("odom_uav_in", 1, &uwb_start::callbackOdomUav, this, ros::TransportHints().tcpNoDelay());

        
//---------------------timer------------------

timer_publish_dist_to_waypoint_ = nh.createTimer(ros::Rate(30), &uwb_start::callbackTimerPublishDistToWaypoint, this);
//timer_publish_uwb_locate = nh.createTimer(ros::Rate(10), &uwb_start::callbackTimerUwbLocate, this);
//------------------------service--------------

	




	
ROS_INFO_ONCE("m here in init");
	//while((!got_sonar_data)||(!got_imu_data)||(!got_uwb_data)){}
		activate();	

	}



//activate fun this is fun to form a traingle of uav
void uwb_start::activate(void)
{
	while(ros::ok()){	
	if((got_sonar_data)&&(got_uwb_data)&&(got_imu_data)){
		float l,m,n;
		float x=0,y=0,z=0,yaw=0;
		float R1,R2,R3,R4,R5,x2,y2,x3,y3,a,b,c;
		  std::cout << __FILE__ << ":" << __LINE__ << "activate function reached "  <<std::endl; 
			uwb_start::takeoff(0);
		  std::cout << __FILE__ << ":" << __LINE__ << "goal reach of uav2 is "<<other_drones_diagnostics["uav2"]<<std::endl; 
		  std::cout << __FILE__ << ":" << __LINE__ << "takeoff for uav1 complete sonar data" <<drones_sonar_locate["uav1"]<<"imu data is"<<drones_imu_locate["uav1"] <<std::endl; 
			std::cout << __FILE__ << ":" << __LINE__ << "z reading of final locate is "<<drones_final_locate["uav1"].z<<"reading from sonar is"<<drones_sonar_locate["uav1"]<<std::endl;
			std::cout << __FILE__ << ":" << __LINE__ << "z reading of final locate is "<<drones_final_locate["uav1"].z<<"reading from sonar is"<<drones_sonar_locate["uav1"]<<"after waiting "<<std::endl;
			uwb_start::takeoff(1);
		  std::cout << __FILE__ << ":" << __LINE__ << "takeoff for uav2 complete sonar data" <<drones_sonar_locate["uav2"]<<"imu data is"<<drones_imu_locate["uav2"] <<std::endl; 
			R1=anchor["uav2"].tag["uav1"];
		  std::cout << __FILE__ << ":" << __LINE__ << "i got uwb distance for second and it is "<<R1<<std::endl; 
			path_set=true;
			uwb_start::goal("uav2",4.0,25.45,1.75,0);
		  std::cout << __FILE__ << ":" << __LINE__ << "goal reach of uav2 is "<<other_drones_diagnostics["uav2"]<<std::endl; 
			ros::Duration(0.5).sleep();
			while(other_drones_diagnostics["uav2"]){}
		  std::cout << __FILE__ << ":" << __LINE__ << "goal for uav2 done "  <<std::endl; 
			ros::Duration(5).sleep();
			R2=anchor["uav2"].tag["uav1"];
			y2=(pow(R2,2)+pow(4,2)-pow(R1,2))/8;
			std::cout << __FILE__ << ":" << __LINE__ <<"y here is "<<y2<<"R1 is "<<R1<<"R2 is "<<R2<<std::endl; 
			uwb_start::goal("uav2",4.5,25.45,1.75,0);
			ros::Duration(0.5).sleep();
			while(other_drones_diagnostics["uav2"]){}
			ros::Duration(5).sleep();
			R3=anchor["uav2"].tag["uav1"];
			std::cout << __FILE__ << ":" << __LINE__ <<"R3 is "<<R3<<std::endl; 
			l=R3-R2;
			if(l>0)
			{
		  std::cout << __FILE__ << ":" << __LINE__ << "it is in right if condition "<<std::endl; 			
			x2=sqrt(pow(R2,2)-pow(y2,2))+0.5;
			}
			if(l<0)
			{
			x2=-sqrt(pow(R2,2)-pow(y2,2))+0.5;
			}
			geometry_msgs::Point X;
			X.x = x2;
			X.y = y2;
			X.z = drones_sonar_locate["uav2"];
		  std::cout << __FILE__ << ":" << __LINE__ << "location of uav2 w.r.t. uav1 is"<<X<<std::endl; 
			drones_uwb_locate["uav2"] = X;
			drones_final_locate["uav2"] = X;
		  std::cout << __FILE__ << ":" << __LINE__ << "i am at final"  <<std::endl; 
			uwb_start::takeoff(2);
			R4=anchor["uav3"].tag["uav1"];
			R5=anchor["uav3"].tag["uav2"];
		  std::cout << __FILE__ << ":" << __LINE__ << "R4 is"<<R4<< "R5 is"<<R5<<std::endl; 
			uwb_start::goal("uav3",-4.00,22.00,1.75,0);
			while(other_drones_diagnostics["uav3"]){}
			ros::Duration(5).sleep();
			float x_cord = X.x;//ok
			float y_cord = X.y;//ok

			float k = (pow(R4,2)+pow(x_cord,2)+pow(y_cord,2)-pow(R5,2))/2;
			m=anchor["uav3"].tag["uav1"]-R4;
			n=anchor["uav3"].tag["uav2"]-R5;
		  std::cout << __FILE__ << ":" << __LINE__ << "updated distance from uav1 is"<<anchor["uav3"].tag["uav1"]<< "updated distance from uav2 is"<<anchor["uav3"].tag["uav2"]<<std::endl; 			
			a=pow(x_cord,2)+pow(y_cord,2);			
			b=2*y_cord*k;
			c=pow(k,2)-(pow(R4,2)*pow(x_cord,2));
		  std::cout << __FILE__ << ":" << __LINE__ << "k is"<<k<< "x_cord is"<<x_cord<<"y_cord"<<y_cord<<"a is"<<a<<"b is"<<b<<"c is"<<c<<std::endl; 
			if(n<0||m<0)
			{
		  std::cout << __FILE__ << ":" << __LINE__ << "it is in right if condition "<<std::endl; 			
			y3=(b-sqrt(pow(b,2)-(4*a*c)))/(2*a)-(0.6);
			x3=(k-(y_cord*y3))/x_cord;
		  std::cout << __FILE__ << ":" << __LINE__ << "x is is"<<x3<< "y is"<<y3<<std::endl; 
			}
			else
			{
			y3=(b+sqrt(pow(b,2)-(4*a*c)))/(2*a)-(0.6);
			x3=(k-(y_cord*y3))/x_cord;
			}
			X.x = x3;
			X.y = y3;
			X.z = drones_sonar_locate["uav3"];
		  std::cout << __FILE__ << ":" << __LINE__ << "location of uav3 w.r.t. uav1 is"<<X<<std::endl; 
			drones_uwb_locate["uav3"] = X;
			drones_final_locate["uav3"] = X;
			//movement();
	}

  }
	
}

void uwb_start::takeoff(int client_id)
{
//set motors
	//ros::Duration(5).sleep();
	std::cout << __FILE__ << ":" << __LINE__ << "i am at takeoff start "  <<std::endl; 
	motor_request.request.data = 1;
	motor_client[client_id].call(motor_request);
	while(!motor_request.response.success)
	{
		ros::Duration(.1).sleep();
		motor_client[client_id].call(motor_request);
	}
	std::cout << __FILE__ << ":" << __LINE__ << "motor on"  <<std::endl; 
	//set arm
	ros::Duration(5).sleep();
	arm_request.request.value = true;
	arm_client[client_id].call(arm_request);
	while(!arm_request.response.success)
	{
		ros::Duration(.1).sleep();
		arm_client[client_id].call(arm_request);
	}
	std::cout << __FILE__ << ":" << __LINE__ << "arming done "  <<std::endl; 
	if(arm_request.response.success)
		{
			ROS_INFO("Arming Successful");	
		}
	//set mode
	ros::Duration(5).sleep();
	srv_setMode.request.base_mode = 0;
	srv_setMode.request.custom_mode = "offboard";
	set_mode_client[client_id].call(srv_setMode);

	if(set_mode_client[client_id].call(srv_setMode)){
		ROS_INFO("setmode send ok");
	}else{
	      ROS_ERROR("Failed SetMode");
	}
	//takeoff
	if(arm_request.response.success){
		ros::Duration(5).sleep();
		takeoff_client[client_id].call(srv_takeoff);
		std::cout << __FILE__ << ":" << __LINE__ << "i got in if after arming"<<"service responce"<<srv_takeoff.response.success<<std::endl; 
		for(int i=0;i<100;i++)
		{
		//std::cout << __FILE__ << ":" << __LINE__ << "i got in while after arming"  <<std::endl; 
			ros::Duration(.1).sleep();
			takeoff_client[client_id].call(srv_takeoff);
		}

		if(takeoff_client[client_id].call(srv_takeoff)){
			ROS_INFO("takeoff %d", srv_takeoff.response.success);
		}else{
			ROS_ERROR("Failed Takeoff");
		}
		}
	while(!takeoff_client[client_id].call(srv_takeoff)){}
	  std::cout << __FILE__ << ":" << __LINE__ << "i am at takeoff end "  <<std::endl; 
}

void uwb_start::callbackTimerPublishDistToWaypoint(const ros::TimerEvent& te)
{
	//ROS_INFO("i got in timer for publishing");
	int l = 1;
	if(goal_set){
	//ROS_ERROR("i got in if condition for publishing");

	itr=new_waypoints.begin();

	while(itr!=new_waypoints.end()){
		int l = *((itr->first).c_str()+3);
		l = l-49;
		//std::cout << __FILE__ << ":" << __LINE__ << "l is "<<l<<"waypoint is"<<"x is"<<(itr->second).at(0)<<"y is"<<(itr->second).at(1)<<"z is"<<(itr->second).at(2)<<"yaw is"<<(itr->second).at(3)<<std::endl; 

		goto_request.request.goal = itr->second;
		//boost::array<double,4> temp = {1.0,2.0,1.5,0.0};
		//goto_request.request.goal = temp;
		goto_client[l].call(goto_request);
		while(!goto_request.response.success)
		{
		std::cout << __FILE__ << ":" << __LINE__ << "i got in here "<<std::endl; 
		goto_client[l].call(goto_request);
		}
	itr++;
	}
	}
	
}



//goto function
void uwb_start::goal(std::string uav_name, float x, float y, float z, float yaw){
boost::array<double,4> new_waypoint;

	new_waypoint.at(0) = x;
	new_waypoint.at(1) = y;
	new_waypoint.at(2) = z;
	new_waypoint.at(3)    = yaw;
	new_waypoints[uav_name]=new_waypoint;
	ROS_INFO("[uwb_start]: Flying to waypoint : x: %2.2f y: %2.2f z: %2.2f yaw: %2.2f uav name: %s",x, y, z, yaw, uav_name.c_str() );
goal_set =true;
}
//localization algo 
//clear
//or mrs_msgs::RtkGps::ConstPtr&
 
/* 
void uwb_start::movement(){
new_waypoints["uav1"].reference.position.y = new_waypoints["uav1"].reference.position.y+15;
new_waypoints["uav2"].reference.position.y = new_waypoints["uav2"].reference.position.y+15;
new_waypoints["uav3"].reference.position.y = new_waypoints["uav3"].reference.position.y+15;
new_waypoints["uav4"].reference.position.y = new_waypoints["uav4"].reference.position.y+15;
new_waypoints["uav5"].reference.position.y = new_waypoints["uav5"].reference.position.y+15;
new_waypoints["uav6"].reference.position.y = new_waypoints["uav6"].reference.position.y+15;

 }
 */

void uwb_start::callbackTimerUwbLocate(const ros::TimerEvent& te)
{
	if((path_set==true)&&(goal_set!=true)){
	//ROS_INFO("[uwb_start]: m here in uwblocate 1");
	std::map<std::string, struct locate>::iterator anchor_itr; 
	std::map<std::string, float>::iterator tag_itr; 
	//ROS_INFO("[uwb_start]: m here in uwblocate 2");
	for(anchor_itr = anchor.begin(); anchor_itr != anchor.end(); anchor_itr++){
	tag_itr = ((anchor_itr->second).tag).begin(); 
	Eigen::MatrixXd A (2,2);
	//ROS_INFO("[uwb_start]: m here in uwblocate 3");

	//here is error in declaration of A
  	std::cout << __FILE__ << ":" << __LINE__  << "value of 1 is" << (drones_final_locate[next(tag_itr, 1)->first].x-drones_final_locate[tag_itr->first].x) <<"value of 2 is"<<(drones_final_locate[next(tag_itr, 1)->first].y-drones_final_locate[tag_itr->first].y)<<"value of 3 is"<<(drones_final_locate[next(tag_itr, 2)->first].x-drones_final_locate[next(tag_itr, 1)->first].x)<< "value of 4 is"<<(drones_final_locate[next(tag_itr, 2)->first].y-drones_final_locate[next(tag_itr, 1)->first].y) <<std::endl;
	A<<(drones_final_locate[next(tag_itr, 1)->first].x-drones_final_locate[tag_itr->first].x),(drones_final_locate[next(tag_itr, 1)->first].y-drones_final_locate[tag_itr->first].y),(drones_final_locate[next(tag_itr, 2)->first].x-drones_final_locate[next(tag_itr, 1)->first].x),(drones_final_locate[next(tag_itr, 2)->first].y-drones_final_locate[next(tag_itr, 1)->first].y);
	std::cout << __FILE__ << ":" << __LINE__  << "matrix A is"<<A<<std::endl;
	//A<< (, , , );
	//ROS_INFO("[uwb_start]: m here in uwblocate 4");
	Eigen::MatrixXd B(2,1);
	B<< ((pow(tag_itr->second,2))-(pow((drones_sonar_locate[anchor_itr->first]-drones_final_locate[tag_itr->first].z),2))-(pow(drones_final_locate[tag_itr->first].x,2))-(pow(drones_final_locate[tag_itr->first].y,2)))-((pow((next(tag_itr, 1))->second,2))-(pow((drones_sonar_locate[anchor_itr->first]-drones_final_locate[(next(tag_itr, 1))->first].z),2))-(pow(drones_final_locate[(next(tag_itr, 1))->first].x,2))-(pow(drones_final_locate[(next(tag_itr, 1))->first].y,2))),
	((pow(next(tag_itr, 1)->second,2))-(pow((drones_sonar_locate[anchor_itr->first]-drones_final_locate[next(tag_itr, 1)->first].z),2))-(pow(drones_final_locate[next(tag_itr, 1)->first].x,2))-(pow(drones_final_locate[next(tag_itr, 1)->first].y,2)))-((pow((next(tag_itr, 2))->second,2))-(pow((drones_sonar_locate[anchor_itr->first]-drones_final_locate[(next(tag_itr, 2))->first].z),2))-(pow(drones_final_locate[(next(tag_itr, 2))->first].x,2))-(pow(drones_final_locate[(next(tag_itr, 2))->first].y,2)));
	std::cout << __FILE__ << ":" << __LINE__  << "matrix B is"<<B<<std::endl;

	//B<< (,);
	//ROS_INFO("[uwb_start]: m here in uwblocate 5");
	geometry_msgs::Point X;

	X.x = (A.inverse()*B)(0);
	X.y = (A.inverse()*B)(1);
	X.z = drones_sonar_locate[anchor_itr->first];
	drones_uwb_locate[anchor_itr->first] = X;
	drones_final_locate[anchor_itr->first] = X;	
	}
  }
  	//ROS_INFO("[uwb_start]: i am exiting uwb locate timer");
}


void uwb_start::callbackOtheruavcoordinates(const mrs_msgs::RtkGpsConstPtr msg, const std::string& topic){
  ROS_INFO("[uwb_start]: m here in callbackOtheruavcoordinates");
  int uav_no = *(topic.c_str()+3); 
  //std::string uav_name="uav"+uav_name-1;
  std::string uav_name="uav"+uav_no;
  drones_gps_locate[uav_name]=*msg;
}
int sonar_count[3];
//callback function for sonar 
void uwb_start::callbacksonar(const sensor_msgs::RangeConstPtr msg, const std::string& topic){
  
  int uav_no = *(topic.c_str()+4); 
  uav_no = uav_no-48;
  std::string uav_name="uav"+std::to_string(uav_no);
  uav_no--;
  sonar_count[uav_no] = 1;
  if(sonar_count[0]&&sonar_count[1]&&sonar_count[2])
	{
	  	got_sonar_data=true;

	}	

  drones_sonar_locate[uav_name] = msg->range+0.08;
  drones_final_locate[uav_name].z=msg->range+0.08;
  //if(uav_name == "uav1")
  //std::cout << __FILE__ << ":" << __LINE__  << "callback sonar data uav name is " << uav_name <<"and range is "<<msg->range<<"data seen as sonar is"<<drones_sonar_locate["uav1"]<<"data seen as final locate is"<<drones_final_locate["uav1"].z<<std::endl; 
}

//callback function for imu sensor
// see this algo 
int c;
double x_0 =0,y_0 =0,z_0 =0, t=0;
int imu_count[3];
void uwb_start::callbackimudata(const mrs_msgs::UavStateConstPtr msg, const std::string& topic){
  //std::cout << __FILE__ << ":" << __LINE__  << "[uwb_start]: m here in callbackimudata x_0 is " << x_0 <<" y_0 is "<<y_0<<"z_0 is "<<z_0<<std::endl; 
  int uav_no = *(topic.c_str()+4); 
  uav_no = uav_no-48;
  std::string uav_name="uav"+std::to_string(uav_no);
  uav_no--;
  imu_count[uav_no] = 1;
  		
  if(imu_count[0]&&imu_count[1]&&imu_count[2])	
  	{
	  	got_imu_data=true;
	}


  double dt = (msg->header.stamp.sec + (msg->header.stamp.nsec/pow(10,9))) - t ;
  t = msg->header.stamp.sec + (msg->header.stamp.nsec/pow(10,9));
  //std::cout << __FILE__ << ":" << __LINE__  << "time is " << t <<"time difference is "<<dt<<std::endl;
	if(c>0 && dt<1)  
	{ double x = x_0 + dt*msg->velocity.linear.x+0.5*pow(dt,2)*(msg->acceleration.linear.x);
	  double y = y_0 + dt*msg->velocity.linear.y+0.5*pow(dt,2)*(msg->acceleration.linear.y);
	  double z = z_0 + dt*msg->velocity.linear.z+0.5*pow(dt,2)*(msg->acceleration.linear.z);
          //std::cout << __FILE__ << ":" << __LINE__  << "difference in x is " << dt*msg->velocity.linear.x+0.5*pow(dt,2)*(msg->acceleration.linear.x) <<"difference in y is "<<dt*msg->velocity.linear.y+0.5*pow(dt,2)*(msg->acceleration.linear.y)<<"difference in z is "<<dt*msg->velocity.linear.z+0.5*pow(dt,2)*(msg->acceleration.linear.z)<<std::endl;
	  x_0 = x;
	  y_0 = y;
	  z_0 = z;
	  geometry_msgs::Point X;
	  X.x=x;
	  X.y=y;
	  X.z=z;
	  drones_imu_locate[uav_name]=X;
	  drones_final_locate[uav_name]=X;
	  //std::cout << __FILE__ << ":" << __LINE__  << "callback imu data uav name is " << uav_name <<"and data is "<<X<<std::endl; 
	}
c++;
}
int uwb_count[3];
//callback function for uwb sensor
void uwb_start::callbackuwbranging(const gtec_msgs::RangingConstPtr msg, const std::string& topic){
//see this condition properly
  std::string uav_name = msg->anchorId;
  int uav_no = *((msg->anchorId).c_str()+3); 
  uav_no = uav_no-49;
  uwb_count[uav_no] = 1;
  //std::cout << __FILE__ << ":" << __LINE__ <<"uwb "<<
  if(uwb_count[0]&&uwb_count[1]&&uwb_count[2])	
  	{
	  	got_uwb_data=true;
		//std::cout << __FILE__ << ":" << __LINE__  << " got uwb data changed "<<std::endl;
	}
  if(msg->range>2000)
	{
  	anchor[uav_name].tag[msg->tagId] = msg->range/1000.0000;
	  //std::cout << __FILE__ << ":" << __LINE__ << uav_name << "uwb reading updated " <<msg->tagId<<"this is written value"<<msg->range/1000<<"this is actual value"<<msg->range<<"this is what it sees"<<anchor[uav_name].tag[msg->tagId]<<std::endl; 	
	}
}
//need to see this 






//more time lag
//check no. of neighbour
//unnecesarry function
/*void uwb_start::callbackOdomGt(const nav_msgs::OdometryConstPtr& msg){
  ROS_INFO_ONCE("m here in callbackOdomGt");
  odom_gt_ = *msg;		
}
void uwb_start::callbackOdomUav(const nav_msgs::OdometryConstPtr& msg){
  ROS_INFO_ONCE("m here in callbackOdomUav");
  odom_uav_ = *msg;		
  }
*/
void uwb_start::callbackTrackerDiag(const mrs_msgs::ControlManagerDiagnosticsConstPtr msg, const std::string& topic){
  ROS_INFO_ONCE("m here in callbackTrackerDiag");
  got_tracker_responce = true;
  int uav_no = *(topic.c_str()+4); 
  uav_no = uav_no-48;
  std::string uav_name="uav"+std::to_string(uav_no);

  other_drones_diagnostics[uav_name] = msg->tracker_status.have_goal;  
  if (!msg->tracker_status.have_goal){
  //std::cout << __FILE__ << ":" << __LINE__ << uav_name << "waypoint reached "  <<std::endl; 
  //uwb_start::activate();
  }
}

double dist3d(const double ax, const double ay, const double az, const double bx, const double by, const double bz) {

  return sqrt(pow(ax - bx, 2) + pow(ay - by, 2) + pow(az - bz, 2));
}
}
PLUGINLIB_EXPORT_CLASS(localization::uwb_start, nodelet::Nodelet);
