/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *
 * @brief Zigbee application template.
 */

#include <zephyr.h>
#include <logging/log.h>
#include <dk_buttons_and_leds.h>

#include <zboss_api.h>
#include <zigbee/zigbee_error_handler.h>
#include <zigbee/zigbee_app_utils.h>
#include <zb_nrf_platform.h>
#include <addons/zcl/zb_zcl_temp_measurement_addons.h>

#include "zb_ha_pressure_sensor.h"

/* Device endpoint, used to receive ZCL commands. */
#define TEMPERATURE_SENSOR_ENDPOINT           13
#define PRESSURE_SENSOR_ENDPOINT              14

#define WEATHER_STATION_UPDATE_ATTR_INTERVAL  (ZB_TIME_ONE_SECOND * 15)

/* Type of power sources available for the device.
 * For possible values see section 3.2.2.2.8 of ZCL specification.
 */
#define BULB_INIT_BASIC_POWER_SOURCE    ZB_ZCL_BASIC_POWER_SOURCE_DC_SOURCE

/* LED indicating that device successfully joined Zigbee network. */
#define ZIGBEE_NETWORK_STATE_LED        DK_LED3

/* LED used for device identification. */
#define IDENTIFY_LED                    DK_LED4

/* Button used to enter the Identify mode. */
#define IDENTIFY_MODE_BUTTON            DK_BTN4_MSK


LOG_MODULE_REGISTER(app);

zb_zcl_temp_measurement_attrs_t temp_attr;
zb_zcl_pressure_measurement_attrs_t pres_attr;


/* Main application customizable context.
 * Stores all settings and static values.
 */
struct zb_device_ctx {
	zb_zcl_basic_attrs_t     basic_attr;
	zb_zcl_identify_attrs_t  identify_attr;
};

/* Zigbee device application context storage. */
static struct zb_device_ctx dev_ctx;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(
	identify_attr_list,
	&dev_ctx.identify_attr.identify_time);

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(
	basic_attr_list,
	&dev_ctx.basic_attr.zcl_version,
	&dev_ctx.basic_attr.power_source);

ZB_ZCL_DECLARE_TEMP_MEASUREMENT_ATTRIB_LIST (
	temp_sensor_attr_list,
	&temp_attr.measure_value,
	&temp_attr.min_measure_value,
	&temp_attr.max_measure_value,
	&temp_attr.tolerance
);

ZB_ZCL_DECLARE_PRESSURE_MEASUREMENT_ATTRIB_LIST (
	pres_sensor_attr_list,
	&pres_attr.measure_value,
	&pres_attr.min_measure_value,
	&pres_attr.max_measure_value,
	&pres_attr.tolerance
);

ZB_HA_DECLARE_TEMPERATURE_SENSOR_CLUSTER_LIST(
	temp_sensor_cluster_list,
	basic_attr_list,
	identify_attr_list,
	temp_sensor_attr_list
);

ZB_HA_DECLARE_PRESSURE_SENSOR_CLUSTER_LIST(
	pres_sensor_cluster_list,
	basic_attr_list,
	identify_attr_list,
	pres_sensor_attr_list
);

ZB_HA_DECLARE_TEMPERATURE_SENSOR_EP(
	temp_sensor_ep,
	TEMPERATURE_SENSOR_ENDPOINT,
	temp_sensor_cluster_list
);

ZB_HA_DECLARE_PRESSURE_SENSOR_EP(
	pres_sensor_ep,
	PRESSURE_SENSOR_ENDPOINT,
	pres_sensor_cluster_list
);

ZBOSS_DECLARE_DEVICE_CTX_2_EP(
	weather_station_ctx,
	temp_sensor_ep,
	pres_sensor_ep);


/**@brief Function for initializing all clusters attributes. */
static void app_clusters_attr_init(void)
{
	/* Basic cluster attributes data */
	dev_ctx.basic_attr.zcl_version = ZB_ZCL_VERSION;
	dev_ctx.basic_attr.power_source = BULB_INIT_BASIC_POWER_SOURCE;

	/* Identify cluster attributes data. */
	dev_ctx.identify_attr.identify_time =
		ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;
}

/**@brief Function to toggle the identify LED
 *
 * @param  bufid  Unused parameter, required by ZBOSS scheduler API.
 */
static void toggle_identify_led(zb_bufid_t bufid)
{
	static int blink_status;

	dk_set_led(IDENTIFY_LED, (++blink_status) % 2);
	ZB_SCHEDULE_APP_ALARM(toggle_identify_led, bufid, ZB_MILLISECONDS_TO_BEACON_INTERVAL(100));
}

/**@brief Function to handle identify notification events on the first endpoint.
 *
 * @param  bufid  Unused parameter, required by ZBOSS scheduler API.
 */
