#ifndef G3T1_3_HPP
#define G3T1_3_HPP

#include <string>
#include <exception>

#include "ros/ros.h"

#include "bsn/range/Range.hpp"
#include "bsn/resource/Battery.hpp"
#include "bsn/generator/Markov.hpp"
#include "bsn/generator/DataGenerator.hpp"
#include "bsn/filters/MovingAverage.hpp"
#include "bsn/operation/Operation.hpp"
#include "bsn/configuration/SensorConfiguration.hpp"

#include "component/Sensor.hpp"

#include "messages/SensorData.h"

class G3T1_3 : public Sensor {
    
	private:
      	G3T1_3(const G3T1_3 &);
    	G3T1_3 &operator=(const G3T1_3 &);
		
  	public:
    	G3T1_3(const int32_t &argc, char **argv);
    	virtual ~G3T1_3();
    	
		virtual void setUp();
    	virtual void tearDown();

        double collect();
        double process(const double &data);
        void transfer(const double &data);

  	private:
		bsn::generator::Markov markov;
		bsn::filters::MovingAverage filter;
		bsn::configuration::SensorConfiguration sensorConfig;

		ros::NodeHandle handle;
		ros::Publisher data_pub;
};

#endif 