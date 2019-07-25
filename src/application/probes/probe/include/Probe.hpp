#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <fstream>
#include <iostream>
#include <chrono>

#include "ros/ros.h"

#include "messages/TaskInfo.h"
#include "messages/ContextInfo.h"

class Probe {

	public:
    	Probe(int &argc, char **argv, std::string);
    	virtual ~Probe();

		void setUp();
		void run();

    private:
      	Probe(const Probe &);
    	Probe &operator=(const Probe &);

  	public:
		void receiveTaskInfo(const messages::TaskInfo::ConstPtr& /*msg*/);
		void receiveContextInfo(const messages::ContextInfo::ConstPtr& /*msg*/);


  	private:
	  ros::Publisher task_pub;
	  ros::Publisher context_pub;
};

#endif 