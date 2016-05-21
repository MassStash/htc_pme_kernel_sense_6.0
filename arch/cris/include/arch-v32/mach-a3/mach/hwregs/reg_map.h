#ifndef __reg_map_h
#define __reg_map_h

/*
 * This file is autogenerated from
 *   file:            reg.rmap
 * 
 *   by ../../../tools/rdesc/bin/rdes2c -base 0xb0000000 -map marb_bar.r marb_foo.r ccd_top.r ccd_stat.r ccd_tg.r ccd_dp.r ccd.r iop_sap_in.r iop_sap_out.r iop_sw_cfg.r iop_sw_cpu.r iop_sw_mpu.r iop_sw_spu.r iop_version.r iop_crc_par.r iop_dmc_in.r iop_dmc_out.r iop_fifo_in_extra.r iop_fifo_in.r iop_fifo_out_extra.r iop_fifo_out.r iop_mc.r iop_mpu.r iop_scrc_in.r iop_scrc_out.r iop_spu.r iop_timer_grp.r iop_trigger_grp.r iop.r -outfile reg_map.h reg.rmap
 * Any changes here will be lost.
 *
 * -*- buffer-read-only: t -*-
 */
typedef enum {
  regi_ccd                                 = 0xb0000000,
  regi_ccd_top                             = 0xb0000000,
  regi_ccd_dp                              = 0xb0000400,
  regi_ccd_stat                            = 0xb0000800,
  regi_ccd_tg                              = 0xb0001000,
  regi_cfg                                 = 0xb0002000,
  regi_clkgen                              = 0xb0004000,
  regi_ddr2_ctrl                           = 0xb0006000,
  regi_dma0                                = 0xb0008000,
  regi_dma1                                = 0xb000a000,
  regi_dma11                               = 0xb000c000,
  regi_dma2                                = 0xb000e000,
  regi_dma3                                = 0xb0010000,
  regi_dma4                                = 0xb0012000,
  regi_dma5                                = 0xb0014000,
  regi_dma6                                = 0xb0016000,
  regi_dma7                                = 0xb0018000,
  regi_dma9                                = 0xb001a000,
  regi_eth                                 = 0xb001c000,
  regi_gio                                 = 0xb0020000,
  regi_h264                                = 0xb0022000,
  regi_hist                                = 0xb0026000,
  regi_iop                                 = 0xb0028000,
  regi_iop_version                         = 0xb0028000,
  regi_iop_fifo_in_extra                   = 0xb0028040,
  regi_iop_fifo_out_extra                  = 0xb0028080,
  regi_iop_trigger_grp0                    = 0xb00280c0,
  regi_iop_trigger_grp1                    = 0xb0028100,
  regi_iop_trigger_grp2                    = 0xb0028140,
  regi_iop_trigger_grp3                    = 0xb0028180,
  regi_iop_trigger_grp4                    = 0xb00281c0,
  regi_iop_trigger_grp5                    = 0xb0028200,
  regi_iop_trigger_grp6                    = 0xb0028240,
  regi_iop_trigger_grp7                    = 0xb0028280,
  regi_iop_crc_par                         = 0xb0028300,
  regi_iop_dmc_in                          = 0xb0028380,
  regi_iop_dmc_out                         = 0xb0028400,
  regi_iop_fifo_in                         = 0xb0028480,
  regi_iop_fifo_out                        = 0xb0028500,
  regi_iop_scrc_in                         = 0xb0028580,
  regi_iop_scrc_out                        = 0xb0028600,
  regi_iop_timer_grp0                      = 0xb0028680,
  regi_iop_timer_grp1                      = 0xb0028700,
  regi_iop_sap_in                          = 0xb0028800,
  regi_iop_sap_out                         = 0xb0028900,
  regi_iop_spu                             = 0xb0028a00,
  regi_iop_sw_cfg                          = 0xb0028b00,
  regi_iop_sw_cpu                          = 0xb0028c00,
  regi_iop_sw_mpu                          = 0xb0028d00,
  regi_iop_sw_spu                          = 0xb0028e00,
  regi_iop_mpu                             = 0xb0029000,
  regi_irq                                 = 0xb002a000,
  regi_irq2                                = 0xb006a000,
  regi_jpeg                                = 0xb002c000,
  regi_l2cache                             = 0xb0030000,
  regi_marb_bar                            = 0xb0032000,
  regi_marb_bar_bp0                        = 0xb0032140,
  regi_marb_bar_bp1                        = 0xb0032180,
  regi_marb_bar_bp2                        = 0xb00321c0,
  regi_marb_bar_bp3                        = 0xb0032200,
  regi_marb_foo                            = 0xb0034000,
  regi_marb_foo_bp0                        = 0xb0034280,
  regi_marb_foo_bp1                        = 0xb00342c0,
  regi_marb_foo_bp2                        = 0xb0034300,
  regi_marb_foo_bp3                        = 0xb0034340,
  regi_pinmux                              = 0xb0038000,
  regi_pio                                 = 0xb0036000,
  regi_sclr                                = 0xb003a000,
  regi_sclr_fifo                           = 0xb003c000,
  regi_ser0                                = 0xb003e000,
  regi_ser1                                = 0xb0040000,
  regi_ser2                                = 0xb0042000,
  regi_ser3                                = 0xb0044000,
  regi_ser4                                = 0xb0046000,
  regi_sser                                = 0xb0048000,
  regi_strcop                              = 0xb004a000,
  regi_strdma0                             = 0xb004e000,
  regi_strdma1                             = 0xb0050000,
  regi_strdma2                             = 0xb0052000,
  regi_strdma3                             = 0xb0054000,
  regi_strdma5                             = 0xb0056000,
  regi_strmux                              = 0xb004c000,
  regi_timer0                              = 0xb0058000,
  regi_timer1                              = 0xb005a000,
  regi_timer2                              = 0xb006e000,
  regi_trace                               = 0xb005c000,
  regi_vin                                 = 0xb005e000,
  regi_vout                                = 0xb0060000
} reg_scope_instances;
#endif /* __reg_map_h */
