/*
 * Copyright (c) 2026 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <soc.h>
#include <smc_cpu_reg.h>
#include <zephyr/sys/sys_io.h>

typedef struct {
	uint32_t core_reset: 1;
	uint32_t rsvd_0: 7;
	uint32_t uncore_reset: 1;
	uint32_t rsvd_1: 7;
	uint32_t debug_reset: 1;
} D2D_SS_ASYNC_CPU_CTRL_reg_t;

typedef union {
	uint32_t val;
	D2D_SS_ASYNC_CPU_CTRL_reg_t f;
} D2D_SS_ASYNC_CPU_CTRL_reg_u;

typedef struct {
    uint32_t noc2axi_reset_n : 1;
    uint32_t d2d_noc_reset_n : 1;
    uint32_t rsvd_0 : 1;
    uint32_t d2d_sys_rst_ni : 1;
    uint32_t d2d_i_axi4l_aresetn : 1;
    uint32_t d2d_i_apb_resetn : 1;
    uint32_t d2d_ll_aresetn : 1;
    uint32_t d2d_qnp_aresetn : 1;
} TT_MIMIR_D2D_STRAP_RESET_reg_t;

typedef union {
    uint32_t val;
    TT_MIMIR_D2D_STRAP_RESET_reg_t f;
} TT_MIMIR_D2D_STRAP_RESET_reg_u;

static uint8_t d2d_fw_bin[] = {
#include "d2d_fw_bin.inc"
};

#define SMC_CPU_SMC_INBOUND_FILTER_CTRL_0__REG_MAP_BASE_ADDR  (0x00015000)
#define SMC_CPU_SMC_OUTBOUND_FILTER_CTRL_0__REG_MAP_BASE_ADDR (0x00016000)

#define ITN_SMN2ITN_RD_INIU_FIREWALL_MEM_TILE0_MAIN_FIREWALL_0__REG_MAP_BASE_ADDR  (0x04000000)
#define ITN_SMN2ITN_RD_INIU_FIREWALL_MEM_TILE1_MAIN_FIREWALL_0__REG_MAP_BASE_ADDR  (0x04000800)
#define ITN_SMN2ITN_RD_INIU_FIREWALL_CCE_TILE0_MAIN_FIREWALL_0__REG_MAP_BASE_ADDR  (0x04001000)
#define ITN_SMN2ITN_RD_INIU_FIREWALL_CCE_TILE1_MAIN_FIREWALL_0__REG_MAP_BASE_ADDR  (0x04001800)
#define ITN_SMN2ITN_RD_INIU_FIREWALL_CCE_CFG_TILE0_MAIN_FIREWALL_0__REG_MAP_BASE_ADDR  (0x04002000)
#define ITN_SMN2ITN_RD_INIU_FIREWALL_CCE_CFG_TILE1_MAIN_FIREWALL_0__REG_MAP_BASE_ADDR  (0x04002800)

#define D2D_0_D2D_D2D_SS_ASYNC_CPU_CTRL_REG_ADDR                                          (0x04201800)


#define D2D_0_STRAP_RESET_REG_ADDR                                                        (0x04300000)
#define D2D_1_STRAP_RESET_REG_ADDR                                                        (0x04500000)

static void config_filter(uint64_t base_addr)
{
	FILTER_CTRL_FILTER_CONFIG_reg_u config;
	config.val = sys_read64(SMC_CPU_SMC_OUTBOUND_FILTER_CTRL_0__REG_MAP_BASE_ADDR);
	config.f.read_en = 1;
	config.f.write_en = 1;
	config.f.addr_mode = 1;
	config.f.allow_ns = 0;
	config.f.data_bus_width = 7; // 128 bits
	config.f.allow_burst = 1;
	/* Set filter configuration */
	sys_write64(config.val, SMC_CPU_SMC_OUTBOUND_FILTER_CTRL_0__REG_MAP_BASE_ADDR);
	sys_write64(0x0000000000000000, base_addr + 0x8); /* Start addr */
	sys_write64(0xFFFFFFFFFFFFFFFF, base_addr + 0x10); /* End addr */

	config.f.allow_ns = 1;
	sys_write64(config.val, SMC_CPU_SMC_OUTBOUND_FILTER_CTRL_0__REG_MAP_BASE_ADDR + 0x20); /* Filter 1 config */
	sys_write64(0x0000000000000000, base_addr + 0x28); /* Filter 1 start addr */
	sys_write64(0xFFFFFFFFFFFFFFFF, base_addr + 0x30); /* Filter 1 end addr */
}

