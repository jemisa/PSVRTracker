#ifndef MORPHEUS_HMD_H
#define MORPHEUS_HMD_H

#include "PSVRConfig.h"
#include "DeviceEnumerator.h"
#include "DeviceInterface.h"
#include "MathUtility.h"
#include <string>
#include <vector>
#include <deque>
#include <array>

// The angle the accelerometer reading will be pitched by
// if the Morpheus is held such that the face plate is perpendicular to the ground
// i.e. where what we consider the "identity" pose
#define MORPHEUS_ACCELEROMETER_IDENTITY_PITCH_DEGREES 0.0f

class MorpheusHMDConfig : public PSVRConfig
{
public:
    static const int CONFIG_VERSION;

    MorpheusHMDConfig(const std::string &fnamebase = "MorpheusHMDConfig")
        : PSVRConfig(fnamebase)
		, is_valid(false)
		, version(CONFIG_VERSION)
		, disable_command_interface(true)
		, position_filter_type("LowPassOptical")
		, orientation_filter_type("MadgwickARG")
		, raw_accelerometer_variance(0.f)
        , max_velocity(1.f)
		, raw_gyro_variance(0.f)
		, raw_gyro_drift(0.f)
		, mean_update_time_delta(0.008333f)
		, position_variance_exp_fit_a(0.0994158462f)
		, position_variance_exp_fit_b(-0.000567041978f)
		, orientation_variance(0.005f)
        , max_poll_failure_count(100)
        , prediction_time(0.f)
		, tracking_color_id(PSVRTrackingColorType_Blue)
    {
		// The Morpheus uses the BMI055 IMU Chip: 
		// https://d3nevzfk7ii3be.cloudfront.net/igi/hnlrYUv5BUb6lMoW.huge
		// https://www.bosch-sensortec.com/bst/products/all_products/bmi055
		//
		// The Accelerometer can operate in one of 4 modes: 
		//   �2g, �4g, �8g, �16g
		// The Gyroscope can operate in one of 5 modes: 
		//   �125�/s, �250�/s, �500�/s, �1000�/s, �2000�/s
		//   (or �2.18 rad/s, �4.36 rad/s, �8.72 rad/s, �17.45 rad/s, �34.9 rad/s)
		//
		// I haven't seen any indication that suggests the Morpheus changes modes.
		// However we need to calibrate the sensor bias at startup
		
		// NOTE: If you are unfamiliar like I was with "LSB/Unit"
		// see http://stackoverflow.com/questions/19161872/meaning-of-lsb-unit-and-unit-lsb

		// Accelerometer configured at �2g, 1024 LSB/g
		accelerometer_gain.x = 1.f / (1024.f);
		accelerometer_gain.y = 1.f / (1024.f);
		accelerometer_gain.z = 1.f / (1024.f);

		// Assume no bias until calibration says otherwise
		raw_accelerometer_bias.x = 0.f;
		raw_accelerometer_bias.y = 0.f;
		raw_accelerometer_bias.z = 0.f;

		// Gyroscope configured at �1000�/s, 32.8 LSB/(�/s)
		// but we want the calibrated gyro value in radians/s so add in a deg->rad conversion as well
		gyro_gain.x = 1.f / (32.8f / k_degrees_to_radians);
		gyro_gain.y = 1.f / (32.8f / k_degrees_to_radians);
		gyro_gain.z = 1.f / (32.8f / k_degrees_to_radians);

		// Assume no bias until calibration says otherwise
		raw_gyro_bias.x = 0.f;
		raw_gyro_bias.y = 0.f;
		raw_gyro_bias.z = 0.f;

		// This is the variance of the calibrated gyro value recorded for 100 samples
		// Units in rad/s^2
		raw_gyro_variance = 1.33875039e-006f;

		// This is the drift of the raw gyro value recorded for 60 seconds
		// Units rad/s
		raw_gyro_drift = 0.00110168592f;

		// This is the ideal accelerometer reading you get when the DS4 is held such that 
		// the light bar facing is perpendicular to gravity.        
		identity_gravity_direction.x = 0.f;
		identity_gravity_direction.y = cosf(MORPHEUS_ACCELEROMETER_IDENTITY_PITCH_DEGREES*k_degrees_to_radians);
		identity_gravity_direction.z = -sinf(MORPHEUS_ACCELEROMETER_IDENTITY_PITCH_DEGREES*k_degrees_to_radians);
    };

    virtual const configuru::Config writeToJSON();
    virtual void readFromJSON(const configuru::Config &pt);

    bool is_valid;
    long version;

	// Flag to disable usage of the command usb interface if other apps want to
	bool disable_command_interface;

	// The type of position filter to use
	std::string position_filter_type;

	// The type of orientation filter to use
	std::string orientation_filter_type;

	// calibrated_acc= raw_acc*acc_gain + acc_bias
	PSVRVector3f accelerometer_gain;
	PSVRVector3f raw_accelerometer_bias;
	// The variance of the raw gyro readings in rad/sec^2
	float raw_accelerometer_variance;

