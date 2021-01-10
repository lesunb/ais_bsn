#include <ros/ros.h>
#include <ros/console.h>

#include <iostream>
#include <string>
#include <vector>
#include <map>

#include "messages/DiagnosticsData.h"
#include "messages/DiagnosticsStatus.h"
#include "archlib/Status.h"

class DiagnosticsAnalyzer {
    public:
        DiagnosticsAnalyzer(int &argc, char **argv, std::string name);
        ~DiagnosticsAnalyzer();

        void setUp();
        void tearDown();
        void body();

        int32_t run();

    private:

        void processCentralhubData(const messages::DiagnosticsData::ConstPtr&);
        void processSensorData(const messages::DiagnosticsData::ConstPtr&);
        void processSensorStatus(const messages::DiagnosticsStatus::ConstPtr&);
        void processSensorOn(const archlib::Status::ConstPtr&);
        void busyWait();

        //ros::NodeHandle nh;
        ros::Subscriber sensorSub;
        ros::Subscriber centralhubSub;
        ros::Subscriber sensorStatusSub;
        ros::Subscriber sensorOnSub;
        
        //int32_t collectedId;
        //int32_t processedId;
        std::map<std::string, int> currentCollectedId;
        std::map<std::string, int> currentSentId;

        std::map<std::string, int> expectedCollectedId;
        std::map<std::string, int> expectedSentId;

        bool init;
        bool ON_reached;
        bool OFF_reached;
        bool COLLECTED_reached;
        bool wait_collect, wait_process;
        bool PROCESSED_reached;

        bool gotMessage;
};