ZTEST(remoteproc_d2d, test_load_fw)
{
	D2D_SS_ASYNC_CPU_CTRL_reg_u d2d_reset_cpu;

	printk("Starting D2D firmware load test!\n");

	/* Remove all resets */
	sys_write32(0xFFFFFFFF, RESET_UNIT_SS_COLD_RESET_N_REG_ADDR);
	sys_write32(0xFFFFFFFF, RESET_UNIT_SS_WARM_RESET_N_REG_ADDR);

	/* First, turn off all firewalls */
	config_filter(SMC_CPU_SMC_OUTBOUND_FILTER_CTRL_0__REG_MAP_BASE_ADDR);
	config_filter(SMC_CPU_SMC_INBOUND_FILTER_CTRL_0__REG_MAP_BASE_ADDR);
	config_filter(ITN_SMN2ITN_RD_INIU_FIREWALL_MEM_TILE0_MAIN_FIREWALL_0__REG_MAP_BASE_ADDR);
	config_filter(ITN_SMN2ITN_RD_INIU_FIREWALL_MEM_TILE1_MAIN_FIREWALL_0__REG_MAP_BASE_ADDR);
	config_filter(ITN_SMN2ITN_RD_INIU_FIREWALL_CCE_TILE0_MAIN_FIREWALL_0__REG_MAP_BASE_ADDR);
	config_filter(ITN_SMN2ITN_RD_INIU_FIREWALL_CCE_TILE1_MAIN_FIREWALL_0__REG_MAP_BASE_ADDR);
	config_filter(ITN_SMN2ITN_RD_INIU_FIREWALL_CCE_CFG_TILE0_MAIN_FIREWALL_0__REG_MAP_BASE_ADDR);
	config_filter(ITN_SMN2ITN_RD_INIU_FIREWALL_CCE_CFG_TILE1_MAIN_FIREWALL_0__REG_MAP_BASE_ADDR);

	/* Remove D2D resets */
	TT_MIMIR_D2D_STRAP_RESET_reg_u d2d_reset;
	d2d_reset.val = 0x0;
	d2d_reset.f.d2d_noc_reset_n = 1;
	sys_write64(d2d_reset.val, D2D_0_STRAP_RESET_REG_ADDR);
	sys_write64(d2d_reset.val, D2D_1_STRAP_RESET_REG_ADDR);
	d2d_reset.f.d2d_i_apb_resetn = 1;
	sys_write64(d2d_reset.val, D2D_0_STRAP_RESET_REG_ADDR);
	sys_write64(d2d_reset.val, D2D_1_STRAP_RESET_REG_ADDR);
	d2d_reset.f.d2d_sys_rst_ni = 1;
	sys_write64(d2d_reset.val, D2D_0_STRAP_RESET_REG_ADDR);
	sys_write64(d2d_reset.val, D2D_1_STRAP_RESET_REG_ADDR);
	d2d_reset.f.d2d_ll_aresetn = 1;
	sys_write64(d2d_reset.val, D2D_0_STRAP_RESET_REG_ADDR);
	sys_write64(d2d_reset.val, D2D_1_STRAP_RESET_REG_ADDR);
	d2d_reset.f.d2d_i_axi4l_aresetn = 1;
	sys_write64(d2d_reset.val, D2D_0_STRAP_RESET_REG_ADDR);
	sys_write64(d2d_reset.val, D2D_1_STRAP_RESET_REG_ADDR);
	d2d_reset.f.d2d_qnp_aresetn = 1;
	sys_write64(d2d_reset.val, D2D_0_STRAP_RESET_REG_ADDR);
	sys_write64(d2d_reset.val, D2D_1_STRAP_RESET_REG_ADDR);
	d2d_reset.f.noc2axi_reset_n = 1;
	sys_write64(d2d_reset.val, D2D_0_STRAP_RESET_REG_ADDR);
	sys_write64(d2d_reset.val, D2D_1_STRAP_RESET_REG_ADDR);

	printk("All firewalls disabled. Proceeding to load D2D firmware...\n");

	/* Release uncore reset so we can access SRAM */
	printk("Attempting to access D2D registers...\n");
	d2d_reset_cpu.val = sys_read32(D2D_0_D2D_D2D_SS_ASYNC_CPU_CTRL_REG_ADDR);
	d2d_reset_cpu.f.uncore_reset = 0;
	d2d_reset_cpu.f.core_reset = 1; /* Keep core in reset while loading firmware */
	printk("Releasing D2D uncore reset to access SRAM...\n");
	sys_write32(d2d_reset_cpu.val, D2D_0_D2D_D2D_SS_ASYNC_CPU_CTRL_REG_ADDR);

	/* Program D2D firmware to remote processor */
	uint32_t load_addr = 0x04202000;

	printk("Loading D2D firmware to address 0x%08X...\n", load_addr);

	/* Zero D2D memory */
	for (size_t i = 0; i < 0x10000; i += 8) {
		sys_write64(0, load_addr + i);
	}

	printk("D2D memory zeroed. Proceeding to load firmware binary...\n");

	/* Write to scratch 0 so TB knows to start waves */
	WRITE_SCRATCH(0, 0xdeadbeef);
	printk("Started wave dump");

	/* Load D2D firmware */
	memcpy((void *)load_addr, d2d_fw_bin, sizeof(d2d_fw_bin));

	printk("D2D firmware loaded. Releasing D2D core reset to start execution...\n");

	/* Release D2D reset */
	d2d_reset_cpu.val = sys_read32(0x04201800);
	d2d_reset_cpu.f.uncore_reset = 0;
	d2d_reset_cpu.f.core_reset = 0;
	sys_write32(d2d_reset_cpu.val, 0x04201800);

	/* Wait for firmware execution */
	k_msleep(100);
	/* Turn off waves */
	WRITE_SCRATCH(0, 0xcafebabe);
	printk("Stopped wave dump");
	/* Read from scratch 1 */
	uint32_t scratch_val = sys_read32(0xC0010108);
	zassert_equal(scratch_val, 0xdeadbeef, "Unexpected value from D2D firmware: 0x%08X",
		      scratch_val);
}

ZTEST_SUITE(remoteproc_d2d, NULL, NULL, NULL, NULL, NULL);