	inline PSVRVector3f get_calibrated_accelerometer_variance() const {
		return 
            {accelerometer_gain.x*raw_accelerometer_variance, 
			accelerometer_gain.y*raw_accelerometer_variance,
            accelerometer_gain.z*raw_accelerometer_variance};
	}

	// Maximum velocity for the controller physics (meters/second)
	float max_velocity;

	// The calibrated "down" direction
	PSVRVector3f identity_gravity_direction;

	// calibrated_gyro= raw_gyro*gyro_gain + gyro_bias
	PSVRVector3f gyro_gain;
	PSVRVector3f raw_gyro_bias;
	// The variance of the raw gyro readings in rad/sec^2
	float raw_gyro_variance;
	// The drift raw gyro readings in rad/second
	float raw_gyro_drift;

	inline PSVRVector3f get_calibrated_gyro_variance() const {
        return {gyro_gain.x*raw_gyro_variance, gyro_gain.y*raw_gyro_variance, gyro_gain.z*raw_gyro_variance};
	}
	inline PSVRVector3f get_calibrated_gyro_drift() const {
        return {gyro_gain.x*raw_gyro_drift, gyro_gain.y*raw_gyro_drift, gyro_gain.z*raw_gyro_drift};
	}

	// The average time between updates in seconds
	float mean_update_time_delta;

	// The variance of the controller position as a function of pixel area
	float position_variance_exp_fit_a;
	float position_variance_exp_fit_b;

	// The variance of the controller orientation (when sitting still) in rad^2
	float orientation_variance;

	inline float get_position_variance(float projection_area) const {
		return position_variance_exp_fit_a*exp(position_variance_exp_fit_b*projection_area);
	}

    long max_poll_failure_count;
	float prediction_time;

	PSVRTrackingColorType tracking_color_id;
};

struct MorpheusHMDSensorFrame
{
	int SequenceNumber;

	PSVRVector3i RawAccel;
	PSVRVector3i RawGyro;

	PSVRVector3f CalibratedAccel;
	PSVRVector3f CalibratedGyro;

	void clear()
	{
		SequenceNumber = 0;
		RawAccel= *k_PSVR_int_vector3_zero;
		RawGyro= *k_PSVR_int_vector3_zero;
		CalibratedAccel= *k_PSVR_float_vector3_zero;
		CalibratedGyro= *k_PSVR_float_vector3_zero;
	}

	void parse_data_input(const MorpheusHMDConfig *config, const struct MorpheusRawSensorFrame *data_input);
};

struct MorpheusHMDSensorState : public CommonHMDSensorState
{
	std::array< MorpheusHMDSensorFrame, 2> SensorFrames;

    MorpheusHMDSensorState()
    {
        clear();
    }

    void clear()
    {
        CommonHMDSensorState::clear();
		DeviceType = Morpheus;

		SensorFrames[0].clear();
		SensorFrames[1].clear();
    }

	void parse_data_input(const MorpheusHMDConfig *config, const struct MorpheusSensorData *data_input);
};

class MorpheusHMD : public IHMDInterface 
{
public:
    MorpheusHMD();
    virtual ~MorpheusHMD();

    // MorpheusHMD
    bool open(); // Opens the first HID device for the controller

    // -- IDeviceInterface
    bool matchesDeviceEnumerator(const DeviceEnumerator *enumerator) const override;
    bool open(const DeviceEnumerator *enumerator) override;
    bool getIsOpen() const override;
    bool getIsReadyToPoll() const override;
    IDeviceInterface::ePollResult poll() override;
    void close() override;
    long getMaxPollFailureCount() const override;
    CommonSensorState::eDeviceType getDeviceType() const override
    {
        return CommonSensorState::Morpheus;
    }
    static CommonSensorState::eDeviceType getDeviceTypeStatic()
    {
        return CommonSensorState::Morpheus;
    }
    const CommonSensorState * getSensorState(int lookBack = 0) const override;

    // -- IHMDInterface
    std::string getUSBDevicePath() const override;
	void getTrackingShape(PSVRTrackingShape &outTrackingShape) const override;
	bool setTrackingColorID(const PSVRTrackingColorType tracking_color_id) override;
	bool getTrackingColorID(PSVRTrackingColorType &out_tracking_color_id) const override;
	float getPredictionTime() const override;

    // -- Getters
    inline const MorpheusHMDConfig *getConfig() const
    {
        return &cfg;
    }
    inline MorpheusHMDConfig *getConfigMutable()
    {
        return &cfg;
    }

    // -- Setters
	void setTrackingEnabled(bool bEnableTracking);

private:
    // Constant while the HMD is open
    MorpheusHMDConfig cfg;
    class MorpheusUSBContext *USBContext;                    // Buffer that holds static MorpheusAPI HMD description

    // Read HMD State
    int NextPollSequenceNumber;
    struct MorpheusSensorData *InData;                        // Buffer to hold most recent MorpheusAPI tracking state
    std::deque<MorpheusHMDSensorState> HMDStates;

	bool bIsTracking;
};

#endif // MORPHEUS_HMD_H
