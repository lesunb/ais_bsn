#include "component/g3t1_1/G3T1_1.hpp"

#define BATT_UNIT 5

using namespace bsn::range;
using namespace bsn::generator;
using namespace bsn::configuration;

G3T1_1::G3T1_1(int &argc, char **argv, const std::string &name) :
    Sensor(argc, argv, name, "oximeter", true, 1, bsn::resource::Battery("oxi_batt", 100, 100, 1)),
    markov(),
    dataGenerator(),
    filter(1),
    sensorConfig(),
    collected_risk() {}

G3T1_1::~G3T1_1() {}

void G3T1_1::setUp() {
    Component::setUp();
    
    std::array<bsn::range::Range,5> ranges;
    std::string s;

   { // Configure markov chain
        std::vector<std::string> lrs,mrs0,hrs0,mrs1,hrs1;

        handle.getParam("LowRisk", s);
        lrs = bsn::utils::split(s, ',');
        handle.getParam("MidRisk0", s);
        mrs0 = bsn::utils::split(s, ',');
        handle.getParam("HighRisk0", s);
        hrs0 = bsn::utils::split(s, ',');
        handle.getParam("MidRisk1", s);
        mrs1 = bsn::utils::split(s, ',');
        handle.getParam("HighRisk1", s);
        hrs1 = bsn::utils::split(s, ',');

        ranges[0] = Range(std::stod(hrs0[0]), std::stod(hrs0[1]));
        ranges[1] = Range(std::stod(mrs0[0]), std::stod(mrs0[1]));
        ranges[2] = Range(std::stod(lrs[0]), std::stod(lrs[1]));
        ranges[3] = Range(std::stod(mrs1[0]), std::stod(mrs1[1]));
        ranges[4] = Range(std::stod(hrs1[0]), std::stod(hrs1[1]));
    }

    { // Configure sensor configuration
        Range low_range = ranges[2];
        
        std::array<Range,2> midRanges;
        midRanges[0] = ranges[1];
        midRanges[1] = ranges[3];
        
        std::array<Range,2> highRanges;
        highRanges[0] = ranges[0];
        highRanges[1] = ranges[4];

        std::array<Range,3> percentages;

        handle.getParam("lowrisk", s);
        std::vector<std::string> low_p = bsn::utils::split(s, ',');
        percentages[0] = Range(std::stod(low_p[0]), std::stod(low_p[1]));

        handle.getParam("midrisk", s);
        std::vector<std::string> mid_p = bsn::utils::split(s, ',');
        percentages[1] = Range(std::stod(mid_p[0]), std::stod(mid_p[1]));

        handle.getParam("highrisk", s);
        std::vector<std::string> high_p = bsn::utils::split(s, ',');
        percentages[2] = Range(std::stod(high_p[0]), std::stod(high_p[1]));

        sensorConfig = SensorConfiguration(0, low_range, midRanges, highRanges, percentages);
    }
}

void G3T1_1::tearDown() {
    Component::tearDown();
}

double G3T1_1::collect() {
    double m_data = 0;
    ros::ServiceClient client = handle.serviceClient<services::PatientData>("getPatientData");
    services::PatientData srv;
    messages::DiagnosticsData msg;

    srv.request.vitalSign = "oxigenation";

    if (client.call(srv)) {
        m_data = srv.response.data;
        ROS_INFO("new data collected: [%s]", std::to_string(m_data).c_str());
    } else {
        ROS_INFO("error collecting data");
    }

    battery.consume(BATT_UNIT);
    collected_risk = sensorConfig.evaluateNumber(m_data);

    my_posix_time = ros::Time::now().toBoost();
    timestamp = boost::posix_time::to_iso_extended_string(my_posix_time);

    currentStatus = "collected";
    publishStatus();
    flushData();


    return m_data;
}

double G3T1_1::process(const double &m_data) {
    double filtered_data;
    
    filter.insert(m_data);
    filtered_data = filter.getValue();
    battery.consume(BATT_UNIT*filter.getRange());

    ROS_INFO("filtered data: [%s]", std::to_string(filtered_data).c_str());

    return filtered_data;
}

void G3T1_1::transfer(const double &m_data) {
    double risk;
    risk = sensorConfig.evaluateNumber(m_data);

    bool acc_failed = false;

    if (risk < 0 || risk > 100) {
        my_posix_time = ros::Time::now().toBoost();
        timestamp = boost::posix_time::to_iso_extended_string(my_posix_time);

        currentStatus = "out of bounds";
        publishStatus();
        flushData();

        acc_failed = true;
        //this->dataId++;
        //throw std::domain_error("risk data out of boundaries");
    }

    if (label(risk) != label(collected_risk)) {
        my_posix_time = ros::Time::now().toBoost();
        timestamp = boost::posix_time::to_iso_extended_string(my_posix_time);

        currentStatus = "sensor accuracy fail";
        publishStatus();
        flushData();

        acc_failed = true;
        //this->dataId++;
        //throw std::domain_error("sensor accuracy fail");
    }

    ros::NodeHandle handle;
    data_pub = handle.advertise<messages::SensorData>("oximeter_data", 10);

    messages::SensorData msg;
    messages::DiagnosticsData statusMsg;

    msg.id = this->dataId;
    msg.type = type;
    msg.data = m_data;
    msg.risk = risk;
    msg.batt = battery.getCurrentLevel();

    data_pub.publish(msg);
    battery.consume(BATT_UNIT);

    if (currentProperty == "p3") {
        my_posix_time = ros::Time::now().toBoost();
        timestamp = boost::posix_time::to_iso_extended_string(my_posix_time);

        if (acc_failed) {
            currentStatus = "data out of range";
        } else {
            currentStatus = "data in range";
        }
        publishStatus();
        flushData();
    }

    my_posix_time = ros::Time::now().toBoost();
    timestamp = boost::posix_time::to_iso_extended_string(my_posix_time);

    currentStatus = "sent";
    publishStatus();
    flushData();

    this->dataId++;

    ROS_INFO("risk calculated and transferred: [%.2f%%]", risk);
}

std::string G3T1_1::label(double &risk) {
    std::string ans;
    if(sensorConfig.isLowRisk(risk)){
        ans = "low";
    } else if (sensorConfig.isMediumRisk(risk)) {
        ans = "moderate";
    } else if (sensorConfig.isHighRisk(risk)) {
        ans = "high";
    } else {
        ans = "unknown";
    }

    return ans;
}