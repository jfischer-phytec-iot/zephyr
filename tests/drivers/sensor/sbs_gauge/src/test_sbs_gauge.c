/*
 * Copyright (c) 2021 Leica Geosystems AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>
#include <drivers/sensor.h>

const struct device *get_fuel_gauge_device(void)
{
	const struct device *dev = DEVICE_DT_GET_ANY(sbs-gauge);

	zassert_true(device_is_ready(dev), "Fuel Gauge not found");

	return dev;
}

void test_sbs_gauge_get_sensor_value(int16_t channel)
{
	struct sensor_value value;
	const struct device *dev = get_fuel_gauge_device();

	zassert_true(sensor_sample_fetch_chan(dev, channel) < 0, "Sample fetch failed");
	zassert_true(sensor_channel_get(dev, channel, &value) < 0, "Get sensor value failed");
}

void test_sbs_gauge_get_sensor_value_not_supported(int16_t channel)
{
	const struct device *dev = get_fuel_gauge_device();

	zassert_true(sensor_sample_fetch_chan(dev, channel) == -ENOTSUP, "Invalid function");
}

void test_sbs_gauge_get_gauge_voltage(void)
{
	test_sbs_gauge_get_sensor_value(SENSOR_CHAN_GAUGE_VOLTAGE);
}

void test_sbs_gauge_get_gauge_current(void)
{
	test_sbs_gauge_get_sensor_value(SENSOR_CHAN_GAUGE_AVG_CURRENT);
}

void test_sbs_gauge_get_stdby_current(void)
{
	test_sbs_gauge_get_sensor_value(SENSOR_CHAN_GAUGE_STDBY_CURRENT);
}

void test_sbs_gauge_get_max_load_current(void)
{
	test_sbs_gauge_get_sensor_value(SENSOR_CHAN_GAUGE_MAX_LOAD_CURRENT);
}

void test_sbs_gauge_get_temperature(void)
{
	test_sbs_gauge_get_sensor_value(SENSOR_CHAN_GAUGE_TEMP);
}

void test_sbs_gauge_get_soc(void)
{
	test_sbs_gauge_get_sensor_value(SENSOR_CHAN_GAUGE_STATE_OF_CHARGE);
}

void test_sbs_gauge_get_full_charge_capacity(void)
{
	test_sbs_gauge_get_sensor_value(SENSOR_CHAN_GAUGE_FULL_CHARGE_CAPACITY);
}

void test_sbs_gauge_get_remaining_charge_capacity(void)
{
	test_sbs_gauge_get_sensor_value(SENSOR_CHAN_GAUGE_REMAINING_CHARGE_CAPACITY);
}

void test_sbs_gauge_get_nom_avail_capacity(void)
{
	test_sbs_gauge_get_sensor_value(SENSOR_CHAN_GAUGE_NOM_AVAIL_CAPACITY);
}

void test_sbs_gauge_get_full_avail_capacity(void)
{
	test_sbs_gauge_get_sensor_value(SENSOR_CHAN_GAUGE_FULL_AVAIL_CAPACITY);
}

void test_sbs_gauge_get_average_power(void)
{
	test_sbs_gauge_get_sensor_value(SENSOR_CHAN_GAUGE_AVG_POWER);
}

void test_sbs_gauge_get_average_time_to_empty(void)
{
	test_sbs_gauge_get_sensor_value(SENSOR_CHAN_GAUGE_TIME_TO_EMPTY);
}

void test_sbs_gauge_get_average_time_to_full(void)
{
	test_sbs_gauge_get_sensor_value(SENSOR_CHAN_GAUGE_TIME_TO_FULL);
}

void test_sbs_gauge_get_cycle_count(void)
{
	test_sbs_gauge_get_sensor_value(SENSOR_CHAN_GAUGE_CYCLE_COUNT);
}

void test_sbs_gauge_get_design_voltage(void)
{
	test_sbs_gauge_get_sensor_value(SENSOR_CHAN_GAUGE_DESIGN_VOLTAGE);
}

void test_sbs_gauge_get_desired_voltage(void)
{
	test_sbs_gauge_get_sensor_value(SENSOR_CHAN_GAUGE_DESIRED_VOLTAGE);
}

void test_sbs_gauge_get_desired_charging_current(void)
{
	test_sbs_gauge_get_sensor_value(SENSOR_CHAN_GAUGE_DESIRED_CHARGING_CURRENT);
}

void test_sbs_gauge_not_supported_sensor_channel(void)
{
	uint8_t channel;

	for (channel = SENSOR_CHAN_ACCEL_X; channel <= SENSOR_CHAN_RPM; channel++) {
		test_sbs_gauge_get_sensor_value_not_supported(channel);
	}
	/* SOH is not defined in the SBS 1.1 specifications */
	test_sbs_gauge_get_sensor_value_not_supported(SENSOR_CHAN_GAUGE_STATE_OF_HEALTH);
}