static void identify_cb(zb_bufid_t bufid)
{
	zb_ret_t zb_err_code;

	if (bufid) {
		/* Schedule a self-scheduling function that will toggle the LED */
		ZB_SCHEDULE_APP_CALLBACK(toggle_identify_led, bufid);
	} else {
		/* Cancel the toggling function alarm and turn off LED */
		zb_err_code = ZB_SCHEDULE_APP_ALARM_CANCEL(toggle_identify_led, ZB_ALARM_ANY_PARAM);
		ZVUNUSED(zb_err_code);

		dk_set_led(IDENTIFY_LED, 0);
	}
}

/**@breif Starts identifying the device.
 *
 * @param  bufid  Unused parameter, required by ZBOSS scheduler API.
 */
static void start_identifying(zb_bufid_t bufid)
{
	zb_ret_t zb_err_code;

	ZVUNUSED(bufid);

	/* Check if endpoint is in identifying mode,
	 * if not put desired endpoint in identifying mode.
	 */
	if (dev_ctx.identify_attr.identify_time ==
	    ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE) {
		LOG_INF("Enter identify mode");
		zb_err_code = zb_bdb_finding_binding_target(
			TEMPERATURE_SENSOR_ENDPOINT);
		ZB_ERROR_CHECK(zb_err_code);
	} else {
		LOG_INF("Cancel identify mode");
		zb_bdb_finding_binding_target_cancel();
	}
}

/**@brief Callback for button events.
 *
 * @param[in]   button_state  Bitmask containing buttons state.
 * @param[in]   has_changed   Bitmask containing buttons
 *                            that have changed their state.
 */
static void button_changed(uint32_t button_state, uint32_t has_changed)
{
	/* Calculate bitmask of buttons that are pressed
	 * and have changed their state.
	 */
	uint32_t buttons = button_state & has_changed;

	if (buttons & IDENTIFY_MODE_BUTTON) {
		ZB_SCHEDULE_APP_CALLBACK(start_identifying, 0);
	}
}

/**@brief Function for initializing LEDs and Buttons. */
static void configure_gpio(void)
{
	int err;

	err = dk_buttons_init(button_changed);
	if (err) {
		LOG_ERR("Cannot init buttons (err: %d)", err);
	}

	err = dk_leds_init();
	if (err) {
		LOG_ERR("Cannot init LEDs (err: %d)", err);
	}
}

/**@brief Zigbee stack event handler.
 *
 * @param[in]   bufid   Reference to the Zigbee stack buffer
 *                      used to pass signal.
 */
void zboss_signal_handler(zb_bufid_t bufid)
{
	/* Update network status LED. */
	zigbee_led_status_update(bufid, ZIGBEE_NETWORK_STATE_LED);

	/* No application-specific behavior is required.
	 * Call default signal handler.
	 */
	ZB_ERROR_CHECK(zigbee_default_signal_handler(bufid));

	/* All callbacks should either reuse or free passed buffers.
	 * If bufid == 0, the buffer is invalid (not passed).
	 */
	if (bufid) {
		zb_buf_free(bufid);
	}
}

void error(void)
{
	dk_set_leds_state(DK_ALL_LEDS_MSK, DK_NO_LEDS_MSK);

	while (true) {
		/* Spin forever */
		k_sleep(K_MSEC(1000));
	}
}

static void pressure_sensor_attr_update(zb_uint8_t arg)
{
	static zb_int16_t temperature_value = 15;

	temperature_value += 5;

	zb_zcl_set_attr_val(TEMPERATURE_SENSOR_ENDPOINT, ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,
						ZB_ZCL_CLUSTER_SERVER_ROLE, ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID,
						(zb_uint8_t*)&temperature_value, ZB_FALSE);
	ZB_SCHEDULE_APP_ALARM(pressure_sensor_attr_update, NULL, WEATHER_STATION_UPDATE_ATTR_INTERVAL);
}

static void temperature_sensor_attr_update(zb_uint8_t arg)
{
	static zb_int16_t pressure_value = 15;

	pressure_value += 13;

	zb_zcl_set_attr_val(PRESSURE_SENSOR_ENDPOINT, ZB_ZCL_CLUSTER_ID_PRESSURE_MEASUREMENT,
						ZB_ZCL_CLUSTER_SERVER_ROLE, ZB_ZCL_ATTR_PRESSURE_MEASUREMENT_VALUE_ID,
						(zb_uint8_t*)&pressure_value, ZB_FALSE);
	ZB_SCHEDULE_APP_ALARM(pressure_sensor_attr_update, NULL, WEATHER_STATION_UPDATE_ATTR_INTERVAL);
}

void main(void)
{
	LOG_INF("Starting Zigbee application template example");

	/* Initialize */
	configure_gpio();

	/* Register device context (endpoints). */
	ZB_AF_REGISTER_DEVICE_CTX(&weather_station_ctx);

	app_clusters_attr_init();

	/* Register handlers to identify notifications */
	ZB_AF_SET_IDENTIFY_NOTIFICATION_HANDLER(TEMPERATURE_SENSOR_ENDPOINT, identify_cb);

	pressure_sensor_attr_update(0);

	temperature_sensor_attr_update(0);

	/* Start Zigbee default thread */
	zigbee_enable();

	LOG_INF("Zigbee application template started");
}
