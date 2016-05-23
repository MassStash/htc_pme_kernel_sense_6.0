/*
 * Copyright (c) 2015, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define pr_fmt(fmt) "%s: " fmt, __func__

#include <linux/bitops.h>
#include <linux/debugfs.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pm_opp.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/of_regulator.h>
#include <linux/regulator/kryo-regulator.h>

#include "cpr3-regulator.h"

#define MSM8996_HMSS_FUSE_CORNERS	5

struct cpr3_msm8996_hmss_fuses {
	u64	ro_sel[MSM8996_HMSS_FUSE_CORNERS];
	u64	init_voltage[MSM8996_HMSS_FUSE_CORNERS];
	u64	target_quot[MSM8996_HMSS_FUSE_CORNERS];
	u64	quot_offset[MSM8996_HMSS_FUSE_CORNERS];
	u64	speed_bin;
	u64	cpr_fusing_rev;
	u64	redundant_fusing;
	u64	limitation;
	u64	partial_binning;
	u64	vdd_mx_ret_fuse;
	u64	vdd_apcc_ret_fuse;
	u64	aging_init_quot_diff;
};

#define CPR3_MSM8996_HMSS_FUSE_COMBO_COUNT	16

enum cpr3_msm8996_hmss_fuse_corner {
	CPR3_MSM8996_HMSS_FUSE_CORNER_MINSVS	= 0,
	CPR3_MSM8996_HMSS_FUSE_CORNER_LOWSVS	= 1,
	CPR3_MSM8996_HMSS_FUSE_CORNER_SVS	= 2,
	CPR3_MSM8996_HMSS_FUSE_CORNER_NOM	= 3,
	CPR3_MSM8996_HMSS_FUSE_CORNER_TURBO	= 4,
};

static const char * const cpr3_msm8996_hmss_fuse_corner_name[] = {
	[CPR3_MSM8996_HMSS_FUSE_CORNER_MINSVS]	= "MinSVS",
	[CPR3_MSM8996_HMSS_FUSE_CORNER_LOWSVS]	= "LowSVS",
	[CPR3_MSM8996_HMSS_FUSE_CORNER_SVS]	= "SVS",
	[CPR3_MSM8996_HMSS_FUSE_CORNER_NOM]	= "NOM",
	[CPR3_MSM8996_HMSS_FUSE_CORNER_TURBO]	= "TURBO",
};

#define MSM8996_HMSS_POWER_CLUSTER_THREAD_ID		0
#define MSM8996_HMSS_PERFORMANCE_CLUSTER_THREAD_ID	1

static const struct cpr3_fuse_param
msm8996_hmss_ro_sel_param[2][MSM8996_HMSS_FUSE_CORNERS][2] = {
	[MSM8996_HMSS_POWER_CLUSTER_THREAD_ID] = {
		{{66, 38, 41}, {} },
		{{66, 38, 41}, {} },
		{{66, 38, 41}, {} },
		{{66, 34, 37}, {} },
		{{66, 30, 33}, {} },
	},
	[MSM8996_HMSS_PERFORMANCE_CLUSTER_THREAD_ID] = {
		{{64, 54, 57}, {} },
		{{64, 54, 57}, {} },
		{{64, 54, 57}, {} },
		{{64, 50, 53}, {} },
		{{64, 46, 49}, {} },
	},
};

static const struct cpr3_fuse_param
msm8996_hmss_init_voltage_param[2][MSM8996_HMSS_FUSE_CORNERS][3] = {
	[MSM8996_HMSS_POWER_CLUSTER_THREAD_ID] = {
		{{67,  0,  5}, {} },
		{{66, 58, 63}, {} },
		{{66, 58, 63}, {} },
		{{66, 52, 57}, {} },
		{{66, 46, 51}, {} },
	},
	[MSM8996_HMSS_PERFORMANCE_CLUSTER_THREAD_ID] = {
		{{65, 16, 21}, {} },
		{{65, 10, 15}, {} },
		{{65, 10, 15}, {} },
		{{65,  4,  9}, {} },
		{{64, 62, 63}, {65,  0,  3}, {} },
	},
};

static const struct cpr3_fuse_param
msm8996_hmss_target_quot_param[2][MSM8996_HMSS_FUSE_CORNERS][3] = {
	[MSM8996_HMSS_POWER_CLUSTER_THREAD_ID] = {
		{{67, 42, 53}, {} },
		{{67, 30, 41}, {} },
		{{67, 30, 41}, {} },
		{{67, 18, 29}, {} },
		{{67,  6, 17}, {} },
	},
	[MSM8996_HMSS_PERFORMANCE_CLUSTER_THREAD_ID] = {
		{{65, 58, 63}, {66,  0,  5}, {} },
		{{65, 46, 57}, {} },
		{{65, 46, 57}, {} },
		{{65, 34, 45}, {} },
		{{65, 22, 33}, {} },
	},
};

static const struct cpr3_fuse_param
msm8996_hmss_quot_offset_param[2][MSM8996_HMSS_FUSE_CORNERS][3] = {
	[MSM8996_HMSS_POWER_CLUSTER_THREAD_ID] = {
		{{} },
		{{} },
		{{68,  6, 13}, {} },
		{{67, 62, 63}, {68, 0, 5}, {} },
		{{67, 54, 61}, {} },
	},
	[MSM8996_HMSS_PERFORMANCE_CLUSTER_THREAD_ID] = {
		{{} },
		{{} },
		{{66, 22, 29}, {} },
		{{66, 14, 21}, {} },
		{{66,  6, 13}, {} },
	},
};

static const struct cpr3_fuse_param msm8996_redundant_fusing_param[] = {
	{73, 61, 63},
	{},
};
#define MSM8996_CPR_REDUNDANT_FUSING 1

static const struct cpr3_fuse_param
msm8996_hmss_redun_ro_sel_param[2][MSM8996_HMSS_FUSE_CORNERS][2] = {
	[MSM8996_HMSS_POWER_CLUSTER_THREAD_ID] = {
		{{76, 36, 39}, {} },
		{{76, 32, 35}, {} },
		{{76, 32, 35}, {} },
		{{76, 28, 31}, {} },
		{{76, 24, 27}, {} },
	},
	[MSM8996_HMSS_PERFORMANCE_CLUSTER_THREAD_ID] = {
		{{74, 52, 55}, {} },
		{{74, 48, 51}, {} },
		{{74, 48, 51}, {} },
		{{74, 44, 47}, {} },
		{{74, 40, 43}, {} },
	},
};

static const struct cpr3_fuse_param
msm8996_hmss_redun_init_voltage_param[2][MSM8996_HMSS_FUSE_CORNERS][3] = {
	[MSM8996_HMSS_POWER_CLUSTER_THREAD_ID] = {
		{{76, 58, 63}, {} },
		{{76, 52, 57}, {} },
		{{76, 52, 57}, {} },
		{{76, 46, 51}, {} },
		{{76, 40, 45}, {} },
	},
	[MSM8996_HMSS_PERFORMANCE_CLUSTER_THREAD_ID] = {
		{{75, 10, 15}, {} },
		{{75,  4,  9}, {} },
		{{75,  4,  9}, {} },
		{{74, 62, 63}, {75,  0,  3}, {} },
		{{74, 56, 61}, {} },
	},
};

static const struct cpr3_fuse_param
msm8996_hmss_redun_target_quot_param[2][MSM8996_HMSS_FUSE_CORNERS][2] = {
	[MSM8996_HMSS_POWER_CLUSTER_THREAD_ID] = {
		{{77, 36, 47}, {} },
		{{77, 24, 35}, {} },
		{{77, 24, 35}, {} },
		{{77, 12, 23}, {} },
		{{77,  0, 11}, {} },
	},
	[MSM8996_HMSS_PERFORMANCE_CLUSTER_THREAD_ID] = {
		{{75, 52, 63}, {} },
		{{75, 40, 51}, {} },
		{{75, 40, 51}, {} },
		{{75, 28, 39}, {} },
		{{75, 16, 27}, {} },
	},
};

static const struct cpr3_fuse_param
msm8996_hmss_redun_quot_offset_param[2][MSM8996_HMSS_FUSE_CORNERS][2] = {
	[MSM8996_HMSS_POWER_CLUSTER_THREAD_ID] = {
		{{} },
		{{} },
		{{68, 11, 18}, {} },
		{{77, 56, 63}, {} },
		{{77, 48, 55}, {} },
	},
	[MSM8996_HMSS_PERFORMANCE_CLUSTER_THREAD_ID] = {
		{{} },
		{{} },
		{{76, 16, 23}, {} },
		{{76,  8, 15}, {} },
		{{76,  0,  7}, {} },
	},
};

static const struct cpr3_fuse_param msm8996_cpr_fusing_rev_param[] = {
	{39, 51, 53},
	{},
};

static const struct cpr3_fuse_param msm8996_hmss_speed_bin_param[] = {
	{38, 29, 31},
	{},
};

static const struct cpr3_fuse_param msm8996_cpr_limitation_param[] = {
	{41, 31, 32},
	{},
};

static const struct cpr3_fuse_param msm8996_vdd_mx_ret_param[] = {
	{41, 2, 4},
	{},
};

static const struct cpr3_fuse_param msm8996_vdd_apcc_ret_param[] = {
	{41, 52, 54},
	{},
};

static const struct cpr3_fuse_param msm8996_cpr_partial_binning_param[] = {
	{39, 55, 59},
	{},
};

static const struct cpr3_fuse_param
msm8996_hmss_aging_init_quot_diff_param[] = {
	{68, 14, 19},
	{},
};

enum msm8996_cpr_limitation {
	MSM8996_CPR_LIMITATION_NONE = 0,
	MSM8996_CPR_LIMITATION_UNSUPPORTED = 2,
	MSM8996_CPR_LIMITATION_NO_CPR_OR_INTERPOLATION = 3,
};

enum msm8996_cpr_partial_binning {
	MSM8996_CPR_PARTIAL_BINNING_SVS = 11,
	MSM8996_CPR_PARTIAL_BINNING_NOM = 12,
};


static const int msm8996_v1_v2_hmss_fuse_ref_volt[MSM8996_HMSS_FUSE_CORNERS] = {
	605000,
	745000, 
	745000,
	905000,
	1015000,
};

static const int msm8996_v3_hmss_fuse_ref_volt[MSM8996_HMSS_FUSE_CORNERS] = {
	605000,
	745000, 
	745000,
	905000,
	1140000,
};

static const int msm8996_v3_speed_bin1_rev5_hmss_fuse_ref_volt[
						MSM8996_HMSS_FUSE_CORNERS] = {
	605000,
	745000, 
	745000,
	905000,
	1040000,
};

static const int msm8996_vdd_apcc_fuse_ret_volt[] = {
	600000, 550000, 500000, 450000, 400000, 350000, 300000, 600000,
};

static const int msm8996_vdd_mx_fuse_ret_volt[] = {
	700000, 650000, 580000, 550000, 490000, 490000, 490000, 490000,
};

#define MSM8996_HMSS_FUSE_STEP_VOLT		10000
#define MSM8996_HMSS_VOLTAGE_FUSE_SIZE		6
#define MSM8996_HMSS_QUOT_OFFSET_SCALE		5
#define MSM8996_HMSS_AGING_INIT_QUOT_DIFF_SCALE	2
#define MSM8996_HMSS_AGING_INIT_QUOT_DIFF_SIZE	6

#define MSM8996_HMSS_CPR_SENSOR_COUNT		25
#define MSM8996_HMSS_THREAD0_SENSOR_MIN		0
#define MSM8996_HMSS_THREAD0_SENSOR_MAX		14
#define MSM8996_HMSS_THREAD1_SENSOR_MIN		15
#define MSM8996_HMSS_THREAD1_SENSOR_MAX		24

#define MSM8996_HMSS_CPR_CLOCK_RATE		19200000

#define MSM8996_HMSS_AGING_SENSOR_ID		11
#define MSM8996_HMSS_AGING_BYPASS_MASK0		(GENMASK(7, 0) & ~BIT(3))

static int cpr3_msm8996_hmss_read_fuse_data(struct cpr3_regulator *vreg)
{
	void __iomem *base = vreg->thread->ctrl->fuse_base;
	struct cpr3_msm8996_hmss_fuses *fuse;
	bool redundant;
	int i, id, rc;

	fuse = devm_kzalloc(vreg->thread->ctrl->dev, sizeof(*fuse), GFP_KERNEL);
	if (!fuse)
		return -ENOMEM;

	rc = cpr3_read_fuse_param(base, msm8996_hmss_speed_bin_param,
				&fuse->speed_bin);
	if (rc) {
		cpr3_err(vreg, "Unable to read speed bin fuse, rc=%d\n", rc);
		return rc;
	}
	cpr3_info(vreg, "speed bin = %llu\n", fuse->speed_bin);

	rc = cpr3_read_fuse_param(base, msm8996_cpr_fusing_rev_param,
				&fuse->cpr_fusing_rev);
	if (rc) {
		cpr3_err(vreg, "Unable to read CPR fusing revision fuse, rc=%d\n",
			rc);
		return rc;
	}
	cpr3_info(vreg, "CPR fusing revision = %llu\n", fuse->cpr_fusing_rev);

	rc = cpr3_read_fuse_param(base, msm8996_redundant_fusing_param,
				&fuse->redundant_fusing);
	if (rc) {
		cpr3_err(vreg, "Unable to read redundant fusing config fuse, rc=%d\n",
			rc);
		return rc;
	}

	redundant = (fuse->redundant_fusing == MSM8996_CPR_REDUNDANT_FUSING);
	cpr3_info(vreg, "using redundant fuses = %c\n",
		redundant ? 'Y' : 'N');

	rc = cpr3_read_fuse_param(base, msm8996_cpr_limitation_param,
				&fuse->limitation);
	if (rc) {
		cpr3_err(vreg, "Unable to read CPR limitation fuse, rc=%d\n",
			rc);
		return rc;
	}
	cpr3_info(vreg, "CPR limitation = %s\n",
		fuse->limitation == MSM8996_CPR_LIMITATION_UNSUPPORTED
		? "unsupported chip" : fuse->limitation
			  == MSM8996_CPR_LIMITATION_NO_CPR_OR_INTERPOLATION
		? "CPR disabled and no interpolation" : "none");

	rc = cpr3_read_fuse_param(base, msm8996_cpr_partial_binning_param,
				&fuse->partial_binning);
	if (rc) {
		cpr3_err(vreg, "Unable to read partial binning fuse, rc=%d\n",
			rc);
		return rc;
	}
	cpr3_info(vreg, "CPR partial binning limitation = %s\n",
		fuse->partial_binning == MSM8996_CPR_PARTIAL_BINNING_SVS
			? "SVS min voltage"
		: fuse->partial_binning == MSM8996_CPR_PARTIAL_BINNING_NOM
			? "NOM min voltage"
		: "none");

	rc = cpr3_read_fuse_param(base, msm8996_vdd_mx_ret_param,
				&fuse->vdd_mx_ret_fuse);
	if (rc) {
		cpr3_err(vreg, "Unable to read VDD_MX retention fuse, rc=%d\n",
			rc);
		return rc;
	}

	rc = cpr3_read_fuse_param(base, msm8996_vdd_apcc_ret_param,
				&fuse->vdd_apcc_ret_fuse);
	if (rc) {
		cpr3_err(vreg, "Unable to read VDD_APCC retention fuse, rc=%d\n",
			rc);
		return rc;
	}

	cpr3_info(vreg, "Retention voltage fuses: VDD_MX = %llu, VDD_APCC = %llu\n",
		  fuse->vdd_mx_ret_fuse, fuse->vdd_apcc_ret_fuse);

	rc = cpr3_read_fuse_param(base, msm8996_hmss_aging_init_quot_diff_param,
				&fuse->aging_init_quot_diff);
	if (rc) {
		cpr3_err(vreg, "Unable to read aging initial quotient difference fuse, rc=%d\n",
			rc);
		return rc;
	}

	id = vreg->thread->thread_id;

	for (i = 0; i < MSM8996_HMSS_FUSE_CORNERS; i++) {
		rc = cpr3_read_fuse_param(base,
			redundant
			    ? msm8996_hmss_redun_init_voltage_param[id][i]
			    : msm8996_hmss_init_voltage_param[id][i],
			&fuse->init_voltage[i]);
		if (rc) {
			cpr3_err(vreg, "Unable to read fuse-corner %d initial voltage fuse, rc=%d\n",
				i, rc);
			return rc;
		}

		rc = cpr3_read_fuse_param(base,
			redundant
			    ? msm8996_hmss_redun_target_quot_param[id][i]
			    : msm8996_hmss_target_quot_param[id][i],
			&fuse->target_quot[i]);
		if (rc) {
			cpr3_err(vreg, "Unable to read fuse-corner %d target quotient fuse, rc=%d\n",
				i, rc);
			return rc;
		}

		rc = cpr3_read_fuse_param(base,
			redundant
			    ? msm8996_hmss_redun_ro_sel_param[id][i]
			    : msm8996_hmss_ro_sel_param[id][i],
			&fuse->ro_sel[i]);
		if (rc) {
			cpr3_err(vreg, "Unable to read fuse-corner %d RO select fuse, rc=%d\n",
				i, rc);
			return rc;
		}

		rc = cpr3_read_fuse_param(base,
			redundant
			    ? msm8996_hmss_redun_quot_offset_param[id][i]
			    : msm8996_hmss_quot_offset_param[id][i],
			&fuse->quot_offset[i]);
		if (rc) {
			cpr3_err(vreg, "Unable to read fuse-corner %d quotient offset fuse, rc=%d\n",
				i, rc);
			return rc;
		}
	}

	vreg->fuse_combo = fuse->cpr_fusing_rev + 8 * fuse->speed_bin;
	if (vreg->fuse_combo >= CPR3_MSM8996_HMSS_FUSE_COMBO_COUNT) {
		cpr3_err(vreg, "invalid CPR fuse combo = %d found\n",
			vreg->fuse_combo);
		return -EINVAL;
	}

	vreg->speed_bin_fuse	= fuse->speed_bin;
	vreg->cpr_rev_fuse	= fuse->cpr_fusing_rev;
	vreg->fuse_corner_count	= MSM8996_HMSS_FUSE_CORNERS;
	vreg->platform_fuses	= fuse;

	return 0;
}

static int cpr3_hmss_parse_corner_data(struct cpr3_regulator *vreg)
{
	int rc;

	rc = cpr3_parse_common_corner_data(vreg);
	if (rc) {
		cpr3_err(vreg, "error reading corner data, rc=%d\n", rc);
		return rc;
	}

	return rc;
}

static int cpr3_msm8996_hmss_calculate_open_loop_voltages(
			struct cpr3_regulator *vreg)
{
	struct device_node *node = vreg->of_node;
	struct cpr3_msm8996_hmss_fuses *fuse = vreg->platform_fuses;
	int rc = 0;
	bool allow_interpolation;
	u64 freq_low, volt_low, freq_high, volt_high;
	int i, j, soc_revision;
	const int *ref_volt;
	int *fuse_volt;
	int *fmax_corner;

	fuse_volt = kcalloc(vreg->fuse_corner_count, sizeof(*fuse_volt),
				GFP_KERNEL);
	fmax_corner = kcalloc(vreg->fuse_corner_count, sizeof(*fmax_corner),
				GFP_KERNEL);
	if (!fuse_volt || !fmax_corner) {
		rc = -ENOMEM;
		goto done;
	}

	soc_revision = vreg->thread->ctrl->soc_revision;
	if (soc_revision == 1 || soc_revision == 2)
		ref_volt = msm8996_v1_v2_hmss_fuse_ref_volt;
	else if (fuse->speed_bin == 1 && fuse->cpr_fusing_rev >= 5)
		ref_volt = msm8996_v3_speed_bin1_rev5_hmss_fuse_ref_volt;
	else
		ref_volt = msm8996_v3_hmss_fuse_ref_volt;

	for (i = 0; i < vreg->fuse_corner_count; i++) {
		fuse_volt[i] = cpr3_convert_open_loop_voltage_fuse(
			ref_volt[i],
			MSM8996_HMSS_FUSE_STEP_VOLT, fuse->init_voltage[i],
			MSM8996_HMSS_VOLTAGE_FUSE_SIZE);

		
		if (i != CPR3_MSM8996_HMSS_FUSE_CORNER_LOWSVS)
			cpr3_info(vreg, "fused %6s: open-loop=%7d uV\n",
				cpr3_msm8996_hmss_fuse_corner_name[i],
				fuse_volt[i]);
	}

	rc = cpr3_adjust_fused_open_loop_voltages(vreg, fuse_volt);
	if (rc) {
		cpr3_err(vreg, "fused open-loop voltage adjustment failed, rc=%d\n",
			rc);
		goto done;
	}

	allow_interpolation = of_property_read_bool(node,
				"qcom,allow-voltage-interpolation");

	fuse_volt[CPR3_MSM8996_HMSS_FUSE_CORNER_LOWSVS]
		= fuse_volt[CPR3_MSM8996_HMSS_FUSE_CORNER_SVS];

	for (i = 1; i < vreg->fuse_corner_count; i++) {
		if (fuse_volt[i] < fuse_volt[i - 1]) {
			cpr3_debug(vreg, "fuse corner %d voltage=%d uV < fuse corner %d voltage=%d uV; overriding: fuse corner %d voltage=%d\n",
				i, fuse_volt[i], i - 1, fuse_volt[i - 1],
				i, fuse_volt[i - 1]);
			fuse_volt[i] = fuse_volt[i - 1];
		}
	}

	if (fuse->limitation == MSM8996_CPR_LIMITATION_NO_CPR_OR_INTERPOLATION)
		allow_interpolation = false;

	if (!allow_interpolation) {
		
		for (i = 0; i < vreg->corner_count; i++)
			vreg->corner[i].open_loop_volt
				= fuse_volt[vreg->corner[i].cpr_fuse_corner];
		goto done;
	}

	
	j = vreg->fuse_corner_count - 1;
	for (i = vreg->corner_count - 1; i >= 0; i--) {
		if (vreg->corner[i].cpr_fuse_corner == j) {
			fmax_corner[j] = i;
			j--;
		}
	}
	if (j >= 0) {
		cpr3_err(vreg, "invalid fuse corner mapping\n");
		rc = -EINVAL;
		goto done;
	}

	for (i = 0; i <= fmax_corner[0]; i++)
		vreg->corner[i].open_loop_volt = fuse_volt[0];

	for (i = CPR3_MSM8996_HMSS_FUSE_CORNER_LOWSVS;
	     i < vreg->fuse_corner_count - 1; i++) {
		fmax_corner[i] = fmax_corner[i + 1];
		fuse_volt[i] = fuse_volt[i + 1];
	}

	
	for (i = 1; i < vreg->fuse_corner_count - 1; i++) {
		freq_low = vreg->corner[fmax_corner[i - 1]].proc_freq;
		volt_low = fuse_volt[i - 1];
		freq_high = vreg->corner[fmax_corner[i]].proc_freq;
		volt_high = fuse_volt[i];

		for (j = fmax_corner[i - 1] + 1; j <= fmax_corner[i]; j++)
			vreg->corner[j].open_loop_volt = cpr3_interpolate(
				freq_low, volt_low, freq_high, volt_high,
				vreg->corner[j].proc_freq);
	}

done:
	if (rc == 0) {
		cpr3_debug(vreg, "unadjusted per-corner open-loop voltages:\n");
		for (i = 0; i < vreg->corner_count; i++)
			cpr3_debug(vreg, "open-loop[%2d] = %d uV\n", i,
				vreg->corner[i].open_loop_volt);

		rc = cpr3_adjust_open_loop_voltages(vreg);
		if (rc)
			cpr3_err(vreg, "open-loop voltage adjustment failed, rc=%d\n",
				rc);
	}

	kfree(fuse_volt);
	kfree(fmax_corner);
	return rc;
}

static int cpr3_msm8996_hmss_set_no_interpolation_quotients(
			struct cpr3_regulator *vreg, int *volt_adjust,
			int *volt_adjust_fuse, int *ro_scale)
{
	struct cpr3_msm8996_hmss_fuses *fuse = vreg->platform_fuses;
	u32 quot, ro;
	int quot_adjust;
	int i, fuse_corner;

	for (i = 0; i < vreg->corner_count; i++) {
		fuse_corner = vreg->corner[i].cpr_fuse_corner;
		quot = fuse->target_quot[fuse_corner];
		quot_adjust = cpr3_quot_adjustment(ro_scale[fuse_corner],
				volt_adjust_fuse[fuse_corner] + volt_adjust[i]);
		ro = fuse->ro_sel[fuse_corner];
		vreg->corner[i].target_quot[ro] = quot + quot_adjust;
		if (quot_adjust)
			cpr3_debug(vreg, "adjusted corner %d RO%u target quot: %u --> %u (%d uV)\n",
				i, ro, quot, vreg->corner[i].target_quot[ro],
				volt_adjust_fuse[fuse_corner] + volt_adjust[i]);
	}

	return 0;
}

static int cpr3_msm8996_hmss_calculate_target_quotients(
			struct cpr3_regulator *vreg)
{
	struct cpr3_msm8996_hmss_fuses *fuse = vreg->platform_fuses;
	int rc;
	bool allow_interpolation;
	u64 freq_low, freq_high, prev_quot;
	u64 *quot_low;
	u64 *quot_high;
	u32 quot, ro;
	int i, j, fuse_corner, quot_adjust;
	int *fmax_corner;
	int *volt_adjust, *volt_adjust_fuse, *ro_scale;

	
	cpr3_info(vreg, "fused MinSVS: quot[%2llu]=%4llu\n",
		fuse->ro_sel[CPR3_MSM8996_HMSS_FUSE_CORNER_MINSVS],
		fuse->target_quot[CPR3_MSM8996_HMSS_FUSE_CORNER_MINSVS]);
	for (i = CPR3_MSM8996_HMSS_FUSE_CORNER_SVS;
	     i <= CPR3_MSM8996_HMSS_FUSE_CORNER_TURBO; i++)
		cpr3_info(vreg, "fused %6s: quot[%2llu]=%4llu, quot_offset[%2llu]=%4llu\n",
			cpr3_msm8996_hmss_fuse_corner_name[i],
			fuse->ro_sel[i], fuse->target_quot[i], fuse->ro_sel[i],
			fuse->quot_offset[i] * MSM8996_HMSS_QUOT_OFFSET_SCALE);

	allow_interpolation = of_property_read_bool(vreg->of_node,
					"qcom,allow-quotient-interpolation");

	if (fuse->limitation == MSM8996_CPR_LIMITATION_NO_CPR_OR_INTERPOLATION)
		allow_interpolation = false;

	volt_adjust = kcalloc(vreg->corner_count, sizeof(*volt_adjust),
					GFP_KERNEL);
	volt_adjust_fuse = kcalloc(vreg->fuse_corner_count,
					sizeof(*volt_adjust_fuse), GFP_KERNEL);
	ro_scale = kcalloc(vreg->fuse_corner_count, sizeof(*ro_scale),
					GFP_KERNEL);
	fmax_corner = kcalloc(vreg->fuse_corner_count, sizeof(*fmax_corner),
					GFP_KERNEL);
	quot_low = kcalloc(vreg->fuse_corner_count, sizeof(*quot_low),
					GFP_KERNEL);
	quot_high = kcalloc(vreg->fuse_corner_count, sizeof(*quot_high),
					GFP_KERNEL);
	if (!volt_adjust || !volt_adjust_fuse || !ro_scale ||
	    !fmax_corner || !quot_low || !quot_high) {
		rc = -ENOMEM;
		goto done;
	}

	rc = cpr3_parse_closed_loop_voltage_adjustments(vreg, &fuse->ro_sel[0],
				volt_adjust, volt_adjust_fuse, ro_scale);
	if (rc) {
		cpr3_err(vreg, "could not load closed-loop voltage adjustments, rc=%d\n",
			rc);
		goto done;
	}

	if (!allow_interpolation) {
		
		return cpr3_msm8996_hmss_set_no_interpolation_quotients(vreg,
				volt_adjust, volt_adjust_fuse, ro_scale);
	}

	
	j = vreg->fuse_corner_count - 1;
	for (i = vreg->corner_count - 1; i >= 0; i--) {
		if (vreg->corner[i].cpr_fuse_corner == j) {
			fmax_corner[j] = i;
			j--;
		}
	}
	if (j >= 0) {
		cpr3_err(vreg, "invalid fuse corner mapping\n");
		rc = -EINVAL;
		goto done;
	}

	i = CPR3_MSM8996_HMSS_FUSE_CORNER_MINSVS;
	quot_adjust = cpr3_quot_adjustment(ro_scale[i], volt_adjust_fuse[i]);
	quot = fuse->target_quot[i] + quot_adjust;
	quot_high[i] = quot;
	ro = fuse->ro_sel[i];
	if (quot_adjust)
		cpr3_debug(vreg, "adjusted fuse corner %d RO%u target quot: %llu --> %u (%d uV)\n",
			i, ro, fuse->target_quot[i], quot, volt_adjust_fuse[i]);
	for (i = 0; i <= fmax_corner[CPR3_MSM8996_HMSS_FUSE_CORNER_MINSVS]; i++)
		vreg->corner[i].target_quot[ro] = quot;

	i = CPR3_MSM8996_HMSS_FUSE_CORNER_LOWSVS;
	quot_high[i] = fuse->target_quot[CPR3_MSM8996_HMSS_FUSE_CORNER_SVS]
			- fuse->quot_offset[CPR3_MSM8996_HMSS_FUSE_CORNER_SVS]
				* MSM8996_HMSS_QUOT_OFFSET_SCALE;
	quot_low[i] = fuse->target_quot[CPR3_MSM8996_HMSS_FUSE_CORNER_MINSVS];
	if (quot_high[i] < quot_low[i]) {
		cpr3_info(vreg, "quot_lowsvs=%llu < quot_minsvs=%llu; overriding: quot_lowsvs=%llu\n",
			quot_high[i], quot_low[i], quot_low[i]);
		quot_high[i] = quot_low[i];
	}
	if (fuse->ro_sel[CPR3_MSM8996_HMSS_FUSE_CORNER_MINSVS]
	    != fuse->ro_sel[CPR3_MSM8996_HMSS_FUSE_CORNER_SVS]) {
		cpr3_info(vreg, "MinSVS RO=%llu != SVS RO=%llu; disabling LowSVS interpolation\n",
			fuse->ro_sel[CPR3_MSM8996_HMSS_FUSE_CORNER_MINSVS],
			fuse->ro_sel[CPR3_MSM8996_HMSS_FUSE_CORNER_SVS]);
		quot_low[i] = quot_high[i];
	}

	for (i = CPR3_MSM8996_HMSS_FUSE_CORNER_SVS;
	     i < vreg->fuse_corner_count; i++) {
		quot_high[i] = fuse->target_quot[i];
		if (fuse->ro_sel[i] == fuse->ro_sel[i - 1])
			quot_low[i] = quot_high[i - 1];
		else
			quot_low[i] = quot_high[i]
					- fuse->quot_offset[i]
					  * MSM8996_HMSS_QUOT_OFFSET_SCALE;
		if (quot_high[i] < quot_low[i]) {
			cpr3_debug(vreg, "quot_high[%d]=%llu < quot_low[%d]=%llu; overriding: quot_high[%d]=%llu\n",
				i, quot_high[i], i, quot_low[i],
				i, quot_low[i]);
			quot_high[i] = quot_low[i];
		}
	}

	
	for (i = 1; i < vreg->fuse_corner_count; i++) {
		quot_adjust = cpr3_quot_adjustment(ro_scale[i],
						   volt_adjust_fuse[i]);
		if (quot_adjust) {
			prev_quot = quot_high[i];
			quot_high[i] += quot_adjust;
			cpr3_debug(vreg, "adjusted fuse corner %d RO%llu target quot: %llu --> %llu (%d uV)\n",
				i, fuse->ro_sel[i], prev_quot, quot_high[i],
				volt_adjust_fuse[i]);
		}

		if (fuse->ro_sel[i] == fuse->ro_sel[i - 1])
			quot_low[i] = quot_high[i - 1];
		else
			quot_low[i] += cpr3_quot_adjustment(ro_scale[i],
						    volt_adjust_fuse[i - 1]);

		if (quot_high[i] < quot_low[i]) {
			cpr3_debug(vreg, "quot_high[%d]=%llu < quot_low[%d]=%llu after adjustment; overriding: quot_high[%d]=%llu\n",
				i, quot_high[i], i, quot_low[i],
				i, quot_low[i]);
			quot_high[i] = quot_low[i];
		}
	}

	
	for (i = 1; i < vreg->fuse_corner_count; i++) {
		freq_low = vreg->corner[fmax_corner[i - 1]].proc_freq;
		freq_high = vreg->corner[fmax_corner[i]].proc_freq;

		ro = fuse->ro_sel[i];
		for (j = fmax_corner[i - 1] + 1; j <= fmax_corner[i]; j++)
			vreg->corner[j].target_quot[ro] = cpr3_interpolate(
				freq_low, quot_low[i], freq_high, quot_high[i],
				vreg->corner[j].proc_freq);
	}

	
	for (i = 0; i < vreg->corner_count; i++) {
		fuse_corner = vreg->corner[i].cpr_fuse_corner;
		ro = fuse->ro_sel[fuse_corner];
		quot_adjust = cpr3_quot_adjustment(ro_scale[fuse_corner],
						   volt_adjust[i]);
		if (quot_adjust) {
			prev_quot = vreg->corner[i].target_quot[ro];
			vreg->corner[i].target_quot[ro] += quot_adjust;
			cpr3_debug(vreg, "adjusted corner %d RO%u target quot: %llu --> %u (%d uV)\n",
				i, ro, prev_quot,
				vreg->corner[i].target_quot[ro],
				volt_adjust[i]);
		}
	}

	
	for (i = 1; i < vreg->corner_count; i++) {
		ro = fuse->ro_sel[vreg->corner[i].cpr_fuse_corner];
		if (fuse->ro_sel[vreg->corner[i - 1].cpr_fuse_corner] == ro
		    && vreg->corner[i].target_quot[ro]
				< vreg->corner[i - 1].target_quot[ro]) {
			cpr3_debug(vreg, "adjusted corner %d RO%u target quot=%u < adjusted corner %d RO%u target quot=%u; overriding: corner %d RO%u target quot=%u\n",
				i, ro, vreg->corner[i].target_quot[ro],
				i - 1, ro, vreg->corner[i - 1].target_quot[ro],
				i, ro, vreg->corner[i - 1].target_quot[ro]);
			vreg->corner[i].target_quot[ro]
				= vreg->corner[i - 1].target_quot[ro];
		}
	}

done:
	kfree(volt_adjust);
	kfree(volt_adjust_fuse);
	kfree(ro_scale);
	kfree(fmax_corner);
	kfree(quot_low);
	kfree(quot_high);
	return rc;
}

static int cpr3_msm8996_partial_binning_override(struct cpr3_regulator *vreg)
{
	struct cpr3_msm8996_hmss_fuses *fuse = vreg->platform_fuses;
	int i, fuse_corner, fmax_corner;

	if (fuse->partial_binning == MSM8996_CPR_PARTIAL_BINNING_SVS)
		fuse_corner = CPR3_MSM8996_HMSS_FUSE_CORNER_SVS;
	else if (fuse->partial_binning == MSM8996_CPR_PARTIAL_BINNING_NOM)
		fuse_corner = CPR3_MSM8996_HMSS_FUSE_CORNER_NOM;
	else
		return 0;

	cpr3_info(vreg, "overriding voltages and quotients for all corners below %s Fmax\n",
		cpr3_msm8996_hmss_fuse_corner_name[fuse_corner]);

	fmax_corner = -1;
	for (i = vreg->corner_count - 1; i >= 0; i--) {
		if (vreg->corner[i].cpr_fuse_corner == fuse_corner) {
			fmax_corner = i;
			break;
		}
	}
	if (fmax_corner < 0) {
		cpr3_err(vreg, "could not find %s Fmax corner\n",
			cpr3_msm8996_hmss_fuse_corner_name[fuse_corner]);
		return -EINVAL;
	}

	for (i = 0; i < fmax_corner; i++)
		vreg->corner[i] = vreg->corner[fmax_corner];

	return 0;
}

static void cpr3_hmss_print_settings(struct cpr3_regulator *vreg)
{
	struct cpr3_corner *corner;
	int i;

	cpr3_debug(vreg, "Corner: Frequency (Hz), Fuse Corner, Floor (uV), Open-Loop (uV), Ceiling (uV)\n");
	for (i = 0; i < vreg->corner_count; i++) {
		corner = &vreg->corner[i];
		cpr3_debug(vreg, "%3d: %10u, %2d, %7d, %7d, %7d\n",
			i, corner->proc_freq, corner->cpr_fuse_corner,
			corner->floor_volt, corner->open_loop_volt,
			corner->ceiling_volt);
	}

	if (vreg->thread->ctrl->apm)
		cpr3_debug(vreg, "APM threshold = %d uV, APM adjust = %d uV\n",
			vreg->thread->ctrl->apm_threshold_volt,
			vreg->thread->ctrl->apm_adj_volt);
}

static int cpr3_hmss_init_thread(struct cpr3_thread *thread)
{
	int rc;

	rc = cpr3_parse_common_thread_data(thread);
	if (rc) {
		cpr3_err(thread->ctrl, "thread %u unable to read CPR thread data from device tree, rc=%d\n",
			thread->thread_id, rc);
		return rc;
	}

	return 0;
}

#define MAX_VREG_NAME_SIZE 25
static int cpr3_hmss_kvreg_init(struct cpr3_regulator *vreg)
{
	struct cpr3_msm8996_hmss_fuses *fuse = vreg->platform_fuses;
	struct device_node *node = vreg->of_node;
	struct cpr3_controller *ctrl = vreg->thread->ctrl;
	int id = vreg->thread->thread_id;
	char kvreg_name_buf[MAX_VREG_NAME_SIZE];
	int rc;

	scnprintf(kvreg_name_buf, MAX_VREG_NAME_SIZE,
		"vdd-thread%d-ldo-supply", id);

	if (!of_find_property(ctrl->dev->of_node, kvreg_name_buf , NULL))
		return 0;
	else if (!of_find_property(node, "qcom,ldo-min-headroom-voltage", NULL))
		return 0;

	scnprintf(kvreg_name_buf, MAX_VREG_NAME_SIZE, "vdd-thread%d-ldo", id);

	vreg->ldo_regulator = devm_regulator_get(ctrl->dev, kvreg_name_buf);
	if (IS_ERR(vreg->ldo_regulator)) {
		rc = PTR_ERR(vreg->ldo_regulator);
		if (rc != -EPROBE_DEFER)
			cpr3_err(vreg, "unable to request %s regulator, rc=%d\n",
				 kvreg_name_buf, rc);
		return rc;
	}

	vreg->ldo_regulator_bypass = BHS_MODE;

	scnprintf(kvreg_name_buf, MAX_VREG_NAME_SIZE, "vdd-thread%d-ldo-ret",
		  id);

	vreg->ldo_ret_regulator = devm_regulator_get(ctrl->dev, kvreg_name_buf);
	if (IS_ERR(vreg->ldo_ret_regulator)) {
		rc = PTR_ERR(vreg->ldo_ret_regulator);
		if (rc != -EPROBE_DEFER)
			cpr3_err(vreg, "unable to request %s regulator, rc=%d\n",
				 kvreg_name_buf, rc);
		return rc;
	}

	if (!ctrl->system_supply_max_volt) {
		cpr3_err(ctrl, "system-supply max voltage must be specified\n");
		return -EINVAL;
	}

	rc = of_property_read_u32(node, "qcom,ldo-min-headroom-voltage",
				&vreg->ldo_min_headroom_volt);
	if (rc) {
		cpr3_err(vreg, "error reading qcom,ldo-min-headroom-voltage, rc=%d\n",
			rc);
		return rc;
	}

	rc = of_property_read_u32(node, "qcom,ldo-max-headroom-voltage",
				  &vreg->ldo_max_headroom_volt);
	if (rc) {
		cpr3_err(vreg, "error reading qcom,ldo-max-headroom-voltage, rc=%d\n",
			 rc);
		return rc;
	}

	rc = of_property_read_u32(node, "qcom,ldo-max-voltage",
				&vreg->ldo_max_volt);
	if (rc) {
		cpr3_err(vreg, "error reading qcom,ldo-max-voltage, rc=%d\n",
			rc);
		return rc;
	}

	
	vreg->ldo_ret_volt =
		max(msm8996_vdd_apcc_fuse_ret_volt[fuse->vdd_apcc_ret_fuse],
		    msm8996_vdd_mx_fuse_ret_volt[fuse->vdd_mx_ret_fuse]);

	rc = regulator_set_voltage(vreg->ldo_ret_regulator, vreg->ldo_ret_volt,
				   INT_MAX);
	if (rc < 0) {
		cpr3_err(vreg, "regulator_set_voltage(ldo_ret) == %d failed, rc=%d\n",
			 vreg->ldo_ret_volt, rc);
		return rc;
	}

	
	of_property_read_u32(node, "qcom,ldo-adjust-voltage",
			     &vreg->ldo_adjust_volt);

	vreg->ldo_mode_allowed = !of_property_read_bool(node,
							"qcom,ldo-disable");

	cpr3_info(vreg, "LDO min headroom=%d uV, LDO max headroom=%d uV, LDO adj=%d uV, LDO mode=%s, LDO retention=%d uV\n",
		  vreg->ldo_min_headroom_volt,
		  vreg->ldo_max_headroom_volt,
		  vreg->ldo_adjust_volt,
		  vreg->ldo_mode_allowed ? "allowed" : "disallowed",
		  vreg->ldo_ret_volt);

	return 0;
}

static int cpr3_hmss_mem_acc_init(struct cpr3_regulator *vreg)
{
	struct cpr3_controller *ctrl = vreg->thread->ctrl;
	int id = vreg->thread->thread_id;
	char mem_acc_vreg_name_buf[MAX_VREG_NAME_SIZE];
	int rc;

	scnprintf(mem_acc_vreg_name_buf, MAX_VREG_NAME_SIZE,
		  "mem-acc-thread%d-supply", id);

	if (!of_find_property(ctrl->dev->of_node, mem_acc_vreg_name_buf,
			      NULL)) {
		cpr3_debug(vreg, "not using memory accelerator regulator\n");
		return 0;
	} else if (!of_property_read_bool(vreg->of_node, "qcom,uses-mem-acc")) {
		return 0;
	}

	scnprintf(mem_acc_vreg_name_buf, MAX_VREG_NAME_SIZE,
		  "mem-acc-thread%d", id);

	vreg->mem_acc_regulator = devm_regulator_get(ctrl->dev,
						     mem_acc_vreg_name_buf);
	if (IS_ERR(vreg->mem_acc_regulator)) {
		rc = PTR_ERR(vreg->mem_acc_regulator);
		if (rc != -EPROBE_DEFER)
			cpr3_err(vreg, "unable to request %s regulator, rc=%d\n",
				 mem_acc_vreg_name_buf, rc);
		return rc;
	}

	return 0;
}

static int cpr3_hmss_init_regulator(struct cpr3_regulator *vreg)
{
	struct cpr3_msm8996_hmss_fuses *fuse;
	int rc;

	rc = cpr3_msm8996_hmss_read_fuse_data(vreg);
	if (rc) {
		cpr3_err(vreg, "unable to read CPR fuse data, rc=%d\n", rc);
		return rc;
	}

	rc = cpr3_hmss_kvreg_init(vreg);
	if (rc) {
		if (rc != -EPROBE_DEFER)
			cpr3_err(vreg, "unable to initialize Kryo Regulator settings, rc=%d\n",
				 rc);
		return rc;
	}

	rc = cpr3_hmss_mem_acc_init(vreg);
	if (rc) {
		if (rc != -EPROBE_DEFER)
			cpr3_err(vreg, "unable to initialize mem-acc regulator settings, rc=%d\n",
				 rc);
		return rc;
	}

	fuse = vreg->platform_fuses;
	if (fuse->limitation == MSM8996_CPR_LIMITATION_UNSUPPORTED) {
		cpr3_err(vreg, "this chip requires an unsupported voltage\n");
		return -EPERM;
	} else if (fuse->limitation
			== MSM8996_CPR_LIMITATION_NO_CPR_OR_INTERPOLATION) {
		vreg->thread->ctrl->cpr_allowed_hw = false;
	}

	rc = of_property_read_u32(vreg->of_node, "qcom,cpr-pd-bypass-mask",
				&vreg->pd_bypass_mask);
	if (rc) {
		cpr3_err(vreg, "error reading qcom,cpr-pd-bypass-mask, rc=%d\n",
			rc);
		return rc;
	}

	rc = cpr3_hmss_parse_corner_data(vreg);
	if (rc) {
		cpr3_err(vreg, "unable to read CPR corner data from device tree, rc=%d\n",
			rc);
		return rc;
	}

	if (of_find_property(vreg->of_node, "qcom,cpr-dynamic-floor-corner",
				NULL)) {
		rc = cpr3_parse_array_property(vreg,
			"qcom,cpr-dynamic-floor-corner",
			1, &vreg->dynamic_floor_corner);
		if (rc) {
			cpr3_err(vreg, "error reading qcom,cpr-dynamic-floor-corner, rc=%d\n",
				rc);
			return rc;
		}

		if (vreg->dynamic_floor_corner <= 0) {
			vreg->uses_dynamic_floor = false;
		} else if (vreg->dynamic_floor_corner < CPR3_CORNER_OFFSET
			   || vreg->dynamic_floor_corner
				> vreg->corner_count - 1 + CPR3_CORNER_OFFSET) {
			cpr3_err(vreg, "dynamic floor corner=%d not in range [%d, %d]\n",
				vreg->dynamic_floor_corner, CPR3_CORNER_OFFSET,
				vreg->corner_count - 1 + CPR3_CORNER_OFFSET);
			return -EINVAL;
		}

		vreg->dynamic_floor_corner -= CPR3_CORNER_OFFSET;
		vreg->uses_dynamic_floor = true;
	}

	rc = cpr3_msm8996_hmss_calculate_open_loop_voltages(vreg);
	if (rc) {
		cpr3_err(vreg, "unable to calculate open-loop voltages, rc=%d\n",
			rc);
		return rc;
	}

	rc = cpr3_limit_open_loop_voltages(vreg);
	if (rc) {
		cpr3_err(vreg, "unable to limit open-loop voltages, rc=%d\n",
			rc);
		return rc;
	}

	cpr3_open_loop_voltage_as_ceiling(vreg);

	rc = cpr3_limit_floor_voltages(vreg);
	if (rc) {
		cpr3_err(vreg, "unable to limit floor voltages, rc=%d\n", rc);
		return rc;
	}

	rc = cpr3_msm8996_hmss_calculate_target_quotients(vreg);
	if (rc) {
		cpr3_err(vreg, "unable to calculate target quotients, rc=%d\n",
			rc);
		return rc;
	}

	rc = cpr3_msm8996_partial_binning_override(vreg);
	if (rc) {
		cpr3_err(vreg, "unable to override voltages and quotients based on partial binning fuse, rc=%d\n",
			rc);
		return rc;
	}

	cpr3_hmss_print_settings(vreg);

	return 0;
}

static int cpr3_hmss_init_aging(struct cpr3_controller *ctrl)
{
	struct cpr3_msm8996_hmss_fuses *fuse = NULL;
	struct cpr3_regulator *vreg;
	u32 aging_ro_scale;
	int i, j, rc;

	for (i = 0; i < ctrl->thread_count; i++) {
		for (j = 0; j < ctrl->thread[i].vreg_count; j++) {
			if (ctrl->thread[i].vreg[j].aging_allowed) {
				ctrl->aging_required = true;
				vreg = &ctrl->thread[i].vreg[j];
				fuse = vreg->platform_fuses;
				break;
			}
		}
	}

	if (!ctrl->aging_required || !fuse)
		return 0;

	rc = cpr3_parse_array_property(vreg, "qcom,cpr-aging-ro-scaling-factor",
					1, &aging_ro_scale);
	if (rc)
		return rc;

	if (aging_ro_scale == 0) {
		cpr3_err(ctrl, "aging RO scaling factor is invalid: %u\n",
			aging_ro_scale);
		return -EINVAL;
	}

	ctrl->aging_vdd_mode = REGULATOR_MODE_NORMAL;
	ctrl->aging_complete_vdd_mode = REGULATOR_MODE_IDLE;

	ctrl->aging_sensor_count = 1;
	ctrl->aging_sensor = kzalloc(sizeof(*ctrl->aging_sensor), GFP_KERNEL);
	if (!ctrl->aging_sensor)
		return -ENOMEM;

	ctrl->aging_sensor->sensor_id = MSM8996_HMSS_AGING_SENSOR_ID;
	ctrl->aging_sensor->bypass_mask[0] = MSM8996_HMSS_AGING_BYPASS_MASK0;
	ctrl->aging_sensor->ro_scale = aging_ro_scale;

	ctrl->aging_sensor->init_quot_diff
		= cpr3_convert_open_loop_voltage_fuse(0,
			MSM8996_HMSS_AGING_INIT_QUOT_DIFF_SCALE,
			fuse->aging_init_quot_diff,
			MSM8996_HMSS_AGING_INIT_QUOT_DIFF_SIZE);

	cpr3_debug(ctrl, "sensor %u aging init quotient diff = %d, aging RO scale = %u QUOT/V\n",
		ctrl->aging_sensor->sensor_id,
		ctrl->aging_sensor->init_quot_diff,
		ctrl->aging_sensor->ro_scale);

	return 0;
}

static int cpr3_hmss_init_controller(struct cpr3_controller *ctrl)
{
	int i, rc;

	rc = cpr3_parse_common_ctrl_data(ctrl);
	if (rc) {
		if (rc != -EPROBE_DEFER)
			cpr3_err(ctrl, "unable to parse common controller data, rc=%d\n",
				rc);
		return rc;
	}

	ctrl->vdd_limit_regulator = devm_regulator_get(ctrl->dev, "vdd-limit");
	if (IS_ERR(ctrl->vdd_limit_regulator)) {
		rc = PTR_ERR(ctrl->vdd_limit_regulator);
		if (rc != -EPROBE_DEFER)
			cpr3_err(ctrl, "unable to request vdd-supply regulator, rc=%d\n",
				rc);
		return rc;
	}

	rc = of_property_read_u32(ctrl->dev->of_node,
			"qcom,cpr-up-down-delay-time",
			&ctrl->up_down_delay_time);
	if (rc) {
		cpr3_err(ctrl, "error reading property qcom,cpr-up-down-delay-time, rc=%d\n",
			rc);
		return rc;
	}

	
	of_property_read_u32(ctrl->dev->of_node,
			     "qcom,system-supply-max-voltage",
			     &ctrl->system_supply_max_volt);

	
	of_property_read_u32(ctrl->dev->of_node, "qcom,cpr-clock-throttling",
			&ctrl->proc_clock_throttle);

	rc = cpr3_apm_init(ctrl);
	if (rc) {
		if (rc != -EPROBE_DEFER)
			cpr3_err(ctrl, "unable to initialize APM settings, rc=%d\n",
				rc);
		return rc;
	}

	ctrl->sensor_count = MSM8996_HMSS_CPR_SENSOR_COUNT;

	ctrl->sensor_owner = devm_kcalloc(ctrl->dev, ctrl->sensor_count,
		sizeof(*ctrl->sensor_owner), GFP_KERNEL);
	if (!ctrl->sensor_owner)
		return -ENOMEM;

	
	for (i = MSM8996_HMSS_THREAD0_SENSOR_MIN;
	     i <= MSM8996_HMSS_THREAD0_SENSOR_MAX; i++)
		ctrl->sensor_owner[i] = 0;
	for (i = MSM8996_HMSS_THREAD1_SENSOR_MIN;
	     i <= MSM8996_HMSS_THREAD1_SENSOR_MAX; i++)
		ctrl->sensor_owner[i] = 1;

	ctrl->cpr_clock_rate = MSM8996_HMSS_CPR_CLOCK_RATE;
	ctrl->ctrl_type = CPR_CTRL_TYPE_CPR3;
	ctrl->supports_hw_closed_loop = true;
	ctrl->use_hw_closed_loop = of_property_read_bool(ctrl->dev->of_node,
						"qcom,cpr-hw-closed-loop");

	if (ctrl->mem_acc_regulator) {
		rc = of_property_read_u32(ctrl->dev->of_node,
					  "qcom,mem-acc-supply-threshold-voltage",
					  &ctrl->mem_acc_threshold_volt);
		if (rc) {
			cpr3_err(ctrl, "error reading property qcom,mem-acc-supply-threshold-voltage, rc=%d\n",
				 rc);
			return rc;
		}

		ctrl->mem_acc_threshold_volt =
			CPR3_ROUND(ctrl->mem_acc_threshold_volt,
				   ctrl->step_volt);

		rc = of_property_read_u32_array(ctrl->dev->of_node,
			"qcom,mem-acc-supply-corner-map",
			&ctrl->mem_acc_corner_map[CPR3_MEM_ACC_LOW_CORNER],
			CPR3_MEM_ACC_CORNERS);
		if (rc) {
			cpr3_err(ctrl, "error reading qcom,mem-acc-supply-corner-map, rc=%d\n",
				 rc);
			return rc;
		}
	}

	return 0;
}

static int cpr3_hmss_regulator_suspend(struct platform_device *pdev,
				pm_message_t state)
{
	struct cpr3_controller *ctrl = platform_get_drvdata(pdev);

	return cpr3_regulator_suspend(ctrl);
}

static int cpr3_hmss_regulator_resume(struct platform_device *pdev)
{
	struct cpr3_controller *ctrl = platform_get_drvdata(pdev);

	return cpr3_regulator_resume(ctrl);
}

static struct of_device_id cpr_regulator_match_table[] = {
	{
		.compatible = "qcom,cpr3-msm8996-v1-hmss-regulator",
		.data = (void *)1
	},
	{
		.compatible = "qcom,cpr3-msm8996-v2-hmss-regulator",
		.data = (void *)2
	},
	{
		.compatible = "qcom,cpr3-msm8996-v3-hmss-regulator",
		.data = (void *)3
	},
	{
		.compatible = "qcom,cpr3-msm8996-hmss-regulator",
		.data = (void *)3
	},
	{}
};

u64* htc_target_quot[2] = {NULL};
int htc_target_quot_len = 0;
static void bak_htc_target_quot(struct cpr3_controller *ctrl)
{
	struct cpr3_msm8996_hmss_fuses *fuse;
	struct cpr3_thread *thread;
	struct cpr3_regulator *vreg;
	int i, size;

	
	size = sizeof(htc_target_quot)/sizeof(u64);
	if (size != ctrl->thread_count) {
		WARN_ON(1);
		return;
	}

	htc_target_quot_len = MSM8996_HMSS_FUSE_CORNERS;
	for (i = 0; i < ctrl->thread_count; i++) {
		thread = &ctrl->thread[i];
		vreg = thread->vreg;
		fuse = vreg->platform_fuses;
		htc_target_quot[i] = fuse->target_quot;
	}
}

static int cpr3_hmss_regulator_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	const struct of_device_id *match;
	struct cpr3_controller *ctrl;
	struct cpr3_regulator *vreg;
	int i, j, rc;

	if (!dev->of_node) {
		dev_err(dev, "Device tree node is missing\n");
		return -EINVAL;
	}

	ctrl = devm_kzalloc(dev, sizeof(*ctrl), GFP_KERNEL);
	if (!ctrl)
		return -ENOMEM;

	ctrl->dev = dev;
	
	ctrl->cpr_allowed_hw = true;

	rc = of_property_read_string(dev->of_node, "qcom,cpr-ctrl-name",
					&ctrl->name);
	if (rc) {
		cpr3_err(ctrl, "unable to read qcom,cpr-ctrl-name, rc=%d\n",
			rc);
		return rc;
	}

	match = of_match_node(cpr_regulator_match_table, dev->of_node);
	if (match)
		ctrl->soc_revision = (uintptr_t)match->data;
	else
		cpr3_err(ctrl, "could not find compatible string match\n");

	rc = cpr3_map_fuse_base(ctrl, pdev);
	if (rc) {
		cpr3_err(ctrl, "could not map fuse base address\n");
		return rc;
	}

	rc = cpr3_allocate_threads(ctrl, MSM8996_HMSS_POWER_CLUSTER_THREAD_ID,
		MSM8996_HMSS_PERFORMANCE_CLUSTER_THREAD_ID);
	if (rc) {
		cpr3_err(ctrl, "failed to allocate CPR thread array, rc=%d\n",
			rc);
		return rc;
	}

	if (ctrl->thread_count < 1) {
		cpr3_err(ctrl, "thread nodes are missing\n");
		return -EINVAL;
	}

	rc = cpr3_hmss_init_controller(ctrl);
	if (rc) {
		if (rc != -EPROBE_DEFER)
			cpr3_err(ctrl, "failed to initialize CPR controller parameters, rc=%d\n",
				rc);
		return rc;
	}

	for (i = 0; i < ctrl->thread_count; i++) {
		rc = cpr3_hmss_init_thread(&ctrl->thread[i]);
		if (rc) {
			cpr3_err(ctrl, "thread %u initialization failed, rc=%d\n",
				ctrl->thread[i].thread_id, rc);
			return rc;
		}

		for (j = 0; j < ctrl->thread[i].vreg_count; j++) {
			vreg = &ctrl->thread[i].vreg[j];

			rc = cpr3_hmss_init_regulator(vreg);
			if (rc) {
				cpr3_err(vreg, "regulator initialization failed, rc=%d\n",
					rc);
				return rc;
			}
		}
	}

	rc = cpr3_hmss_init_aging(ctrl);
	if (rc) {
		cpr3_err(ctrl, "failed to initialize aging configurations, rc=%d\n",
			rc);
		return rc;
	}

	platform_set_drvdata(pdev, ctrl);
	bak_htc_target_quot(ctrl);

	return cpr3_regulator_register(pdev, ctrl);
}

static int cpr3_hmss_regulator_remove(struct platform_device *pdev)
{
	struct cpr3_controller *ctrl = platform_get_drvdata(pdev);

	return cpr3_regulator_unregister(ctrl);
}

static struct platform_driver cpr3_hmss_regulator_driver = {
	.driver		= {
		.name		= "qcom,cpr3-hmss-regulator",
		.of_match_table	= cpr_regulator_match_table,
		.owner		= THIS_MODULE,
	},
	.probe		= cpr3_hmss_regulator_probe,
	.remove		= cpr3_hmss_regulator_remove,
	.suspend	= cpr3_hmss_regulator_suspend,
	.resume		= cpr3_hmss_regulator_resume,
};

static int cpr_regulator_init(void)
{
	return platform_driver_register(&cpr3_hmss_regulator_driver);
}

static void cpr_regulator_exit(void)
{
	platform_driver_unregister(&cpr3_hmss_regulator_driver);
}

MODULE_DESCRIPTION("CPR3 HMSS regulator driver");
MODULE_LICENSE("GPL v2");

arch_initcall(cpr_regulator_init);
module_exit(cpr_regulator_exit);
