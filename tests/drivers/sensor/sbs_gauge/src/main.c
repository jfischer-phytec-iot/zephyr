/*
 * Copyright (c) 2021 Leica Geosystems AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

#include "test_sbs_gauge.h"

void test_main(void)
{
	ztest_test_suite(framework_tests,
		ztest_unit_test(test_sbs_gauge_get_gauge_voltage),
		ztest_unit_test(test_sbs_gauge_get_gauge_current),
		ztest_unit_test(test_sbs_gauge_get_stdby_current),
		ztest_unit_test(test_sbs_gauge_get_max_load_current),
		ztest_unit_test(test_sbs_gauge_get_temperature),
		ztest_unit_test(test_sbs_gauge_get_soc),
		ztest_unit_test(test_sbs_gauge_get_full_charge_capacity),
		ztest_unit_test(test_sbs_gauge_get_remaining_charge_capacity),
		ztest_unit_test(test_sbs_gauge_get_nom_avail_capacity),
		ztest_unit_test(test_sbs_gauge_get_full_avail_capacity),
		ztest_unit_test(test_sbs_gauge_get_average_power),
		ztest_unit_test(test_sbs_gauge_get_average_time_to_empty),
		ztest_unit_test(test_sbs_gauge_get_average_time_to_full),
		ztest_unit_test(test_sbs_gauge_get_cycle_count),
		ztest_unit_test(test_sbs_gauge_get_design_voltage),
		ztest_unit_test(test_sbs_gauge_get_desired_voltage),
		ztest_unit_test(test_sbs_gauge_get_desired_charging_current),
		ztest_unit_test(test_sbs_gauge_not_supported_sensor_channel)
	);

	ztest_run_test_suite(framework_tests);
}
