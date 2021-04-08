/*
 * Copyright (c) 2021 Leica Geosystems AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TEST_SBS_GAUGE_H_
#define __TEST_SBS_GAUGE_H_

const struct device *get_fuel_gauge_device(void);

void test_sbs_gauge_get_sensor_value(int16_t channel);
void test_sbs_gauge_get_sensor_value_not_supported(int16_t channel);

void test_sbs_gauge_get_gauge_voltage(void);
void test_sbs_gauge_get_gauge_current(void);
void test_sbs_gauge_get_stdby_current(void);
void test_sbs_gauge_get_max_load_current(void);
void test_sbs_gauge_get_temperature(void);
void test_sbs_gauge_get_soc(void);
void test_sbs_gauge_get_full_charge_capacity(void);
void test_sbs_gauge_get_remaining_charge_capacity(void);
void test_sbs_gauge_get_nom_avail_capacity(void);
void test_sbs_gauge_get_full_avail_capacity(void);
void test_sbs_gauge_get_average_power(void);
void test_sbs_gauge_get_average_time_to_empty(void);
void test_sbs_gauge_get_average_time_to_full(void);
void test_sbs_gauge_get_cycle_count(void);
void test_sbs_gauge_get_design_voltage(void);
void test_sbs_gauge_get_desired_voltage(void);
void test_sbs_gauge_get_desired_charging_current(void);

void test_sbs_gauge_not_supported_sensor_channel(void);

#endif /* __TEST_SBS_GAUGE_H_ */
