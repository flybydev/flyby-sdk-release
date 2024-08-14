#include <iostream>
#include <string>
#include <sstream>
#include <utility>

#include <gst/gst.h>

#include <flyby/drone/drone.h>
#include <flyby/payload/component/camera.h>
#include <flyby/payload/component/gimbal.h>
#include <flyby/payload/component/lrf.h>
#include <flyby/payload/payload.h>
#include <flyby/stream/stream.h>

#include <flyby/util/log.h>
#include <flyby/util/version.h>

#define SIMPLE_PAYLOAD_VERSION "1.0.0"

using namespace flyby;

class SimpleGimbal : public Gimbal {
public:
	SimpleGimbal() : Gimbal() {}

	~SimpleGimbal() override = default;

	void set_roll_pitch_yaw_angle(double roll, double pitch, double yaw) override {
		std::cout << "set_roll_picth_yaw_angle" << std::endl;
	}

	void set_roll_pitch_yaw_speed(double roll, double pitch, double yaw) override {
		std::cout << "set_roll_picth_yaw_speed" << std::endl;
	}
};

class SimpleEOCamera : public Camera {
public:
	SimpleEOCamera() : Camera("Standard") {
		m_picture = true;
		m_video = true;
		m_zoom = true;
		m_gimbal = true;
	}

	~SimpleEOCamera() override = default;

	void set_optical_zoom_level(int level) override {
		std::cout << "set_optical_zoom_level" << std::endl;
	}

	void set_shutter_speed(unsigned int speed) override {
		std::cout << "set_shutter_speed" << std::endl;
	}

	void start_video_recording() override {
		m_is_recording = true;
		std::cout << "start_video_recording" << std::endl;
	}

	void stop_video_recording() override {
		m_is_recording = false;
		std::cout << "stop_video_recording" << std::endl;
	}

	void set_focus_position(unsigned int position) override {
		std::cout << "set_focus_position" << std::endl;
	}
};

class SimpleIRCamera : public IRCamera {
public:
	SimpleIRCamera() : IRCamera("Thermal") {
		m_zoom = false;
		m_picture = false;
		m_video = false;
		m_gimbal = false;
	}

	~SimpleIRCamera() override = default;

	void set_optical_zoom_level(int level) override {
		std::cout << "set_optical_zoom_level (IR)" << std::endl;
	}
};

class SimpleLRF : public LRF {
public:
	SimpleLRF() : LRF() {}
	~SimpleLRF() override = default;
};

class SimpleStream : public Stream {
public:
	explicit SimpleStream(bool snow) : Stream("Simple Stream") {
		auto m_eo_camera = std::make_shared<SimpleEOCamera>();
		auto m_ir_camera = std::make_shared<SimpleIRCamera>();

		register_camera(m_eo_camera);
		register_camera(m_ir_camera);

		m_snow = snow;
	}

	~SimpleStream() override = default;

	void start_stream(unsigned int endpoint_idx, std::function<void(std::string)> set_alive_callback) override {
		gst_init(nullptr, nullptr);

		const char *pipeline_str_base;
		if (m_snow) {
			pipeline_str_base = "videotestsrc pattern=snow ! x264enc ! rtspclientsink location=rtsp://0.0.0.0:8554/";
		} else {
			pipeline_str_base = "videotestsrc pattern=blink ! x264enc ! rtspclientsink location=rtsp://0.0.0.0:8554/";
		}
		
		std::ostringstream sstream;
		sstream << pipeline_str_base;
		sstream << endpoint_idx;

		std::string pipeline_description = sstream.str();

		GError *error = nullptr;
		GstElement *pipeline = gst_parse_launch(pipeline_description.c_str(), &error);
		if (error) {
			std::cerr << "Failed to create pipeline: " << error->message << std::endl;
			g_clear_error(&error);
			return;
		}

		GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
		if (ret == GST_STATE_CHANGE_FAILURE) {
			std::cerr << "Failed to set pipeline to playing state" << std::endl;
			gst_object_unref(pipeline);
			return;
		}

		set_alive_callback(get_uuid());

		GMainLoop *main_loop = g_main_loop_new(nullptr, FALSE);
		g_main_loop_run(main_loop);

		gst_element_set_state(pipeline, GST_STATE_NULL);
		gst_object_unref(pipeline);
		g_main_loop_unref(main_loop);
	}

	void set_active_camera(unsigned int camera_idx) override {
	}

private:
	bool m_snow;
};

class SimplePayload : public Payload {
public:
	SimplePayload(std::shared_ptr<Logger> log) : Payload { "Simple Payload", SIMPLE_PAYLOAD_VERSION }, log {std::move( log )} {
		auto m_gimbal = std::make_shared<SimpleGimbal>();
		register_gimbal(m_gimbal);

		auto m_lrf = std::make_shared<SimpleLRF>();
		register_lrf(m_lrf);

		auto m_stream = std::make_shared<SimpleStream>(true);
		register_stream(m_stream);

		auto m_stream_2 = std::make_shared<SimpleStream>(false);
		register_stream(m_stream_2);
	}

	~SimplePayload() override = default;

private:
	std::shared_ptr<Logger> log;
};

int main(int argc, char** argv) {
	auto log = std::make_shared<Logger>("simple_payload.log");

	auto drone = DroneFactory::create_drone();

	auto payload = std::make_shared<SimplePayload>(log);

	std::cout << "SDK VERSION: " << FLYBYSDK_VERSION << std::endl;

	drone->register_payload(payload);

	drone->start_server();
}

