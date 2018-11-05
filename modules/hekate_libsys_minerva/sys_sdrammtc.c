/*
 * Minerva Training Cell
 * DRAM Training for Tegra X1 SoC. Supports DDR2/3 and LPDDR3/4.
 *
 * Copyright (c) 2018 CTCaer  <ctcaer@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <stdlib.h>
#include "mtc.h"
#include "mtc_switch_tables.h"
#include "types.h"
#include "../../common/common_module.h"

#define EPRINTF(...)
#define EPRINTFARGS(...)

bool emc_2X_clk_src_is_pllmb;
bool fsp_for_src_freq;
bool train_ram_patterns;

void _usleep(u32 microseconds)
{
	u32 start = TMR(0x10);
	while ((u32)(TMR(0x10) - start) <= microseconds)
		;
}

pllm_clk_config_t pllm_clk_config_table[15] =
{
	// rate_min, rate_dst, pll_feedback_div, pll_input_div, pll_post_div.
	{12000, 800000,  0x42, 1, 0},
	{13000, 800000,  0x3D, 1, 0},
	{38400, 297600,  0x5D, 4, 2},
	{38400, 400000,  0x7D, 4, 2},
	{38400, 408000,  0x55, 4, 1},
	{38400, 532800,  0x6F, 4, 1},
	{38400, 665600,  0x68, 3, 1},
	{38400, 800000,  0x7D, 3, 1},
	{38400, 931200,  0x61, 4, 0},
	{38400, 1065600, 0x6F, 4, 0},
	{38400, 1200000, 0x7D, 4, 0},
	{38400, 1331200, 0x68, 3, 0},
	{38400, 1459200, 0x4C, 2, 0},
	{38400, 1600000, 0x7D, 3, 0},
 	{0,     0,       0,    0, 0}
};

u32 burst_regs_emc_addr_table[221] = {
	0x02C, 0x030, 0x590, 0x580, 0x0C0, 0x034, 0x038, 0x03C,
	0x040, 0x044, 0x048, 0x144, 0x0AC, 0x5C0, 0x04C, 0x050,
	0x054, 0x058, 0x0B8, 0x4E0, 0x05C, 0x498, 0x494, 0x2D0,
	0x490, 0x48C, 0x060, 0x568, 0x468, 0x46C, 0x14C, 0x4A4,
	0x150, 0x154, 0x56C, 0x064, 0x068, 0x06C, 0x2CC, 0x2D8,
	0x2D4, 0x070, 0x074, 0x3DC, 0x078, 0x07C, 0x080, 0x084,
	0x088, 0x08C, 0x11C, 0x118, 0x0B4, 0x090, 0x3E4, 0x094,
	0x158, 0x15C, 0x098, 0x09C, 0x0A0, 0x0A4, 0x4A8, 0x0A8,
	0x0B0, 0x104, 0x584, 0x2BC, 0x2C0, 0xCF4, 0x55C, 0x554,
	0x610, 0x614, 0x630, 0x634, 0x4AC, 0x670, 0x674, 0x680,
	0x684, 0x688, 0x68C, 0x690, 0x694, 0x6A0, 0x6A4, 0x6A8,
	0x6AC, 0x6B0, 0x6B4, 0xC00, 0xC04, 0xC08, 0xC0C, 0xC10,
	0xC20, 0xC24, 0xC28, 0x80C, 0x81C, 0x82C, 0x83C, 0x84C,
	0x85C, 0x86C, 0x87C, 0x88C, 0x89C, 0x8AC, 0x8BC, 0x90C,
	0x91C, 0x92C, 0x93C, 0x94C, 0x95C, 0x96C, 0x97C, 0x980,
	0x984, 0x988, 0x98C, 0x990, 0x994, 0x998, 0x99C, 0x9A0,
	0x9A4, 0x9A8, 0x9AC, 0x9B0, 0x9B4, 0x9B8, 0x9BC, 0x480,
	0x310, 0x314, 0x100, 0x2E0, 0x2E4, 0x0C8, 0x0C4, 0x464,
	0x5E4, 0x5E8, 0xC78, 0xC44, 0x00C, 0x560, 0x3E0, 0x564,
	0x594, 0x598, 0x5A4, 0x5A8, 0xC40, 0xC54, 0xC50, 0xC5C,
	0xC58, 0xC60, 0xC64, 0xC68, 0xC34, 0xC38, 0xCF0, 0x330,
	0x318, 0x334, 0x31C, 0xC3C, 0x49C, 0x720, 0x724, 0x728,
	0x72C, 0x730, 0x734, 0x5F0, 0x740, 0x744, 0x748, 0x74C,
	0x750, 0x754, 0x760, 0x770, 0x774, 0x778, 0x780, 0x784,
	0x788, 0x3B4, 0x460, 0x3BC, 0x3C4, 0x3F4, 0x3F8, 0x4C4,
	0x3FC, 0x400, 0xE04, 0xE44, 0xE6C, 0xE30, 0xE34, 0xE38,
	0xE3C, 0xE0C, 0xE10, 0xE14, 0xED0, 0xE24, 0xE28, 0xE2C,
	0xE18, 0xE1C, 0xE20, 0xE5C, 0x4D0
};

u32 burst_reg_per_ch_emc01_addr_table[8] = {
	0x34B4, 0x44B4, 0x34B8, 0x44B8,
	0x34BC, 0x44BC, 0x34C0, 0x44C0
};

u32 vref_perch_regs_emc01_addr_table[4] = {
	0x3ED4, 0x4ED4, 0x3ED8, 0x4ED8
};

u32 training_mod_regs_emc01_addr_table[20] = {
	0x3E98, 0x4E98, 0x3E9C, 0x4E9C, 0x3EA0,
	0x4EA0, 0x3EA4, 0x4EA4, 0x3EA8, 0x4EA8,
	0x3EAC, 0x4EAC, 0x3EB0, 0x4EB0, 0x3EB4,
	0x4EB4, 0x3EB8, 0x4EB8, 0x3EBC, 0x4EBC
};

u32 trim_regs_emc_addr_table[138] = {
	0x6C0, 0x6C4, 0x6C8, 0x6CC, 0x6E0,
	0x6E4, 0x6E8, 0x6EC, 0xA00, 0xA04,
	0xA08, 0xA10, 0xA14, 0xA18, 0xA20,
	0xA24, 0xA28, 0xA30, 0xA34, 0xA38,
	0xA40, 0xA44, 0xA48, 0xA50, 0xA54,
	0xA58, 0xA60, 0xA64, 0xA68, 0xA70,
	0xA74, 0xA78, 0xB00, 0xB04, 0xB08,
	0xB10, 0xB14, 0xB18, 0xB20, 0xB24,
	0xB28, 0xB30, 0xB34, 0xB38, 0xB40,
	0xB44, 0xB48, 0xB50, 0xB54, 0xB58,
	0xB60, 0xB64, 0xB68, 0xB70, 0xB74,
	0xB78, 0xBF0, 0xBF4, 0xBE0, 0xBE4,
	0x640, 0x644, 0x648, 0x64C, 0x650,
	0x654, 0x660, 0x664, 0x668, 0x66C,
	0x800, 0x804, 0x808, 0x810, 0x814,
	0x818, 0x820, 0x824, 0x828, 0x830,
	0x834, 0x838, 0x840, 0x844, 0x848,
	0x850, 0x854, 0x858, 0x860, 0x864,
	0x868, 0x870, 0x874, 0x878, 0x880,
	0x884, 0x888, 0x890, 0x894, 0x898,
	0x8A0, 0x8A4, 0x8A8, 0x8B0, 0x8B4,
	0x8B8, 0x900, 0x904, 0x908, 0x910,
	0x914, 0x918, 0x920, 0x924, 0x928,
	0x930, 0x934, 0x938, 0x940, 0x944,
	0x948, 0x950, 0x954, 0x958, 0x960,
	0x964, 0x968, 0x970, 0x974, 0x978,
	0x600, 0x604, 0x608, 0x60C, 0x620,
	0x624, 0x628, 0x62C
};

u32 trim_perch_regs_emc01_addr_table[10] = {
	0x359C, 0x45A0, 0x3588, 0x4588,
	0x358C, 0x458C, 0x35AC, 0x45B8,
	0x35BC, 0x45C4
};

u32 burst_mc_regs_addr_table[33] = {
	0x090, 0x094, 0x6F0, 0x6F4, 0x098,
	0x09C, 0x0A0, 0x0A4, 0x0A8, 0x0AC,
	0x0B0, 0x0B4, 0x0B8, 0x0BC, 0x0C0,
	0x6C4, 0x0C4, 0x6C0, 0x0D0, 0x0D4,
	0x0D8, 0x0DC, 0x0C8, 0x0E0, 0xBCC,
	0xBD0, 0xBD4, 0xBD8, 0xBDC, 0xBE0,
	0xBE4, 0xBE8, 0xBEC
};

u32 la_scale_regs_mc_addr_table[24] = {
	0x44C, 0x50C, 0x960, 0x37C, 0x380,
	0x390, 0x3B8, 0x3BC, 0x3C0, 0x3C4,
	0x344, 0x348, 0x320, 0x310, 0x314,
	0x2E4, 0x3AC, 0x3E8, 0x328, 0x3D8,
	0x394, 0x398, 0x370, 0x374
};

u32 ram_pattern_dq_table[0x500] = {
	/* DQ RAM Patterns Table 0 */
	0x18181818, 0x61616161, 0x85858585, 0x14141414, 0x51515151,
	0x47474747, 0x1E1E1E1E, 0x79797979, 0xE5E5E5E5, 0x94949494,
	0x51515151, 0x46464646, 0x19191919, 0x67676767, 0x9C9C9C9C,
	0x71717171, 0xC5C5C5C5, 0x17171717, 0x5F5F5F5F, 0x7E7E7E7E,
	0xFBFBFBFB, 0xEDEDEDED, 0xB4B4B4B4, 0xD2D2D2D2, 0x48484848,
	0x21212121, 0x85858585, 0x16161616, 0x59595959, 0x66666666,
	0x9A9A9A9A, 0x69696969, 0xA4A4A4A4, 0x93939393, 0x4F4F4F4F,
	0x3F3F3F3F, 0xFCFCFCFC, 0xF3F3F3F3, 0xCDCDCDCD, 0x37373737,
	0xDCDCDCDC, 0x70707070, 0xC3C3C3C3, 0x0F0F0F0F, 0x3E3E3E3E,
	0xFAFAFAFA, 0xEBEBEBEB, 0xACACACAC, 0xB3B3B3B3, 0xCCCCCCCC,
	0x31313131, 0xC5C5C5C5, 0x15151515, 0x57575757, 0x5F5F5F5F,
	0x7F7F7F7F, 0xFDFDFDFD, 0xF4F4F4F4, 0xD0D0D0D0, 0x42424242,
	0x08080808, 0x23232323, 0x8F8F8F8F, 0x3F3F3F3F, 0x18181818,
	0x61616161, 0x85858585, 0x14141414, 0x51515151, 0x47474747,
	0x1E1E1E1E, 0x79797979, 0xE5E5E5E5, 0x94949494, 0x51515151,
	0x46464646, 0x19191919, 0x67676767, 0x9C9C9C9C, 0x71717171,
	0xC5C5C5C5, 0x17171717, 0x5F5F5F5F, 0x7E7E7E7E, 0xFBFBFBFB,
	0xEDEDEDED, 0xB4B4B4B4, 0xD2D2D2D2, 0x48484848, 0x21212121,
	0x85858585, 0x16161616, 0x59595959, 0x66666666, 0x9A9A9A9A,
	0x69696969, 0xA4A4A4A4, 0x93939393, 0x4F4F4F4F, 0x3F3F3F3F,
	0xFCFCFCFC, 0xF3F3F3F3, 0xCDCDCDCD, 0x37373737, 0xDCDCDCDC,
	0x70707070, 0xC3C3C3C3, 0x0F0F0F0F, 0x3E3E3E3E, 0xFAFAFAFA,
	0xEBEBEBEB, 0xACACACAC, 0xB3B3B3B3, 0xCCCCCCCC, 0x31313131,
	0xC5C5C5C5, 0x15151515, 0x57575757, 0x5F5F5F5F, 0x7F7F7F7F,
	0xFDFDFDFD, 0xF4F4F4F4, 0xD0D0D0D0, 0x42424242, 0x08080808,
	0x23232323, 0x8F8F8F8F, 0x3F3F3F3F, 0x06060606, 0x18181818,
	0x21212121, 0x05050505, 0x14141414, 0x11111111, 0x07070707,
	0x1E1E1E1E, 0x39393939, 0x25252525, 0x14141414, 0x11111111,
	0x06060606, 0x19191919, 0x27272727, 0x1C1C1C1C, 0x31313131,
	0x05050505, 0x17171717, 0x1F1F1F1F, 0x3E3E3E3E, 0x3B3B3B3B,
	0x2D2D2D2D, 0x34343434, 0x12121212, 0x08080808, 0x21212121,
	0x05050505, 0x16161616, 0x19191919, 0x26262626, 0x1A1A1A1A,
	0x29292929, 0x24242424, 0x13131313, 0x0F0F0F0F, 0x3F3F3F3F,
	0x3C3C3C3C, 0x33333333, 0x0D0D0D0D, 0x37373737, 0x1C1C1C1C,
	0x30303030, 0x03030303, 0x0F0F0F0F, 0x3E3E3E3E, 0x3A3A3A3A,
	0x2B2B2B2B, 0x2C2C2C2C, 0x33333333, 0x0C0C0C0C, 0x31313131,
	0x05050505, 0x15151515, 0x17171717, 0x1F1F1F1F, 0x3F3F3F3F,
	0x3D3D3D3D, 0x34343434, 0x10101010, 0x02020202, 0x08080808,
	0x23232323, 0x0F0F0F0F, 0x06060606, 0x18181818, 0x21212121,
	0x05050505, 0x14141414, 0x11111111, 0x07070707, 0x1E1E1E1E,
	0x39393939, 0x25252525, 0x14141414, 0x11111111, 0x06060606,
	0x19191919, 0x27272727, 0x1C1C1C1C, 0x31313131, 0x05050505,
	0x17171717, 0x1F1F1F1F, 0x3E3E3E3E, 0x3B3B3B3B, 0x2D2D2D2D,
	0x34343434, 0x12121212, 0x08080808, 0x21212121, 0x05050505,
	0x16161616, 0x19191919, 0x26262626, 0x1A1A1A1A, 0x29292929,
	0x24242424, 0x13131313, 0x0F0F0F0F, 0x3F3F3F3F, 0x3C3C3C3C,
	0x33333333, 0x0D0D0D0D, 0x37373737, 0x1C1C1C1C, 0x30303030,
	0x03030303, 0x0F0F0F0F, 0x3E3E3E3E, 0x3A3A3A3A, 0x2B2B2B2B,
	0x2C2C2C2C, 0x33333333, 0x0C0C0C0C, 0x31313131, 0x05050505,
	0x15151515, 0x17171717, 0x1F1F1F1F, 0x3F3F3F3F, 0x3D3D3D3D,
	0x34343434, 0x10101010, 0x02020202, 0x08080808, 0x23232323,
	0x0F0F0F0F,

	/* DQ RAM Patterns Table 1 */
	0x00000000, 0x00000000, 0xFFFFFFFF, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF,
	0x00000000, 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000,
	0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000,
	0x00000000, 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000,
	0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000,
	0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000,
	0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0x00000000,
	0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
	0x00000000, 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000,
	0x00000000, 0x00000000, 0xFFFFFFFF, 0x00000000, 0x00000000,
	0x00000000, 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF, 0x00000000,
	0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000,
	0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF,
	0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000,
	0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF,
	0x00000000, 0xFFFFFFFF, 0x00000000, 0x00000000, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000,
	0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000,
	0x00000000, 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000,
	0x3F3F3F3F, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x3F3F3F3F, 0x3F3F3F3F, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F,
	0x00000000, 0x00000000, 0x00000000, 0x3F3F3F3F, 0x3F3F3F3F,
	0x3F3F3F3F, 0x3F3F3F3F, 0x00000000, 0x00000000, 0x3F3F3F3F,
	0x00000000, 0x00000000, 0x00000000, 0x3F3F3F3F, 0x00000000,
	0x3F3F3F3F, 0x3F3F3F3F, 0x00000000, 0x00000000, 0x3F3F3F3F,
	0x3F3F3F3F, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000,
	0x3F3F3F3F, 0x00000000, 0x00000000, 0x3F3F3F3F, 0x3F3F3F3F,
	0x3F3F3F3F, 0x3F3F3F3F, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x3F3F3F3F,
	0x3F3F3F3F, 0x3F3F3F3F, 0x00000000, 0x00000000, 0x00000000,
	0x3F3F3F3F, 0x00000000, 0x00000000, 0x00000000, 0x3F3F3F3F,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x3F3F3F3F, 0x3F3F3F3F, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000,
	0x00000000, 0x00000000, 0x3F3F3F3F, 0x3F3F3F3F, 0x3F3F3F3F,
	0x3F3F3F3F, 0x00000000, 0x00000000, 0x3F3F3F3F, 0x00000000,
	0x00000000, 0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F,
	0x3F3F3F3F, 0x00000000, 0x00000000, 0x3F3F3F3F, 0x3F3F3F3F,
	0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F,
	0x00000000, 0x00000000, 0x3F3F3F3F, 0x3F3F3F3F, 0x3F3F3F3F,
	0x3F3F3F3F, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x3F3F3F3F, 0x3F3F3F3F,
	0x3F3F3F3F, 0x00000000, 0x00000000, 0x00000000, 0x3F3F3F3F,
	0x00000000,

	/* DQ RAM Patterns Table 2 */
	0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000,
	0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF,
	0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000,
	0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF,
	0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000,
	0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF,
	0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000,
	0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF,
	0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000,
	0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF,
	0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000,
	0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF,
	0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000,
	0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF,
	0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000,
	0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF,
	0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000,
	0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF,
	0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000,
	0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF,
	0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000,
	0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF,
	0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000,
	0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF,
	0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000,
	0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0x3F3F3F3F,
	0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000,
	0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F,
	0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000,
	0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F,
	0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000,
	0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F,
	0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000,
	0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F,
	0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000,
	0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F,
	0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000,
	0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F,
	0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000,
	0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F,
	0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000,
	0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F,
	0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000,
	0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F,
	0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000,
	0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F,
	0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000,
	0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F,
	0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000,
	0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F,
	0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000,
	0x3F3F3F3F,

	/* DQ RAM Patterns Table 3 */
	0x80808080, 0x00000000, 0x80808080, 0x00000000, 0x80808080,
	0x00000000, 0x80808080, 0x40404040, 0x00000000, 0x40404040,
	0x00000000, 0x40404040, 0x00000000, 0x40404040, 0x20202020,
	0x00000000, 0x20202020, 0x00000000, 0x20202020, 0x00000000,
	0x20202020, 0x10101010, 0x00000000, 0x10101010, 0x00000000,
	0x10101010, 0x00000000, 0x10101010, 0x08080808, 0x00000000,
	0x08080808, 0x00000000, 0x08080808, 0x00000000, 0x08080808,
	0x04040404, 0x00000000, 0x04040404, 0x00000000, 0x04040404,
	0x00000000, 0x04040404, 0x02020202, 0x00000000, 0x02020202,
	0x00000000, 0x02020202, 0x00000000, 0x02020202, 0x01010101,
	0x00000000, 0x01010101, 0x00000000, 0x01010101, 0x00000000,
	0x01010101, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x80808080,
	0x00000000, 0x80808080, 0x00000000, 0x80808080, 0x00000000,
	0x80808080, 0x40404040, 0x00000000, 0x40404040, 0x00000000,
	0x40404040, 0x00000000, 0x40404040, 0x20202020, 0x00000000,
	0x20202020, 0x00000000, 0x20202020, 0x00000000, 0x20202020,
	0x10101010, 0x00000000, 0x10101010, 0x00000000, 0x10101010,
	0x00000000, 0x10101010, 0x08080808, 0x00000000, 0x08080808,
	0x00000000, 0x08080808, 0x00000000, 0x08080808, 0x04040404,
	0x00000000, 0x04040404, 0x00000000, 0x04040404, 0x00000000,
	0x04040404, 0x02020202, 0x00000000, 0x02020202, 0x00000000,
	0x02020202, 0x00000000, 0x02020202, 0x01010101, 0x00000000,
	0x01010101, 0x00000000, 0x01010101, 0x00000000, 0x01010101,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x20202020,
	0x00000000, 0x20202020, 0x00000000, 0x20202020, 0x00000000,
	0x20202020, 0x00000000, 0x20202020, 0x00000000, 0x10101010,
	0x00000000, 0x10101010, 0x00000000, 0x10101010, 0x00000000,
	0x10101010, 0x00000000, 0x10101010, 0x00000000, 0x08080808,
	0x00000000, 0x08080808, 0x00000000, 0x08080808, 0x00000000,
	0x08080808, 0x00000000, 0x08080808, 0x00000000, 0x04040404,
	0x00000000, 0x04040404, 0x00000000, 0x04040404, 0x00000000,
	0x04040404, 0x00000000, 0x04040404, 0x00000000, 0x02020202,
	0x00000000, 0x02020202, 0x00000000, 0x02020202, 0x00000000,
	0x02020202, 0x00000000, 0x02020202, 0x00000000, 0x01010101,
	0x00000000, 0x01010101, 0x00000000, 0x01010101, 0x00000000,
	0x01010101, 0x00000000, 0x01010101, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x20202020, 0x00000000,
	0x20202020, 0x00000000, 0x20202020, 0x00000000, 0x20202020,
	0x00000000, 0x20202020, 0x00000000, 0x10101010, 0x00000000,
	0x10101010, 0x00000000, 0x10101010, 0x00000000, 0x10101010,
	0x00000000, 0x10101010, 0x00000000, 0x08080808, 0x00000000,
	0x08080808, 0x00000000, 0x08080808, 0x00000000, 0x08080808,
	0x00000000, 0x08080808, 0x00000000, 0x04040404, 0x00000000,
	0x04040404, 0x00000000, 0x04040404, 0x00000000, 0x04040404,
	0x00000000, 0x04040404, 0x00000000, 0x02020202, 0x00000000,
	0x02020202, 0x00000000, 0x02020202, 0x00000000, 0x02020202,
	0x00000000, 0x02020202, 0x00000000, 0x01010101, 0x00000000,
	0x01010101, 0x00000000, 0x01010101, 0x00000000, 0x01010101,
	0x00000000, 0x01010101, 0x00000000, 0x00000000, 0x00000000,
	0x00000000,

	/* DQ RAM Patterns Table 4 */
	0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA,
	0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555,
	0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC,
	0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333,
	0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA,
	0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555,
	0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC,
	0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333,
	0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA,
	0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555,
	0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC,
	0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333,
	0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA,
	0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555,
	0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC,
	0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333,
	0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA,
	0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555,
	0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC,
	0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333,
	0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA,
	0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555,
	0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC,
	0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333,
	0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA,
	0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555,
	0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC,
	0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333,
	0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA,
	0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555,
	0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC,
	0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333,
	0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA,
	0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555,
	0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC,
	0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333,
	0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA,
	0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555,
	0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC,
	0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333,
	0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA,
	0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555,
	0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC,
	0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333,
	0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA,
	0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555,
	0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC,
	0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333,
	0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA,
	0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555,
	0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC,
	0x33333333
};

u32 ram_pattern_dmi_table[0x500] = {
	/* DMI RAM Patterns Table 0 */
	0xF, 0xF, 0x0, 0xF, 0xF, 0x0, 0xF, 0xF,
	0x0, 0xF, 0x0, 0xF, 0xF, 0x0, 0xF, 0xF,
	0xF, 0xF, 0x0, 0xF, 0xF, 0x0, 0x0, 0x0,
	0xF, 0xF, 0x0, 0xF, 0x0, 0x0, 0xF, 0x0,
	0xF, 0xF, 0xF, 0x0, 0xF, 0xF, 0xF, 0x0,
	0x0, 0xF, 0xF, 0x0, 0x0, 0xF, 0x0, 0xF,
	0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,
	0x0, 0x0, 0x0, 0x0, 0xF, 0xF, 0xF, 0x0,
	0xF, 0xF, 0x0, 0xF, 0xF, 0x0, 0xF, 0xF,
	0x0, 0xF, 0x0, 0xF, 0xF, 0x0, 0xF, 0xF,
	0xF, 0xF, 0x0, 0xF, 0xF, 0x0, 0x0, 0x0,
	0xF, 0xF, 0x0, 0xF, 0x0, 0x0, 0xF, 0x0,
	0xF, 0xF, 0xF, 0x0, 0xF, 0xF, 0xF, 0x0,
	0x0, 0xF, 0xF, 0x0, 0x0, 0xF, 0x0, 0xF,
	0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,
	0x0, 0x0, 0x0, 0x0, 0xF, 0xF, 0xF, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,

	/* DMI RAM Patterns Table 1 */
	0x0, 0x0, 0xF, 0x0, 0x0, 0x0, 0x0, 0x0,
	0xF, 0xF, 0x0, 0x0, 0x0, 0x0, 0xF, 0x0,
	0xF, 0x0, 0x0, 0x0, 0xF, 0xF, 0xF, 0xF,
	0x0, 0x0, 0xF, 0x0, 0x0, 0x0, 0xF, 0x0,
	0xF, 0xF, 0x0, 0x0, 0xF, 0xF, 0xF, 0x0,
	0xF, 0x0, 0xF, 0x0, 0x0, 0xF, 0xF, 0xF,
	0xF, 0xF, 0x0, 0xF, 0x0, 0x0, 0x0, 0x0,
	0xF, 0xF, 0xF, 0x0, 0x0, 0x0, 0xF, 0x0,
	0x0, 0x0, 0xF, 0x0, 0x0, 0x0, 0x0, 0x0,
	0xF, 0xF, 0x0, 0x0, 0x0, 0x0, 0xF, 0x0,
	0xF, 0x0, 0x0, 0x0, 0xF, 0xF, 0xF, 0xF,
	0x0, 0x0, 0xF, 0x0, 0x0, 0x0, 0xF, 0x0,
	0xF, 0xF, 0x0, 0x0, 0xF, 0xF, 0xF, 0x0,
	0xF, 0x0, 0xF, 0x0, 0x0, 0xF, 0xF, 0xF,
	0xF, 0xF, 0x0, 0xF, 0x0, 0x0, 0x0, 0x0,
	0xF, 0xF, 0xF, 0x0, 0x0, 0x0, 0xF, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,

	/* DMI RAM Patterns Table 2 */
	0x0, 0xF, 0x0, 0xF, 0x0, 0xF, 0x0, 0xF,
	0x0, 0xF, 0x0, 0xF, 0x0, 0xF, 0x0, 0xF,
	0x0, 0xF, 0x0, 0xF, 0x0, 0xF, 0x0, 0xF,
	0x0, 0xF, 0x0, 0xF, 0x0, 0xF, 0x0, 0xF,
	0x0, 0xF, 0x0, 0xF, 0x0, 0xF, 0x0, 0xF,
	0x0, 0xF, 0x0, 0xF, 0x0, 0xF, 0x0, 0xF,
	0x0, 0xF, 0x0, 0xF, 0x0, 0xF, 0x0, 0xF,
	0x0, 0xF, 0x0, 0xF, 0x0, 0xF, 0x0, 0xF,
	0x0, 0xF, 0x0, 0xF, 0x0, 0xF, 0x0, 0xF,
	0x0, 0xF, 0x0, 0xF, 0x0, 0xF, 0x0, 0xF,
	0x0, 0xF, 0x0, 0xF, 0x0, 0xF, 0x0, 0xF,
	0x0, 0xF, 0x0, 0xF, 0x0, 0xF, 0x0, 0xF,
	0x0, 0xF, 0x0, 0xF, 0x0, 0xF, 0x0, 0xF,
	0x0, 0xF, 0x0, 0xF, 0x0, 0xF, 0x0, 0xF,
	0x0, 0xF, 0x0, 0xF, 0x0, 0xF, 0x0, 0xF,
	0x0, 0xF, 0x0, 0xF, 0x0, 0xF, 0x0, 0xF,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,

	/* DMI RAM Patterns Table 3 */
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0xF, 0x0, 0xF, 0x0, 0xF, 0x0, 0xF, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0xF, 0x0, 0xF, 0x0, 0xF, 0x0, 0xF, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,

	/* DMI RAM Patterns Table 4 */
	0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
	0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
	0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
	0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
	0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
	0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
	0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
	0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
	0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
	0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
	0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
	0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
	0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
	0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
	0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
	0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
	0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
	0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
	0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
	0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
	0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
	0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
	0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
	0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
	0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
	0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
	0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
	0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
	0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
	0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
	0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
	0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3
};

u32 periodic_training_addr[10] =
{
	EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_0,
	EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_1,
	EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_2,
	EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_3,
	EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_0,
	EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_1,
	EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_2,
	EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_3,
	EMC_DATA_BRLSHFT_0,
	EMC_DATA_BRLSHFT_1
};

s32 _fceil(float var)
{
	s32 result = (s32)(var + 0.5f);

	return result;
}

u32 _pllm_clk_base_cfg(s32 rate_KHz, s32 min_rate_KHz, u32 clk_src_emc, s32 emc_2X_clk_src_is_PLLMB)
{
	u32 dividers = 0;
	s32 i = 0;
	pllm_clk_config_t *pllm_clk_config;

	for (i = 0; pllm_clk_config_table[i].rate_min; i++)
	{
		if (pllm_clk_config_table[i].rate_min == min_rate_KHz && pllm_clk_config_table[i].rate_dst == rate_KHz)
			break;
	}

	pllm_clk_config = &pllm_clk_config_table[i];
	if (pllm_clk_config->rate_min)
	{
		dividers = pllm_clk_config->pll_input_div | (pllm_clk_config->pll_feedback_div << 8) | ((pllm_clk_config->pll_post_div & 0x1F) << 20);
		if (emc_2X_clk_src_is_PLLMB)
		{
			CLOCK(CLK_RST_CONTROLLER_PLLMB_BASE) = dividers;
			CLOCK(CLK_RST_CONTROLLER_PLLMB_BASE) |= PLLM_ENABLE;
			if ((clk_src_emc >> EMC_2X_CLK_SRC_SHIFT) == PLLM_UD)
				clk_src_emc = (clk_src_emc & 0x1FFFFFFF) | (PLLMB_UD << EMC_2X_CLK_SRC_SHIFT);
			else if (!(clk_src_emc >> EMC_2X_CLK_SRC_SHIFT))
				clk_src_emc |= (PLLMB_OUT0 << EMC_2X_CLK_SRC_SHIFT);
			while (!(CLOCK(CLK_RST_CONTROLLER_PLLMB_BASE) & PLLM_LOCK))
				;
		}
		else
		{
			CLOCK(CLK_RST_CONTROLLER_PLLM_BASE) = dividers;
			CLOCK(CLK_RST_CONTROLLER_PLLM_MISC2) |= 0x10u; // PLLM_EN_LCKDET.
			CLOCK(CLK_RST_CONTROLLER_PLLM_BASE) |= PLLM_ENABLE;
			if ((clk_src_emc >> EMC_2X_CLK_SRC_SHIFT) == PLLM_UD)
				clk_src_emc = (clk_src_emc & 0x1FFFFFFF) | (PLLM_UD << EMC_2X_CLK_SRC_SHIFT);
			while (!(CLOCK(CLK_RST_CONTROLLER_PLLM_BASE) & PLLM_LOCK))
				;
		}
	}
	return clk_src_emc;
}

void _ccfifo_write(u32 addr, u32 data_val, u32 delay) //addr and delay are u16
{
	EMC(EMC_CCFIFO_DATA) = data_val;
	EMC(EMC_CCFIFO_ADDR) = (addr & 0xffff) | ((delay & 0x7FFF) << 16) | (1 << 31);
}

u32 _start_periodic_compensation()
{
	EMC(EMC_MPC) = 0x4B;

	return EMC(EMC_MPC);
}

u32 _actual_osc_clocks(u32 in)
{
	u32 actual_clock;

	actual_clock = 16 * in;
	if (in > 63)
	{
		actual_clock = 2048;
		if (in > 127)
		{
			if (in >= 192)
				actual_clock = 8192;
			else
				actual_clock = 4096;
		}
	}

	return actual_clock;
}

bool _wait_emc_status(u32 reg_offset, u32 bit_mask, bool updated_state, s32 emc_channel)
{
	bool err = true;

	for (s32 i = 0; i < EMC_STATUS_UPDATE_TIMEOUT; i++)
	{
		if (emc_channel)
		{
			if (emc_channel != 1)
				goto done;
			if (((EMC_CH1(reg_offset) & bit_mask) != 0) == updated_state)
			{
				err = false;
				break;
			}
		}
		else
		{
			if (((EMC(reg_offset) & bit_mask) != 0) == updated_state)
			{
				err = false;
				break;
			}
		}
		_usleep(1);
	}
done:
	return err;
}

bool _timing_update(s32 dual_channel)
{
	bool err = 0;

	EMC(EMC_TIMING_CONTROL) = 1;
	err = _wait_emc_status(EMC_EMC_STATUS, TIMING_UPDATE_STALLED, false, EMC_CH0);
	if (dual_channel)
		err |= _wait_emc_status(EMC_EMC_STATUS, TIMING_UPDATE_STALLED, false, EMC_CH1);

	return err;
}

void _change_dll_src(emc_table_t *mtc_table_entry, u32 clk_src_emc)
{
	u32 emc_2x_clk_src = clk_src_emc >> EMC_2X_CLK_SRC_SHIFT;

	u32 dll_setting = ((((mtc_table_entry->dll_clk_src & 0x1FFFFFFF)
			| (emc_2x_clk_src << EMC_2X_CLK_SRC_SHIFT)) & 0xFFFFFF00)
		| (clk_src_emc & 0xFF)) & 0xFFFFF3FF;

	if (emc_2x_clk_src == PLLMB_UD)
		dll_setting |= 0x400; // PLLM_VCOB.
	else if (emc_2x_clk_src != PLLM_UD)
		dll_setting |= 0x800; // EMC_DLL_SWITCH_OUT.

	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_EMC_DLL) = dll_setting;

	//OLD
	u32 clk_enb_emc_dll = ((mtc_table_entry->clk_out_enb_x_0_clk_enb_emc_dll & 1) << 14) | (CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_X) & 0xFFFFBFFF);
	CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_X) = clk_enb_emc_dll;

	//NEW
	// u32 temp = CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_X);
	// _usleep(2);
	// if (mtc_table_entry->clk_out_enb_x_0_clk_enb_emc_dll)
	// 	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_X_SET) |= 0x4000;
	// else
	// 	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_X_CLR) |= 0x4000;
	// temp = CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_X);
	// _usleep(2);
}

u32 _digital_dll_prelock(emc_table_t *mtc_table_entry, u32 needs_tristate_training, u32 selected_clk_src_emc)
{
	s32 dual_channel = (EMC(EMC_FBIO_CFG7) >> 1) & ((EMC(EMC_FBIO_CFG7) >> 2) & 1);

	EMC(EMC_CFG_DIG_DLL) = (EMC(EMC_CFG_DIG_DLL) & 0xFFFFF824) | 0x3C8;

	_timing_update(dual_channel);

	while (EMC(EMC_CFG_DIG_DLL) & 1)
		;
	if (dual_channel)
		while (EMC_CH1(EMC_CFG_DIG_DLL) & 1)
			;

	EMC(EMC_DLL_CFG_0) = mtc_table_entry->burst_regs.emc_dll_cfg_0_idx;
	EMC(EMC_DLL_CFG_1) = mtc_table_entry->burst_regs.emc_dll_cfg_1_idx;

	_change_dll_src(mtc_table_entry, selected_clk_src_emc);

	EMC(EMC_CFG_DIG_DLL) |= 1;
	
	_timing_update(dual_channel);

	while (!(EMC(EMC_CFG_DIG_DLL) & 1))
		;
	if (dual_channel)
		while (!(EMC_CH1(EMC_CFG_DIG_DLL) & 1))
			;

	while ((((EMC(EMC_DIG_DLL_STATUS) >> 17) & 1) ^ 1) | (((EMC(EMC_DIG_DLL_STATUS) >> 15) & 1) ^ 1))
		;

	if (needs_tristate_training)
	{
		EMC(EMC_DBG) |= 2u;
		EMC(EMC_CFG_DIG_DLL) &= 0xFFFFFFFE; //Disable CFG_DLL_EN: [PMC] Enable digital DLL.
		EMC(EMC_DBG) &= 0xFFFFFFFD;
		while (EMC(EMC_CFG_DIG_DLL) & 1)
			;
		if (dual_channel)
			while (EMC_CH1(EMC_CFG_DIG_DLL) & 1)
				;
	}

	return EMC(EMC_DIG_DLL_STATUS) & 0x7FF;
}

void _digital_dll_disable()
{
	bool dual_channel = (EMC(EMC_FBIO_CFG7) >> 1) & ((EMC(EMC_FBIO_CFG7) >> 2) & 1);

	EMC(EMC_CFG_DIG_DLL) &= 0xFFFFFFFE;

	_timing_update(dual_channel);

	while (EMC(EMC_CFG_DIG_DLL) & 1)
		;
	if (dual_channel)
	{
		while (EMC_CH1(EMC_CFG_DIG_DLL) & 1)
			;
	}
}

void _digital_dll_enable(s32 channel1_enabled)
{
	EMC(EMC_CFG_DIG_DLL) |= 1;

	_timing_update(channel1_enabled);

	while (!(EMC(EMC_CFG_DIG_DLL) & 1))
		;
	if (channel1_enabled)
	{
		while (!(EMC_CH1(EMC_CFG_DIG_DLL) & 1))
			;
	}
}

void _digital_dll_enable_rs(s32 channel1_enabled)
{
	EMC(EMC_CFG_DIG_DLL) = (EMC(EMC_CFG_DIG_DLL) & 0xFFFFFF24) | 0x89;

	_timing_update(channel1_enabled);

	while (!(EMC(EMC_CFG_DIG_DLL) & 1))
		;
	if (channel1_enabled)
	{
		while (!(EMC_CH1(EMC_CFG_DIG_DLL) & 1))
			;
	}
}

u32 _dvfs_power_ramp_down(bool flip_backward, emc_table_t *src_emc_table_entry, emc_table_t *dst_emc_table_entry, float src_clock_period)
{
	u32 pmacro_cmd_pad;
	u32 pmacro_rfu1;
	u32 pmacro_cfg5;
	u32 pmacro_common_tx;
	u32 pmacro_dq_pad;

	float src_clk_per_pc = (100.0f / src_clock_period) + 1.0f;

	if (flip_backward)
	{
		pmacro_cmd_pad = dst_emc_table_entry->burst_regs.emc_pmacro_cmd_pad_tx_ctrl_idx;
		pmacro_dq_pad = dst_emc_table_entry->burst_regs.emc_pmacro_data_pad_tx_ctrl_idx;
		pmacro_rfu1 = dst_emc_table_entry->burst_regs.emc_pmacro_brick_ctrl_rfu1_idx;
		pmacro_cfg5 = dst_emc_table_entry->burst_regs.emc_fbio_cfg5_idx;
		pmacro_common_tx = dst_emc_table_entry->burst_regs.emc_pmacro_common_pad_tx_ctrl_idx;
	}
	else
	{
		pmacro_cmd_pad = src_emc_table_entry->burst_regs.emc_pmacro_cmd_pad_tx_ctrl_idx;
		pmacro_dq_pad = (dst_emc_table_entry->burst_regs.emc_pmacro_data_pad_tx_ctrl_idx & 0x101) | src_emc_table_entry->burst_regs.emc_pmacro_data_pad_tx_ctrl_idx;
		pmacro_rfu1 = src_emc_table_entry->burst_regs.emc_pmacro_brick_ctrl_rfu1_idx;
		pmacro_cfg5 = src_emc_table_entry->burst_regs.emc_fbio_cfg5_idx;
		pmacro_common_tx = src_emc_table_entry->burst_regs.emc_pmacro_common_pad_tx_ctrl_idx;
	}
	u32 pmacro_cmd_pad_drvforceon = pmacro_cmd_pad | 0x4000000;

	u32 ramp_down_wait = (u32)(float)(src_clock_period * 12.0f);

	_ccfifo_write(EMC_PMACRO_CMD_PAD_TX_CTRL, pmacro_cmd_pad_drvforceon, 0);
	_ccfifo_write(EMC_FBIO_CFG5, pmacro_cfg5 | 0x100, 12);

	if (src_clock_period >= 1.0f) // Dvfs high speed threshold.
	{
		_ccfifo_write(EMC_PMACRO_BRICK_CTRL_RFU1, pmacro_rfu1 & 0xF800F800, (u32)(float)(src_clk_per_pc + 19.0f));
		ramp_down_wait = (u32)(float)((float)ramp_down_wait + (100.0f + (src_clock_period * 20.0f)));
	}
	else
	{
		ramp_down_wait += 100;
		if (src_clock_period >= 0.416666667) // Iobrick dcc threshold.
			_ccfifo_write(EMC_PMACRO_BRICK_CTRL_RFU1, pmacro_rfu1 & 0xFEEDFEED, (u32)src_clk_per_pc);
		else
		{
			pmacro_dq_pad = (pmacro_dq_pad & 0xFEFEFDFD) | 0x10200;
			pmacro_cmd_pad_drvforceon = (pmacro_cmd_pad & 0xFEFEFDFD) | 0x4010200;
			_ccfifo_write(EMC_PMACRO_CMD_PAD_TX_CTRL, pmacro_cmd_pad_drvforceon, (u32)src_clk_per_pc);
			_ccfifo_write(EMC_PMACRO_DATA_PAD_TX_CTRL, pmacro_dq_pad, 0);
			_ccfifo_write(EMC_PMACRO_BRICK_CTRL_RFU1, pmacro_rfu1 & 0xFEEDFEED, 0);
		}
		ramp_down_wait += 200;
		_ccfifo_write(EMC_PMACRO_BRICK_CTRL_RFU1, pmacro_rfu1 & 0xFE40FE40, (u32)src_clk_per_pc);
		if (src_clock_period >= 0.416666667) // Iobrick dcc threshold.
			_ccfifo_write(EMC_PMACRO_BRICK_CTRL_RFU1, pmacro_rfu1 & 0xF800F800, (u32)src_clk_per_pc);
		else
		{
			_ccfifo_write(EMC_PMACRO_CMD_PAD_TX_CTRL, pmacro_cmd_pad_drvforceon & 0xFEFEFDFD, (u32)src_clk_per_pc);
			_ccfifo_write(EMC_PMACRO_DATA_PAD_TX_CTRL, pmacro_dq_pad & 0xFEFEFDFD, 0);
			_ccfifo_write(EMC_PMACRO_BRICK_CTRL_RFU1, pmacro_rfu1 & 0xF800F800, 0);
		}
	}
	if (src_clock_period >= 1.66666667) // Dvfs mid speed threshold.
	{
		_ccfifo_write(EMC_PMACRO_COMMON_PAD_TX_CTRL, pmacro_common_tx & 0xFFFFFFF0, (u32)src_clk_per_pc);
	}
	else
	{
		ramp_down_wait += 400;
		_ccfifo_write(EMC_PMACRO_COMMON_PAD_TX_CTRL, pmacro_common_tx & 0xFFFFFFFA, (u32)src_clk_per_pc);
		_ccfifo_write(EMC_PMACRO_COMMON_PAD_TX_CTRL, pmacro_common_tx & 0xFFFFFFF0, (u32)src_clk_per_pc);
		_ccfifo_write(0, 0, (u32)src_clk_per_pc);
	}

	return ramp_down_wait;
}

u32 _dvfs_power_ramp_up(bool flip_backward, emc_table_t *src_emc_table_entry, emc_table_t *dst_emc_table_entry, u8 needs_training, float dst_clock_period)
{
	u32 pmacro_cmd_pad;
	u32 pmacro_dq_pad;
	u32 pmacro_rfu1;
	u32 pmacro_cfg5;
	u32 pmacro_common_tx;
	u32 pmacro_cmd_pad_data;
	u32 ramp_up_wait = 0;

	float dst_clk_per_pc = (100.0f / dst_clock_period) + 1.0f;
	if (flip_backward)
	{
		pmacro_cmd_pad = src_emc_table_entry->burst_regs.emc_pmacro_cmd_pad_tx_ctrl_idx;
		pmacro_dq_pad = src_emc_table_entry->burst_regs.emc_pmacro_data_pad_tx_ctrl_idx;
		pmacro_rfu1 = src_emc_table_entry->burst_regs.emc_pmacro_brick_ctrl_rfu1_idx;
		pmacro_cfg5 = src_emc_table_entry->burst_regs.emc_fbio_cfg5_idx;
		pmacro_common_tx = src_emc_table_entry->burst_regs.emc_pmacro_common_pad_tx_ctrl_idx;
	}
	else if (needs_training & 3)
	{
		pmacro_cmd_pad = dst_emc_table_entry->shadow_regs_ca_train.emc_pmacro_cmd_pad_tx_ctrl_idx;
		pmacro_dq_pad = dst_emc_table_entry->shadow_regs_ca_train.emc_pmacro_data_pad_tx_ctrl_idx;
		pmacro_rfu1 = dst_emc_table_entry->shadow_regs_ca_train.emc_pmacro_brick_ctrl_rfu1_idx;
		pmacro_cfg5 = dst_emc_table_entry->shadow_regs_ca_train.emc_fbio_cfg5_idx;
		pmacro_common_tx = dst_emc_table_entry->shadow_regs_ca_train.emc_pmacro_common_pad_tx_ctrl_idx;
	}
	else if (needs_training & 0xC)
	{
		pmacro_cmd_pad = dst_emc_table_entry->shadow_regs_quse_train.emc_pmacro_cmd_pad_tx_ctrl_idx;
		pmacro_dq_pad = dst_emc_table_entry->shadow_regs_quse_train.emc_pmacro_data_pad_tx_ctrl_idx;
		pmacro_rfu1 = dst_emc_table_entry->shadow_regs_quse_train.emc_pmacro_brick_ctrl_rfu1_idx;
		pmacro_cfg5 = dst_emc_table_entry->shadow_regs_quse_train.emc_fbio_cfg5_idx;
		pmacro_common_tx = dst_emc_table_entry->shadow_regs_quse_train.emc_pmacro_common_pad_tx_ctrl_idx;
	}
	else if (needs_training & 0xF0)
	{
		pmacro_cmd_pad = dst_emc_table_entry->shadow_regs_rdwr_train.emc_pmacro_cmd_pad_tx_ctrl_idx;
		pmacro_dq_pad = dst_emc_table_entry->shadow_regs_rdwr_train.emc_pmacro_data_pad_tx_ctrl_idx;
		pmacro_rfu1 = dst_emc_table_entry->shadow_regs_rdwr_train.emc_pmacro_brick_ctrl_rfu1_idx;
		pmacro_cfg5 = dst_emc_table_entry->shadow_regs_rdwr_train.emc_fbio_cfg5_idx;
		pmacro_common_tx = dst_emc_table_entry->shadow_regs_rdwr_train.emc_pmacro_common_pad_tx_ctrl_idx;
	}
	else
	{
		pmacro_cmd_pad = dst_emc_table_entry->burst_regs.emc_pmacro_cmd_pad_tx_ctrl_idx;
		pmacro_dq_pad = dst_emc_table_entry->burst_regs.emc_pmacro_data_pad_tx_ctrl_idx;
		pmacro_rfu1 = dst_emc_table_entry->burst_regs.emc_pmacro_brick_ctrl_rfu1_idx;
		pmacro_cfg5 = dst_emc_table_entry->burst_regs.emc_fbio_cfg5_idx;
		pmacro_common_tx = dst_emc_table_entry->burst_regs.emc_pmacro_common_pad_tx_ctrl_idx;
	}
	pmacro_cmd_pad_data = (pmacro_cmd_pad & 0xFEFEFDFD) | 0x4000000;

	if (dst_clock_period >= 1.66666667) // Dvfs mid speed threshold.
	{
		_ccfifo_write(EMC_PMACRO_COMMON_PAD_TX_CTRL, pmacro_common_tx | 8, 0);
		if (dst_clock_period >= 1.0) // Dvfs high speed threshold.
		{
			if (dst_clock_period >= 1.66666667) // Dvfs mid speed threshold.
			{
				_ccfifo_write(EMC_PMACRO_BRICK_CTRL_RFU1, pmacro_rfu1 | 0x600, 0);
				_ccfifo_write(EMC_FBIO_CFG5, pmacro_cfg5 & 0xFFFFFEFF, 12);
				ramp_up_wait = (u32)((float)(dst_clock_period * 12.0f) + 0.0);
			}
			else
			{
				_ccfifo_write(EMC_PMACRO_BRICK_CTRL_RFU1, pmacro_rfu1 | 0x6000600, (u32)dst_clk_per_pc);
				_ccfifo_write(EMC_FBIO_CFG5, pmacro_cfg5 & 0xFFFFFEFF, (u32)(float)(dst_clk_per_pc + 9.0f));
				ramp_up_wait = (u32)(float)(100.0f + (float)(dst_clock_period * 10.0f));
			}
			_ccfifo_write(EMC_PMACRO_CMD_PAD_TX_CTRL, pmacro_cmd_pad_data & 0xFBFFFFFF, 5);

			return ramp_up_wait;
		}

		_ccfifo_write(EMC_PMACRO_BRICK_CTRL_RFU1, pmacro_rfu1 & 0xFE40FE40, (u32)dst_clk_per_pc);
		_ccfifo_write(EMC_PMACRO_BRICK_CTRL_RFU1, pmacro_rfu1 & 0xFEEDFEED, (u32)dst_clk_per_pc);
		_ccfifo_write(EMC_PMACRO_BRICK_CTRL_RFU1, pmacro_rfu1, (u32)dst_clk_per_pc);
		_ccfifo_write(EMC_FBIO_CFG5, pmacro_cfg5 & 0xFFFFFEFF, (u32)(float)(dst_clk_per_pc + 9.0f));
		ramp_up_wait = (u32)(float)((float)300 + (float)(100.0f + (float)(dst_clock_period * 10.0f)));
		_ccfifo_write(EMC_PMACRO_CMD_PAD_TX_CTRL, pmacro_cmd_pad_data & 0xFBFFFFFF, 5);

		return ramp_up_wait;
	}
	_ccfifo_write(EMC_PMACRO_COMMON_PAD_TX_CTRL, pmacro_common_tx & 0xA, 0);
	_ccfifo_write(EMC_PMACRO_COMMON_PAD_TX_CTRL, pmacro_common_tx & 0xF, (u32)dst_clk_per_pc);

	if (dst_clock_period < 1.0) // Dvfs high speed threshold.
	{
		if (dst_clock_period >= 0.416666667) // Iobrick dcc threshold.
			_ccfifo_write(EMC_PMACRO_BRICK_CTRL_RFU1, pmacro_rfu1 & 0xFE40FE40, (u32)dst_clk_per_pc);
		else
		{
			pmacro_cmd_pad_data = (pmacro_cmd_pad & 0xFEFEFDFD) | 0x4010200;
			pmacro_dq_pad = (pmacro_dq_pad & 0xFEFEFDFD) | 0x10200;
			_ccfifo_write(EMC_PMACRO_CMD_PAD_TX_CTRL, pmacro_cmd_pad_data, (u32)dst_clk_per_pc);
			_ccfifo_write(EMC_PMACRO_DATA_PAD_TX_CTRL, pmacro_dq_pad, 0);
			_ccfifo_write(EMC_PMACRO_BRICK_CTRL_RFU1, pmacro_rfu1 & 0xFE40FE40, 0);
		}

		_ccfifo_write(EMC_PMACRO_BRICK_CTRL_RFU1, pmacro_rfu1 & 0xFEEDFEED, (u32)dst_clk_per_pc);

		if (dst_clock_period >= 0.416666667) // Iobrick dcc threshold.
			_ccfifo_write(EMC_PMACRO_BRICK_CTRL_RFU1, pmacro_rfu1, (u32)dst_clk_per_pc);
		else
		{
			pmacro_cmd_pad_data |= 0x1010202u;
			pmacro_dq_pad |= 0x1010202;
			_ccfifo_write(EMC_PMACRO_CMD_PAD_TX_CTRL, pmacro_cmd_pad_data, (u32)dst_clk_per_pc);
			_ccfifo_write(EMC_PMACRO_DATA_PAD_TX_CTRL, pmacro_dq_pad, 0);
			_ccfifo_write(EMC_PMACRO_BRICK_CTRL_RFU1, pmacro_rfu1, 0);
		}

		_ccfifo_write(EMC_FBIO_CFG5, pmacro_cfg5 & 0xFFFFFEFF, (u32)(float)(dst_clk_per_pc + 9.0f));
		ramp_up_wait = (u32)(float)((float)400 + (float)(100.0f + (float)(dst_clock_period * 10.0f)));
		_ccfifo_write(EMC_PMACRO_CMD_PAD_TX_CTRL, pmacro_cmd_pad_data & 0xFBFFFFFF, 5);

		return ramp_up_wait;
	}

	// 1.0 > dst_clock_period < 1.66666667.
	_ccfifo_write(EMC_PMACRO_BRICK_CTRL_RFU1, pmacro_rfu1 | 0x6000600, (u32)dst_clk_per_pc);
	_ccfifo_write(EMC_FBIO_CFG5, pmacro_cfg5 & 0xFFFFFEFF, (u32)(float)(dst_clk_per_pc + 9.0f));

	ramp_up_wait = (u32)(float)((float)100 + (float)(100.0f + (float)(dst_clock_period * 10.0f)));
	_ccfifo_write(EMC_PMACRO_CMD_PAD_TX_CTRL, pmacro_cmd_pad_data & 0xFBFFFFFF, 5);

	return ramp_up_wait;
}

void _request_mmr_data(u32 data, bool dual_channel)
{
	EMC(EMC_MRR) = data;
	_wait_emc_status(EMC_EMC_STATUS, MRR_DIVLD, true, EMC_CH0);
	if (dual_channel)
		_wait_emc_status(EMC_EMC_STATUS, MRR_DIVLD, true, EMC_CH1);
}

/*
u32 _minerva_periodic_compensation_handler_old(emc_table_t *src_emc_entry, emc_table_t *dst_emc_entry, s32 dram_dev_num, s32 channel1_enabled, bool needs_wr_training)
{
	u32 temp_ch0_0 = 0;
	u32 temp_ch0_1 = 0;
	u32 temp_ch1_0 = 0;
	u32 temp_ch1_1 = 0;
	u32 dst_rate_MHz_128 = 0;
	s32 tree_margin = 0;
	u32 cval = 0;
	s32 tdel0_0 = 0;
	s32 tdel0_1 = 0;
	s32 tdel1_0 = 0;
	s32 tdel1_1 = 0;

	EPRINTFARGS("0x%08X, 0x%08X", src_emc_entry, dst_emc_entry);

	_request_mmr_data(0x80130000, channel1_enabled); // Dev0 MRR 19.
	temp_ch0_0 = (EMC(EMC_MRR) & 0xFF) << 8;
	temp_ch0_1 = EMC(EMC_MRR) & 0xFF00;
	if (channel1_enabled)
	{
		temp_ch1_0 = (EMC_CH1(EMC_MRR) & 0xFF) << 8;
		temp_ch1_1 = EMC_CH1(EMC_MRR) & 0xFF00;
	}

	_request_mmr_data(0x80120000, channel1_enabled); // Dev0 MRR 18.
	temp_ch0_0 |= EMC(EMC_MRR) & 0xFF;
	temp_ch0_1 |= (EMC(EMC_MRR) & 0xFF00) >> 8;
	if (channel1_enabled)
	{
		temp_ch1_0 |= EMC_CH1(EMC_MRR) & 0xFF;
		temp_ch1_1 |= (EMC_CH1(EMC_MRR) & 0xFF00) >> 8;
	}

	dst_rate_MHz_128 = (dst_emc_entry->rate_khz << 7) / 1000;

	//u64 delay = 1000000 * _actual_osc_clocks(src_emc_entry->run_clocks);
	//tree_margin = dst_emc_entry->tree_margin;
	//cval = delay / (src_emc_entry->rate_khz * 2 * temp_ch0_0);
	u32 delay = (u64)((u64)_actual_osc_clocks(src_emc_entry->run_clocks) * 1000000000) / (u64)(src_emc_entry->rate_khz * 2);
	tree_margin = dst_emc_entry->tree_margin;
	cval = delay / temp_ch0_0;
	tdel0_0 = abs((s32)(dst_emc_entry->current_dram_clktree_c0d0u0 - cval));
	if ((dst_rate_MHz_128 * tdel0_0 / 1000) > tree_margin || needs_wr_training)
		dst_emc_entry->current_dram_clktree_c0d0u0 = cval;
	cval = delay / temp_ch0_1;
	tdel0_1 = abs((s32)(dst_emc_entry->current_dram_clktree_c0d0u1 - cval));
	if (tdel0_1 >= tdel0_0)
		tdel0_0 = tdel0_1;
	if ((dst_rate_MHz_128 * tdel0_1 / 1000) > tree_margin || needs_wr_training)
		dst_emc_entry->current_dram_clktree_c0d0u1 = cval;

	if (channel1_enabled)
	{
		cval = delay / temp_ch1_0;
		tdel1_0 = abs((s32)(dst_emc_entry->current_dram_clktree_c1d0u0 - cval));
		if ((dst_rate_MHz_128 * tdel1_0 / 1000) > tree_margin || needs_wr_training)
			dst_emc_entry->current_dram_clktree_c1d0u0 = cval;
		cval = delay / temp_ch1_1;
		tdel1_1 = abs((s32)(dst_emc_entry->current_dram_clktree_c1d0u1 - cval));
		if (tdel1_1 >= tdel1_0)
			tdel1_0 = tdel1_1;
		if (tdel0_0 < tdel1_0)
			tdel0_0 = tdel1_0;
		if ((dst_rate_MHz_128 * tdel1_1 / 1000) > tree_margin || needs_wr_training)
			dst_emc_entry->current_dram_clktree_c1d0u1 = cval;

		if (dram_dev_num != TWO_RANK)
			goto out;

		EMC(EMC_MRR) = 0x40130000; // Dev1 MRR 19
		_wait_emc_status(EMC_EMC_STATUS, 0x100000, true, 0);
	}
	else
	{
		if (dram_dev_num != TWO_RANK)
			goto out;

		EMC(EMC_MRR) = 0x40130000; // Dev1 MRR 19
	}

	_wait_emc_status(EMC_EMC_STATUS, 0x100000, true, channel1_enabled);
	temp_ch0_0 = (EMC(EMC_MRR) & 0xFF) << 8;
	temp_ch0_1 = EMC(EMC_MRR) & 0xFF00;
	if (channel1_enabled)
	{
		temp_ch1_0 = (EMC_CH1(EMC_MRR) & 0xFF) << 8;
		temp_ch1_1 = EMC_CH1(EMC_MRR) & 0xFF00;
	}

	_request_mmr_data(0x40120000, channel1_enabled); // Dev1 MRR 18
	temp_ch0_0 |= EMC(EMC_MRR) & 0xFF;
	temp_ch0_1 |= ((EMC(EMC_MRR) & 0xFF00) >> 8);
	if (channel1_enabled)
	{
		temp_ch1_0 |= EMC_CH1(EMC_MRR) & 0xFF;
		temp_ch1_1 |= (EMC_CH1(EMC_MRR) & 0xFF00) >> 8;
	}
	cval = delay / temp_ch0_0;
	tdel0_0 = abs((s32)(dst_emc_entry->current_dram_clktree_c0d1u0 - cval));
	if ((dst_rate_MHz_128 * tdel0_0 / 1000) > tree_margin || needs_wr_training)
		dst_emc_entry->current_dram_clktree_c0d1u0 = cval;
	cval = delay / temp_ch0_1;
	tdel0_1 = abs((s32)(dst_emc_entry->current_dram_clktree_c0d1u1 - cval));
	if (tdel0_0 < tdel0_1)
		tdel0_0 = tdel0_1;
	if ((dst_rate_MHz_128 * tdel0_1 / 1000) > tree_margin || needs_wr_training)
		dst_emc_entry->current_dram_clktree_c0d1u1 = cval;

	if (!channel1_enabled)
		goto out;
	cval = delay / temp_ch1_0;
	tdel1_0 = abs((s32)(dst_emc_entry->current_dram_clktree_c1d1u0 - cval));
	if ((dst_rate_MHz_128 * tdel1_0 / 1000) > tree_margin || needs_wr_training)
		dst_emc_entry->current_dram_clktree_c1d1u0 = cval;
	cval = delay / temp_ch1_1;
	tdel1_1 = abs((s32)(dst_emc_entry->current_dram_clktree_c1d1u1 - cval));
	if (tdel1_1 >= tdel1_0)
		tdel1_0 = tdel1_1;
	if (tdel0_0 < tdel1_0)
		tdel0_0 = tdel1_0;

	if ((dst_rate_MHz_128 * tdel1_1 / 1000) > tree_margin || needs_wr_training)
	{
		dst_emc_entry->current_dram_clktree_c1d1u1 = cval;
out:
		if (needs_wr_training)
		{
			dst_emc_entry->trained_dram_clktree_c0d0u0 = dst_emc_entry->current_dram_clktree_c0d0u0;
			dst_emc_entry->trained_dram_clktree_c0d0u1 = dst_emc_entry->current_dram_clktree_c0d0u1;
			dst_emc_entry->trained_dram_clktree_c0d1u0 = dst_emc_entry->current_dram_clktree_c0d1u0;
			dst_emc_entry->trained_dram_clktree_c0d1u1 = dst_emc_entry->current_dram_clktree_c0d1u1;
			dst_emc_entry->trained_dram_clktree_c1d0u0 = dst_emc_entry->current_dram_clktree_c1d0u0;
			dst_emc_entry->trained_dram_clktree_c1d0u1 = dst_emc_entry->current_dram_clktree_c1d0u1;
			dst_emc_entry->trained_dram_clktree_c1d1u0 = dst_emc_entry->current_dram_clktree_c1d1u0;
			dst_emc_entry->trained_dram_clktree_c1d1u1 = dst_emc_entry->current_dram_clktree_c1d1u1;
		}
	}

	return (u32)tdel0_0;
}

u32 _digital_dll_prelock_old(emc_table_t *mtc_table_entry, u32 needs_tristate_training, u32 selected_clk_src_emc)
{
	s32 ddllcal_ctrl_start_trim = 0;

	s32 dual_channel = (EMC(EMC_FBIO_CFG7) >> 1) & ((EMC(EMC_FBIO_CFG7) >> 2) & 1);

	EMC(EMC_CFG_DIG_DLL) = (EMC(EMC_CFG_DIG_DLL) & 0xFFFFF8EC) | 0x3C8;

	_timing_update(dual_channel);

	while (EMC(EMC_CFG_DIG_DLL) & 1)
		;
	if (dual_channel)
		while (EMC_CH1(EMC_CFG_DIG_DLL) & 1)
			;

	EMC(EMC_DLL_CFG_0) = (EMC(EMC_DLL_CFG_0) & 0xDF00000F) | 0x1FA340AF;

	ddllcal_ctrl_start_trim = 150;
	if (mtc_table_entry->rate_khz >= 600000 && mtc_table_entry->rate_khz < 800000)
		ddllcal_ctrl_start_trim = 100;
	else if (mtc_table_entry->rate_khz >= 800000 && mtc_table_entry->rate_khz < 1000000)
		ddllcal_ctrl_start_trim = 70;
	else if (mtc_table_entry->rate_khz >= 1000000 && mtc_table_entry->rate_khz < 1200000)
		ddllcal_ctrl_start_trim = 30;
	else if (mtc_table_entry->rate_khz >= 1200000)
		ddllcal_ctrl_start_trim = 20;

	EMC(EMC_DLL_CFG_1) = (EMC(EMC_DLL_CFG_1) & 0xFFFFF800) | ddllcal_ctrl_start_trim;

	_change_dll_src(mtc_table_entry, selected_clk_src_emc);

	EMC(EMC_CFG_DIG_DLL) |= 1;
	
	_timing_update(dual_channel);

	while (!(EMC(EMC_CFG_DIG_DLL) & 1))
		;
	if (dual_channel)
		while (!(EMC_CH1(EMC_CFG_DIG_DLL) & 1))
			;

	while ((((EMC(EMC_DIG_DLL_STATUS) >> 17) & 1) ^ 1) | (((EMC(EMC_DIG_DLL_STATUS) >> 15) & 1) ^ 1))
		;

	if (needs_tristate_training)
	{
		EMC(EMC_DBG) |= 2u;
		EMC(EMC_CFG_DIG_DLL) &= 0xFFFFFFFE; //Disable CFG_DLL_EN: [PMC] Enable digital DLL.
		EMC(EMC_DBG) &= 0xFFFFFFFD;
		while (EMC(EMC_CFG_DIG_DLL) & 1)
			;
		if (dual_channel)
		{
			while (EMC_CH1(EMC_CFG_DIG_DLL) & 1)
				;
		}
	}

	return EMC(EMC_DIG_DLL_STATUS) & 0x7FF;
}


*/

u32 _minerva_update_clock_tree_delay(emc_table_t *src_emc_entry, emc_table_t *dst_emc_entry, s32 dram_dev_num, s32 channel1_enabled, enum tree_update_mode_t update_type)
{
	s32 temp_ch0_0 = 0;
	s32 temp_ch0_1 = 0;
	s32 temp_ch1_0 = 0;
	s32 temp_ch1_1 = 0;

	s32 dst_rate_mhz;

	u32 cval = 0;
	s32 tdel0_0 = 0;
	s32 tdel0_1 = 0;
	s32 tdel1_0 = 0;
	s32 tdel1_1 = 0;
	s32 tmp_tdel0_0 = 0;

	temp_ch0_0 = 0x10624DD3; // div 1000 denominator
	temp_ch0_1 = dst_emc_entry->rate_khz;
	dst_rate_mhz = dst_emc_entry->rate_khz / 1000;
	u32 upd_type_bits = 1 << update_type;

	u32 tval = 1000 * (1000 * _actual_osc_clocks(src_emc_entry->run_clocks) / (src_emc_entry->rate_khz / 1000));
	if (update_type <= PERIODIC_TRAINING_UPDATE)
	{
		temp_ch0_1 = 1 << update_type;
		temp_ch0_0 = 0x5400;
		if (upd_type_bits & 0x5400)
		{
			_request_mmr_data(0x80130000, channel1_enabled); // Dev0 MRR 19.
			temp_ch0_0 = (EMC(EMC_MRR) & 0xFF) << 8;
			temp_ch0_1 = EMC(EMC_MRR) & 0xFF00;
			if (channel1_enabled)
			{
				temp_ch1_0 = (EMC_CH1(EMC_MRR) & 0xFF) << 8;
				temp_ch1_1 = EMC_CH1(EMC_MRR) & 0xFF00;
			}

			_request_mmr_data(0x80120000, channel1_enabled); // Dev0 MRR 18.
			temp_ch0_0 |= EMC(EMC_MRR) & 0xFF;
			temp_ch0_1 |= (EMC(EMC_MRR) & 0xFF00) >> 8;
			if (channel1_enabled)
			{
				temp_ch1_0 |= EMC_CH1(EMC_MRR) & 0xFF;
				temp_ch1_1 |= (EMC_CH1(EMC_MRR) & 0xFF00) >> 8;
			}
		}
	}

	//u32 delay = (u64)((u64)_actual_osc_clocks(src_emc_entry->run_clocks) * 1000000) / (u64)(src_emc_entry->rate_khz * 2);
	cval = tval / (2 * temp_ch0_0);
	switch (update_type)
	{
	case DVFS_PT1:
	case TRAINING_PT1:
		dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d0u0_idx += 100 * cval;
		tdel0_0 = 0;
		if (update_type > PERIODIC_TRAINING_UPDATE || !(upd_type_bits & 0x6800))
			goto calc_td0_0;
		break;
	case DVFS_UPDATE:
		dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d0u0_idx =
			dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d0u0_idx / dst_emc_entry->ptfv_list.ptfv_dvfs_samples_idx;
		break;
	case TRAINING_UPDATE:
		dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d0u0_idx =
			dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d0u0_idx / dst_emc_entry->ptfv_list.ptfv_write_samples_idx;
		break;
	case PERIODIC_TRAINING_UPDATE:
		dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d0u0_idx =
			(100 * cval + dst_emc_entry->ptfv_list.ptfv_movavg_weight_idx * dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d0u0_idx)
			/ (dst_emc_entry->ptfv_list.ptfv_movavg_weight_idx + 1);
		break;
	default:
		tdel0_0 = 0;
		if (update_type > PERIODIC_TRAINING_UPDATE || !(upd_type_bits & 0x6800))
			goto calc_td0_0;
		break;
	}
	
	tdel0_0 = dst_emc_entry->current_dram_clktree_c0d0u0 - (dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d0u0_idx / 100);
	if (tdel0_0 < 0)
		tdel0_0 = !tdel0_0;
	if (update_type == TRAINING_UPDATE || (dst_rate_mhz * tdel0_0 << 7) / 1000000 > dst_emc_entry->tree_margin)
		dst_emc_entry->current_dram_clktree_c0d0u0 = dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d0u0_idx / 100;

calc_td0_0:
	cval = tval / (2 * temp_ch0_1);
	switch (update_type)
	{
	case DVFS_PT1:
	case TRAINING_PT1:
		dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d0u1_idx += 100 * cval;
		if (update_type > PERIODIC_TRAINING_UPDATE || !(upd_type_bits & 0x6800))
			goto calc_td1_0;
		break;
	case DVFS_UPDATE:
		dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d0u1_idx =
			dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d0u1_idx / dst_emc_entry->ptfv_list.ptfv_dvfs_samples_idx;
		break;
	case TRAINING_UPDATE:
		dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d0u1_idx =
			dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d0u1_idx / dst_emc_entry->ptfv_list.ptfv_write_samples_idx;
		break;
	case PERIODIC_TRAINING_UPDATE:
		dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d0u1_idx =
			(100 * cval + dst_emc_entry->ptfv_list.ptfv_movavg_weight_idx * dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d0u1_idx)
			/ (dst_emc_entry->ptfv_list.ptfv_movavg_weight_idx + 1);
		break;
	default:
		if (update_type > PERIODIC_TRAINING_UPDATE || !(upd_type_bits & 0x6800))
			goto calc_td1_0;
		break;
	}

	tdel0_1 = dst_emc_entry->current_dram_clktree_c0d0u1 - (dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d0u1_idx / 100);
	if (tdel0_1 < 0)
		tdel0_1 = !tdel0_1;
	if (tdel0_1 > tdel0_0)
		tdel0_0 = tdel0_1;
	if (update_type == TRAINING_UPDATE || (dst_rate_mhz * tdel0_1 << 7) / 1000000 > dst_emc_entry->tree_margin)
		dst_emc_entry->current_dram_clktree_c0d0u1 = dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d0u1_idx / 100;

calc_td1_0:
	if (channel1_enabled == 1)
	{
		cval = tval / (2 * temp_ch1_0);
		switch (update_type)
		{
		case DVFS_PT1:
		case TRAINING_PT1:
			dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d0u0_idx += 100 * cval;
			if (update_type > PERIODIC_TRAINING_UPDATE || !(upd_type_bits & 0x6800))
				goto calc_td1_1;
			break;
		case DVFS_UPDATE:
			dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d0u0_idx =
				dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d0u0_idx / dst_emc_entry->ptfv_list.ptfv_dvfs_samples_idx;
			break;
		case TRAINING_UPDATE:
			dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d0u0_idx =
				dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d0u0_idx / dst_emc_entry->ptfv_list.ptfv_write_samples_idx;
			break;
		case PERIODIC_TRAINING_UPDATE:
			dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d0u0_idx =
				(100 * cval + dst_emc_entry->ptfv_list.ptfv_movavg_weight_idx * dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d0u0_idx)
				/ (dst_emc_entry->ptfv_list.ptfv_movavg_weight_idx + 1);
			break;
		default:
			if (update_type > PERIODIC_TRAINING_UPDATE || !(upd_type_bits & 0x6800))
				goto calc_td1_1;
			break;
		}

		tdel1_0 = dst_emc_entry->current_dram_clktree_c1d0u0 - (dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d0u0_idx / 100);
		if (tdel1_0 < 0)
			tdel1_0 = !tdel1_0;
		if (tdel1_0 > tdel0_0)
			tdel0_0 = tdel1_0;
		if (update_type == TRAINING_UPDATE || (dst_rate_mhz * tdel1_0 << 7) / 1000000 > dst_emc_entry->tree_margin)
			dst_emc_entry->current_dram_clktree_c1d0u0 = dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d0u0_idx / 100;

calc_td1_1:
		cval = tval / (2 * temp_ch1_1);
		switch (update_type)
		{
		case DVFS_PT1:
		case TRAINING_PT1:
			dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d0u1_idx += 100 * cval;
			if (update_type > PERIODIC_TRAINING_UPDATE || !(upd_type_bits & 0x6800))
				goto calc_dev2;
			break;
		case DVFS_UPDATE:
			dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d0u1_idx =
				dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d0u1_idx / dst_emc_entry->ptfv_list.ptfv_dvfs_samples_idx;
			break;
		case TRAINING_UPDATE:
			dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d0u1_idx =
				dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d0u1_idx / dst_emc_entry->ptfv_list.ptfv_write_samples_idx;
			break;
		case PERIODIC_TRAINING_UPDATE:
			dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d0u1_idx =
				(100 * cval + dst_emc_entry->ptfv_list.ptfv_movavg_weight_idx * dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d0u1_idx)
				/ (dst_emc_entry->ptfv_list.ptfv_movavg_weight_idx + 1);
			break;
		default:
			if (update_type > PERIODIC_TRAINING_UPDATE || !(upd_type_bits & 0x6800))
				goto calc_dev2;
			break;
		}

		tdel1_1 = dst_emc_entry->current_dram_clktree_c1d0u1 - (dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d0u1_idx / 100);
		if (tdel1_1 < 0)
			tdel1_1 = !tdel1_1;
		if (tdel1_1 > tdel0_0)
			tdel0_0 = tdel1_1;
		if (update_type == TRAINING_UPDATE || (dst_rate_mhz * tdel1_1 << 7) / 1000000 > dst_emc_entry->tree_margin)
			dst_emc_entry->current_dram_clktree_c1d0u1 = dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d0u1_idx / 100;
	}

calc_dev2:
	if (dram_dev_num != TWO_RANK)
		goto out;

	if (update_type <= PERIODIC_TRAINING_UPDATE && upd_type_bits & 0x5400)
	{
		_request_mmr_data(0x40130000, channel1_enabled); // Dev1 MRR 19.
		temp_ch0_0 = (EMC(EMC_MRR) & 0xFF) << 8;
		temp_ch0_1 = EMC(EMC_MRR) & 0xFF00;
		if (channel1_enabled == 1)
		{
			temp_ch1_0 = (EMC_CH1(EMC_MRR) & 0xFF) << 8;
			temp_ch1_1 = EMC_CH1(EMC_MRR) & 0xFF00;
		}

		_request_mmr_data(0x40120000, channel1_enabled); // Dev1 MRR 18
		temp_ch0_0 |= EMC(EMC_MRR) & 0xFF;
		temp_ch0_1 |= ((EMC(EMC_MRR) & 0xFF00) >> 8);
		if (channel1_enabled == 1)
		{
			temp_ch1_0 |= EMC_CH1(EMC_MRR) & 0xFF;
			temp_ch1_1 |= (EMC_CH1(EMC_MRR) & 0xFF00) >> 8;
		}
		
	}

	cval = tval / (2 * temp_ch0_0);
	switch (update_type )
	{
	case DVFS_PT1:
	case TRAINING_PT1:
		dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d1u0_idx += 100 * cval;
		if (update_type > PERIODIC_TRAINING_UPDATE || !(upd_type_bits & 0x6800))
			goto calc_tmp_td0_1;
		break;
	case DVFS_UPDATE:
		dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d1u0_idx =
			dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d1u0_idx / dst_emc_entry->ptfv_list.ptfv_dvfs_samples_idx;
		break;
	case TRAINING_UPDATE:
		dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d1u0_idx =
			dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d1u0_idx / dst_emc_entry->ptfv_list.ptfv_write_samples_idx;
		break;
	case PERIODIC_TRAINING_UPDATE:
		dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d1u0_idx =
			(100 * cval + dst_emc_entry->ptfv_list.ptfv_movavg_weight_idx * dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d1u0_idx)
			/ (dst_emc_entry->ptfv_list.ptfv_movavg_weight_idx + 1);
		break;
	default:
		if (update_type > PERIODIC_TRAINING_UPDATE || !(upd_type_bits & 0x6800))
			goto calc_tmp_td0_1;
		break;
	}

	tmp_tdel0_0 = dst_emc_entry->current_dram_clktree_c0d1u0 - (dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d1u0_idx / 100);
	if (tmp_tdel0_0 < 0)
		tmp_tdel0_0 = !tmp_tdel0_0;
	if (tmp_tdel0_0 > tdel0_0)
		tdel0_0 = tmp_tdel0_0;
	if (update_type == TRAINING_UPDATE || (dst_rate_mhz * tmp_tdel0_0 << 7) / 1000000 > dst_emc_entry->tree_margin)
		dst_emc_entry->current_dram_clktree_c0d1u0 = dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d1u0_idx / 100;

calc_tmp_td0_1:
	cval = tval / (2 * temp_ch0_1);
	switch (update_type)
	{
	case DVFS_PT1:
	case TRAINING_PT1:
		dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d1u1_idx += 100 * cval;
		if (update_type > PERIODIC_TRAINING_UPDATE || !(upd_type_bits & 0x6800))
			goto calc_tmp_td1_0;
		break;
	case DVFS_UPDATE:
		dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d1u1_idx =
			dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d1u1_idx / dst_emc_entry->ptfv_list.ptfv_dvfs_samples_idx;
		break;
	case TRAINING_UPDATE:
		dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d1u1_idx =
			dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d1u1_idx / dst_emc_entry->ptfv_list.ptfv_write_samples_idx;
		break;
	case PERIODIC_TRAINING_UPDATE:
		dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d1u1_idx =
			(100 * cval + dst_emc_entry->ptfv_list.ptfv_movavg_weight_idx * dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d1u1_idx)
			/ (dst_emc_entry->ptfv_list.ptfv_movavg_weight_idx + 1);
		break;
	default:
		if (update_type > PERIODIC_TRAINING_UPDATE || !(upd_type_bits & 0x6800))
			goto calc_tmp_td1_0;
		break;
	}

	tdel0_1 = dst_emc_entry->current_dram_clktree_c0d1u1 - (dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d1u1_idx / 100);
	if (tdel0_1 < 0)
		tdel0_1 = !tdel0_1;
	if (tdel0_1 > tdel0_0)
		tdel0_0 = tdel0_1;
	if (update_type == TRAINING_UPDATE || (dst_rate_mhz * tdel0_1 << 7) / 1000000 > dst_emc_entry->tree_margin)
		dst_emc_entry->current_dram_clktree_c0d1u1 = dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d1u1_idx / 100;

calc_tmp_td1_0:
	if (channel1_enabled == 1)
	{
		cval = tval / (2 * temp_ch1_0);
		switch (update_type)
		{
		case DVFS_PT1:
		case TRAINING_PT1:
			dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d1u0_idx += 100 * cval;
			if (update_type > PERIODIC_TRAINING_UPDATE || !(upd_type_bits & 0x6800))
				goto calc_tmp_td1_1;
			break;
		case DVFS_UPDATE:
			dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d1u0_idx =
				dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d1u0_idx / dst_emc_entry->ptfv_list.ptfv_dvfs_samples_idx;
			break;
		case TRAINING_UPDATE:
			dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d1u0_idx =
				dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d1u0_idx / dst_emc_entry->ptfv_list.ptfv_write_samples_idx;
			break;
		case PERIODIC_TRAINING_UPDATE:
			dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d1u0_idx =
				(100 * cval + dst_emc_entry->ptfv_list.ptfv_movavg_weight_idx * dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d1u0_idx)
				/ (dst_emc_entry->ptfv_list.ptfv_movavg_weight_idx + 1);
			break;
		default:
			if (update_type > PERIODIC_TRAINING_UPDATE || !(upd_type_bits & 0x6800))
				goto calc_tmp_td1_1;
			break;
		}

		tdel1_0 = dst_emc_entry->current_dram_clktree_c1d1u0 - (dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d1u0_idx / 100);
		if (tdel1_0 < 0)
			tdel1_0 = !tdel1_0;
		if (tdel1_0 > tdel0_0)
			tdel0_0 = tdel1_0;
		if (update_type == TRAINING_UPDATE || (dst_rate_mhz * tdel1_0 << 7) / 1000000 > dst_emc_entry->tree_margin)
			dst_emc_entry->current_dram_clktree_c1d1u0 = dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d1u0_idx / 100;

calc_tmp_td1_1:
		cval = tval / (2 * temp_ch1_1);
		switch (update_type)
		{
		case DVFS_PT1:
		case TRAINING_PT1:
			dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d1u1_idx += 100 * cval;
			if (update_type > PERIODIC_TRAINING_UPDATE || !(upd_type_bits & 0x6800))
				goto out;
			break;
		case DVFS_UPDATE:
			dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d1u1_idx =
				dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d1u1_idx / dst_emc_entry->ptfv_list.ptfv_dvfs_samples_idx;
			break;
		case TRAINING_UPDATE:
			dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d1u1_idx =
				dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d1u1_idx / dst_emc_entry->ptfv_list.ptfv_write_samples_idx;
			break;
		case PERIODIC_TRAINING_UPDATE:
			dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d1u1_idx =
				(100 * cval + dst_emc_entry->ptfv_list.ptfv_movavg_weight_idx * dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d1u1_idx)
				/ (dst_emc_entry->ptfv_list.ptfv_movavg_weight_idx + 1);
			break;
		default:
			if (update_type > PERIODIC_TRAINING_UPDATE || !(upd_type_bits & 0x6800))
				goto out;
			break;
		}

		tdel1_1 = dst_emc_entry->current_dram_clktree_c1d1u1 - (dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d1u1_idx / 100);
		if (tdel1_1 < 0)
			tdel1_1 = !tdel1_1;
		if (tdel1_1 > tdel0_0)
			tdel0_0 = tdel1_1;
		if (update_type == TRAINING_UPDATE || (dst_rate_mhz * tdel1_1 << 7) / 1000000 > dst_emc_entry->tree_margin)
			dst_emc_entry->current_dram_clktree_c1d1u1 = dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d1u1_idx / 100;
	}

out:
	if (update_type == TRAINING_UPDATE)
	{
		dst_emc_entry->trained_dram_clktree_c0d0u0 = dst_emc_entry->current_dram_clktree_c0d0u0;
		dst_emc_entry->trained_dram_clktree_c0d0u1 = dst_emc_entry->current_dram_clktree_c0d0u1;
		dst_emc_entry->trained_dram_clktree_c0d1u0 = dst_emc_entry->current_dram_clktree_c0d1u0;
		dst_emc_entry->trained_dram_clktree_c0d1u1 = dst_emc_entry->current_dram_clktree_c0d1u1;
		dst_emc_entry->trained_dram_clktree_c1d0u0 = dst_emc_entry->current_dram_clktree_c1d0u0;
		dst_emc_entry->trained_dram_clktree_c1d0u1 = dst_emc_entry->current_dram_clktree_c1d0u1;
		dst_emc_entry->trained_dram_clktree_c1d1u0 = dst_emc_entry->current_dram_clktree_c1d1u0;
		dst_emc_entry->trained_dram_clktree_c1d1u1 = dst_emc_entry->current_dram_clktree_c1d1u1;
	}

	return (u32)tdel0_0;
}

u32 _minerva_periodic_compensation_handler(emc_table_t *src_emc_entry, emc_table_t *dst_emc_entry, s32 dram_dev_num, s32 channel1_enabled, enum comp_seq_t seq_type)
{
	if (!dst_emc_entry->periodic_training)
		return seq_type;

	u32 adel = 0;
	u32 delay = 1000 * _actual_osc_clocks(src_emc_entry->run_clocks) / src_emc_entry->rate_khz + 2;

	if (seq_type == DVFS_SEQUENCE)
	{
		if (src_emc_entry->periodic_training && dst_emc_entry->ptfv_list.ptfv_config_ctrl_idx & 1)
		{
			u32 samples = dst_emc_entry->ptfv_list.ptfv_dvfs_samples_idx;
			dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d0u0_idx = src_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d0u0_idx * samples;
			dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d0u1_idx = src_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d0u1_idx * samples;
			dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d0u0_idx = src_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d0u0_idx * samples;
			dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d0u1_idx = src_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d0u1_idx * samples;
			dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d1u0_idx = src_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d1u0_idx * samples;
			dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d1u1_idx = src_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d1u1_idx * samples;
			dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d1u0_idx = src_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d1u0_idx * samples;
			dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d1u1_idx = src_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d1u1_idx * samples;
		}
		else
		{
			dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d0u0_idx = 0;
			dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d0u1_idx = 0;
			dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d0u0_idx = 0;
			dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d0u1_idx = 0;
			dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d1u0_idx = 0;
			dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d1u1_idx = 0;
			dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d1u0_idx = 0;
			dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d1u1_idx = 0;

			for (s32 i = 0; i < dst_emc_entry->ptfv_list.ptfv_dvfs_samples_idx; i++)
			{
				_start_periodic_compensation();
				_usleep(delay);
				_minerva_update_clock_tree_delay(src_emc_entry, dst_emc_entry, dram_dev_num, channel1_enabled, DVFS_PT1);
			}
		}
		adel = _minerva_update_clock_tree_delay(src_emc_entry, dst_emc_entry, dram_dev_num, channel1_enabled, DVFS_UPDATE);

		return adel;
	}
	else if (seq_type == WRITE_TRAINING_SEQUENCE)
	{
		dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d0u0_idx = 0;
		dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d0u1_idx = 0;
		dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d0u0_idx = 0;
		dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d0u1_idx = 0;
		dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d1u0_idx = 0;
		dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c0d1u1_idx = 0;
		dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d1u0_idx = 0;
		dst_emc_entry->ptfv_list.ptfv_dqsosc_movavg_c1d1u1_idx = 0;

		for (s32 i = 0; i < dst_emc_entry->ptfv_list.ptfv_write_samples_idx; i++)
		{
			_start_periodic_compensation();
			_usleep(delay);
			_minerva_update_clock_tree_delay(src_emc_entry, dst_emc_entry, dram_dev_num, channel1_enabled, TRAINING_PT1);
		}
		adel = _minerva_update_clock_tree_delay(src_emc_entry, dst_emc_entry, dram_dev_num, channel1_enabled, TRAINING_UPDATE);

		return adel;
	}
	else if (seq_type == PERIODIC_TRAINING_SEQUENCE)
	{
		_start_periodic_compensation();
		_usleep(delay);
		adel = _minerva_update_clock_tree_delay(src_emc_entry, dst_emc_entry, dram_dev_num, channel1_enabled, PERIODIC_TRAINING_UPDATE);

		return adel;
	}

	return seq_type;
}

#define STORE_TRIM_VAL(chan, rank, reg, byte) \
		((mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_long_dq_rank##rank##_##reg##_idx >>	\
	 		EMC_PMACRO_OB_DDLL_LONG_DQ_BYTE##byte##_SHIFT) & 0x7FF) \
		+ \
		(((mtc_table_entry->trim_perch_regs.emc##chan##_data_brlshft_##rank##_idx >>	\
			EMC_DATA_BRLSHFT_##rank##_RANK##rank##_BYTE##byte##_DATA_BRLSHFT_SHIFT) & 0x7) << 6)

u32 _minerva_apply_periodic_compensation_trimmer(emc_table_t *mtc_table_entry, u32 trim_emc_reg_addr)
{
	u32 trimmer = 0;
	s32 tree_delta[4] = {0};
	s32 tree_delta_taps[4] = {0};
	s32 new_trim[] = {
		// chan, rank, reg, byte.
		STORE_TRIM_VAL(0, 0, 0, 0),
		STORE_TRIM_VAL(0, 0, 0, 1),
		STORE_TRIM_VAL(0, 0, 1, 2),
		STORE_TRIM_VAL(0, 0, 1, 3),

		STORE_TRIM_VAL(1, 0, 2, 4),
		STORE_TRIM_VAL(1, 0, 2, 5),
		STORE_TRIM_VAL(1, 0, 3, 6),
		STORE_TRIM_VAL(1, 0, 3, 7),

		STORE_TRIM_VAL(0, 1, 0, 0),
		STORE_TRIM_VAL(0, 1, 0, 1),
		STORE_TRIM_VAL(0, 1, 1, 2),
		STORE_TRIM_VAL(0, 1, 1, 3),

		STORE_TRIM_VAL(1, 1, 2, 4),
		STORE_TRIM_VAL(1, 1, 2, 5),
		STORE_TRIM_VAL(1, 1, 3, 6),
		STORE_TRIM_VAL(1, 1, 3, 7)
	};

	u32 dst_rate_mhz = mtc_table_entry->rate_khz / 1000;

	switch (trim_emc_reg_addr) 
	{
	case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_0:
	case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_1:
	case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_2:
	case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_3:
	case EMC_DATA_BRLSHFT_0:
		tree_delta[0] = (mtc_table_entry->current_dram_clktree_c0d0u0 - mtc_table_entry->trained_dram_clktree_c0d0u0) << 7;
		tree_delta[1] = (mtc_table_entry->current_dram_clktree_c0d0u1 - mtc_table_entry->trained_dram_clktree_c0d0u1) << 7;
		tree_delta[2] = (mtc_table_entry->current_dram_clktree_c1d0u0 - mtc_table_entry->trained_dram_clktree_c1d0u0) << 7;
		tree_delta[3] = (mtc_table_entry->current_dram_clktree_c1d0u1 - mtc_table_entry->trained_dram_clktree_c1d0u1) << 7;
		tree_delta_taps[0] = (tree_delta[0] * (s32)dst_rate_mhz) / 1000000;
		tree_delta_taps[1] = (tree_delta[1] * (s32)dst_rate_mhz) / 1000000;
		tree_delta_taps[2] = (tree_delta[2] * (s32)dst_rate_mhz) / 1000000;
		tree_delta_taps[3] = (tree_delta[3] * (s32)dst_rate_mhz) / 1000000;
		for (s32 i = 0; i < 4; i++)
		{
			if ((tree_delta_taps[i] > mtc_table_entry->tree_margin) || (tree_delta_taps[i] < (-1 * mtc_table_entry->tree_margin)))
			{
				new_trim[i * 2] += tree_delta_taps[i];
				new_trim[i * 2 + 1] += tree_delta_taps[i];
			}
		}
		if (trim_emc_reg_addr == EMC_DATA_BRLSHFT_0)
		{
			for (s32 i = 0; i < 8; i++)
				new_trim[i] /= 64;
		}
		else
		{
			for (s32 i = 0; i < 8; i++)
				new_trim[i] %= 64;
		}
		break;
	case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_0:
	case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_1:
	case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_2:
	case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_3:
	case EMC_DATA_BRLSHFT_1:
		tree_delta[0] = (mtc_table_entry->current_dram_clktree_c0d1u0 - mtc_table_entry->trained_dram_clktree_c0d1u0) << 7;
		tree_delta[1] = (mtc_table_entry->current_dram_clktree_c0d1u1 - mtc_table_entry->trained_dram_clktree_c0d1u1) << 7;
		tree_delta[2] = (mtc_table_entry->current_dram_clktree_c1d1u0 - mtc_table_entry->trained_dram_clktree_c1d1u0) << 7;
		tree_delta[3] = (mtc_table_entry->current_dram_clktree_c1d1u1 - mtc_table_entry->trained_dram_clktree_c1d1u1) << 7;
		tree_delta_taps[0] = (tree_delta[0] * (s32)dst_rate_mhz) / 1000000;
		tree_delta_taps[1] = (tree_delta[1] * (s32)dst_rate_mhz) / 1000000;
		tree_delta_taps[2] = (tree_delta[2] * (s32)dst_rate_mhz) / 1000000;
		tree_delta_taps[3] = (tree_delta[3] * (s32)dst_rate_mhz) / 1000000;
		for (s32 i = 0; i < 4; i++)
		{
			if ((tree_delta_taps[i] > mtc_table_entry->tree_margin) || (tree_delta_taps[i] < (-1 * mtc_table_entry->tree_margin))) {
				new_trim[8 + i * 2] += tree_delta_taps[i];
				new_trim[8 + i * 2 + 1] += tree_delta_taps[i];
			}
		}
		if (trim_emc_reg_addr == EMC_DATA_BRLSHFT_1)
		{
			for (s32 i = 0; i < 8; i++)
				new_trim[i + 8] /= 64;
		}
		else
		{
			for (s32 i = 0; i < 8; i++)
				new_trim[i + 8] %= 64;
		}
		break;
	}

	switch (trim_emc_reg_addr)
	{
	case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_0:
		trimmer = (new_trim[0] & 0x7FF) | ((new_trim[1] & 0x7FF) << 16);
		break;
	case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_1:
		trimmer = (new_trim[2] & 0x7FF) | ((new_trim[3] & 0x7FF) << 16);
		break;
	case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_2:
		trimmer = (new_trim[4] & 0x7FF) | ((new_trim[5] & 0x7FF) << 16);
		break;
	case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_3:
		trimmer = (new_trim[6] & 0x7FF) | ((new_trim[7] & 0x7FF) << 16);
		break;
	case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_0:
		trimmer = (new_trim[8] & 0x7FF) | ((new_trim[9] & 0x7FF) << 16);
		break;
	case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_1:
		trimmer = (new_trim[10] & 0x7FF) | ((new_trim[11] & 0x7FF) << 16);
		break;
	case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_2:
		trimmer = (new_trim[12] & 0x7FF) | ((new_trim[13] & 0x7FF) << 16);
		break;
	case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_3:
		trimmer = (new_trim[14] & 0x7FF) | ((new_trim[15] & 0x7FF) << 16);
		break;
	case EMC_DATA_BRLSHFT_0:
		trimmer = (new_trim[0] & 7)
				| ((new_trim[1] & 7) << 3)
				| ((new_trim[2] & 7) << 6)
				| ((new_trim[3] & 7) << 9)
				| ((new_trim[4] & 7) << 12)
				| ((new_trim[5] & 7) << 15)
				| ((new_trim[6] & 7) << 18)
				| ((new_trim[7] & 7) << 21);
		break;
	case EMC_DATA_BRLSHFT_1:
		trimmer = (new_trim[8] & 7)
				| ((new_trim[9] & 7) << 3)
				| ((new_trim[10] & 7) << 6)
				| ((new_trim[11] & 7) << 9)
				| ((new_trim[12] & 7) << 12)
				| ((new_trim[13] & 7) << 15)
				| ((new_trim[14] & 7) << 18)
				| ((new_trim[15] & 7) << 21);
		break;
	default:
		break;
	}

	return trimmer;
}

bool _check_freq_changed(u32 dst_entry_rate_KHz, u32 dst_entry_clk_src_emc, u32 src_entry_rate_KHz, u32 src_entry_clk_src_emc)
{
	float dst_div_clock;
	float src_div_clock;
	float src_end_div_clk_ratio;

	u32 src_entry_emc_2X_clk_src = src_entry_clk_src_emc >> EMC_2X_CLK_SRC_SHIFT;
	u32 dst_entry_emc_2X_clk_src = dst_entry_clk_src_emc >> EMC_2X_CLK_SRC_SHIFT;
	u32 src_entry_emc_2X_clk_src_div = src_entry_clk_src_emc & 0xFF;
	u32 dst_entry_emc_2X_clk_src_div = dst_entry_clk_src_emc & 0xFF;
	u32 pll_post_divider = 0;
	switch (CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_EMC) >> EMC_2X_CLK_SRC_SHIFT)
	{
		case PLLM_OUT0:
		case PLLM_UD:
			pll_post_divider = (CLOCK(CLK_RST_CONTROLLER_PLLM_BASE) >> 20) & 0x1F;
			break;
		case PLLMB_UD:
		case PLLMB_OUT0:
			pll_post_divider = (CLOCK(CLK_RST_CONTROLLER_PLLMB_BASE) >> 20) & 0x1F;
			break;
		default:
			break;
	}
	if (pll_post_divider > 5)
	{
		while (true)
			;
	}

	if (src_entry_emc_2X_clk_src <= PLLMB_UD)
		src_entry_emc_2X_clk_src_div = 0;
	if (dst_entry_emc_2X_clk_src <= PLLMB_UD)
		dst_entry_emc_2X_clk_src_div = 0;

	if (dst_entry_emc_2X_clk_src != src_entry_emc_2X_clk_src && (dst_entry_emc_2X_clk_src & 0xFFFFFFFB || src_entry_emc_2X_clk_src & 0xFFFFFFFB))
		return true;

	dst_div_clock = (double)dst_entry_rate_KHz
		* ((double)((dst_entry_emc_2X_clk_src_div >> 1) + 1)
		+ (double)(dst_entry_emc_2X_clk_src_div & 1) * 0.5)
		* (double)(pll_post_divider + 1);
	src_div_clock = (double)src_entry_rate_KHz
		* ((double)((src_entry_emc_2X_clk_src_div >> 1) + 1)
		+ (double)(src_entry_emc_2X_clk_src_div & 1) * 0.5)
		* (double)(pll_post_divider + 1);

	src_end_div_clk_ratio = src_div_clock / dst_div_clock;

	if (src_end_div_clk_ratio > 1.01f || src_end_div_clk_ratio < 0.99f)
		return true;
	else
		return false;
}

void _save_train_results(emc_table_t *mtc_table_entry, u32 needs_training, s32 dram_dev_num, bool channel1_enabled)
{
	bool needs_ca_training = needs_training & 1;
	bool needs_ca_vref_training = (needs_training >> 1) & 1;
	bool needs_quse_training = (needs_training >> 2) & 1;
	bool needs_quse_vref_training = (needs_training >> 3) & 1;
	bool needs_wr_training = (needs_training >> 4) & 1;
	bool needs_wr_vref_training = (needs_training >> 5) & 1;
	bool needs_rd_training = (needs_training >> 6) & 1;
	bool needs_rd_vref_training = (needs_training >> 7) & 1;
	bool needs_training_in_self_refresh = (needs_training >> 9) & 1;

	if (needs_ca_training)
	{
		mtc_table_entry->trim_perch_regs.emc_cmd_brlshft_0_idx = EMC_CH0(EMC_CMD_BRLSHFT_0);
		mtc_table_entry->trim_perch_regs.emc_cmd_brlshft_1_idx = channel1_enabled ? EMC_CH1(EMC_CMD_BRLSHFT_1) : 0;
		mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_4_idx = EMC_CH0(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_4);
		mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_5_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_5) : 0;

		if (needs_training_in_self_refresh)
		{
			mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd0_0_idx = EMC(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD0_0);
			mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd0_1_idx = EMC(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD0_1);
			mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd0_2_idx = EMC(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD0_2);
			mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd1_0_idx = EMC(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD1_0);
			mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd1_1_idx = EMC(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD1_1);
			mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd1_2_idx = EMC(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD1_2);
			mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd2_0_idx = EMC(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD2_0);
			mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd2_1_idx = EMC(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD2_1);
			mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd2_2_idx = EMC(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD2_2);
			mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd3_0_idx = EMC(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD3_0);
			mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd3_1_idx = EMC(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD3_1);
			mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd3_2_idx = EMC(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD3_2);
		}
	}

	if (needs_ca_vref_training)
	{
		mtc_table_entry->burst_reg_per_ch.emc0_mrw10_idx = (EMC_CH0(EMC_TRAINING_OPT_CA_VREF) & 0xFFFF) | 0x880C0000;
		mtc_table_entry->burst_reg_per_ch.emc1_mrw10_idx = (channel1_enabled ? EMC_CH1(EMC_TRAINING_OPT_CA_VREF) & 0xFFFF : 0) | 0x880C0000;

		u32 mrw11_dev_selectn = 0;
		if (dram_dev_num == TWO_RANK)
			mrw11_dev_selectn = 0x480C0000;
		else
			mrw11_dev_selectn = 0xC80C0000;

		mtc_table_entry->burst_reg_per_ch.emc0_mrw11_idx = 
			((EMC_CH0(EMC_TRAINING_OPT_CA_VREF) >> 16) & 0xFF)
			| (EMC_CH0(EMC_TRAINING_OPT_CA_VREF) >> 24 << 8)
			| (mrw11_dev_selectn & 0xFFFFFF00);

		mtc_table_entry->burst_reg_per_ch.emc1_mrw11_idx =
			(((channel1_enabled ? EMC_CH1(EMC_TRAINING_OPT_CA_VREF) : 0) >> 16) & 0xFF)
			| ((channel1_enabled ? EMC_CH1(EMC_TRAINING_OPT_CA_VREF) : 0) >> 24 << 8)
			| (mrw11_dev_selectn & 0xFFFFFF00);
	}

	if (needs_quse_training || needs_rd_training)
	{
		mtc_table_entry->trim_perch_regs.emc_quse_brlshft_0_idx = EMC_CH0(EMC_QUSE_BRLSHFT_0);
		mtc_table_entry->trim_perch_regs.emc_quse_brlshft_1_idx = channel1_enabled ? EMC_CH1(EMC_QUSE_BRLSHFT_1) : 0;

		mtc_table_entry->trim_regs.emc_pmacro_quse_ddll_rank0_0_idx = EMC_CH0(EMC_PMACRO_QUSE_DDLL_RANK0_0);
		mtc_table_entry->trim_regs.emc_pmacro_quse_ddll_rank0_1_idx = EMC_CH0(EMC_PMACRO_QUSE_DDLL_RANK0_1);
		mtc_table_entry->trim_regs.emc_pmacro_quse_ddll_rank0_2_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_QUSE_DDLL_RANK0_2) : 0;
		mtc_table_entry->trim_regs.emc_pmacro_quse_ddll_rank0_3_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_QUSE_DDLL_RANK0_3) : 0;

		if (dram_dev_num == TWO_RANK)
		{
			mtc_table_entry->trim_perch_regs.emc_quse_brlshft_2_idx = EMC_CH0(EMC_QUSE_BRLSHFT_2);
			mtc_table_entry->trim_perch_regs.emc_quse_brlshft_3_idx = channel1_enabled ? EMC_CH1(EMC_QUSE_BRLSHFT_3) : 0;

			mtc_table_entry->trim_regs.emc_pmacro_quse_ddll_rank1_0_idx = EMC_CH0(EMC_PMACRO_QUSE_DDLL_RANK1_0);
			mtc_table_entry->trim_regs.emc_pmacro_quse_ddll_rank1_1_idx = EMC_CH0(EMC_PMACRO_QUSE_DDLL_RANK1_1);
			mtc_table_entry->trim_regs.emc_pmacro_quse_ddll_rank1_2_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_QUSE_DDLL_RANK1_2) : 0;
			mtc_table_entry->trim_regs.emc_pmacro_quse_ddll_rank1_3_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_QUSE_DDLL_RANK1_3) : 0;
		}
	}

	if (needs_quse_vref_training)
	{
		if (dram_dev_num == TWO_RANK)
		{
			u32 emc0_opt_dqs_array[4] = {0};
			u32 emc1_opt_dqs_array[4] = {0};
			u32 emc1_training_opt_dqs_ib_vref_rank0_val = channel1_enabled ? EMC_CH1(EMC_TRAINING_OPT_DQS_IB_VREF_RANK0) : 0;
			u32 emc1_training_opt_dqs_ib_vref_rank1_val = channel1_enabled ? EMC_CH1(EMC_TRAINING_OPT_DQS_IB_VREF_RANK1) : 0;

			for (u32 i = 0; i < 4; i++)
			{
				emc0_opt_dqs_array[i] = (EMC_CH0(EMC_TRAINING_OPT_DQS_IB_VREF_RANK0) >> (8 * i)) & 0xFF;
				emc1_opt_dqs_array[i] = (emc1_training_opt_dqs_ib_vref_rank0_val >> (8 * i)) & 0xFF;
			}

			u32 ib_vref_dqs_0 = 0;
			u32 ib_vref_dqs_1 = 0;
			for (u32 i = 0; i < 4; i++)
			{
				ib_vref_dqs_0 |= (emc0_opt_dqs_array[i] + ((EMC_CH0(EMC_TRAINING_OPT_DQS_IB_VREF_RANK1) >> (8 * i)) & 0xFF)) >> 1 << (8 * i);
				ib_vref_dqs_1 |= (emc1_opt_dqs_array[i] + ((emc1_training_opt_dqs_ib_vref_rank1_val >> (8 * i)) & 0xFF)) >> 1 << (8 * i);
			}

			mtc_table_entry->trim_regs.emc_pmacro_ib_vref_dqs_0_idx = ib_vref_dqs_0;
			mtc_table_entry->trim_regs.emc_pmacro_ib_vref_dqs_1_idx = ib_vref_dqs_1;
		}
		else
		{
			mtc_table_entry->trim_regs.emc_pmacro_ib_vref_dqs_0_idx = EMC(EMC_PMACRO_IB_VREF_DQS_0);
			mtc_table_entry->trim_regs.emc_pmacro_ib_vref_dqs_1_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_IB_VREF_DQS_1) : 0;
		}
	}

	if (needs_rd_training)
	{
		mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank0_0_idx = EMC_CH0(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_0);
		mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank0_1_idx = EMC_CH0(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_1);
		mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank0_2_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_2) : 0;
		mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank0_3_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_3) : 0;

		if (dram_dev_num == TWO_RANK)
		{
			mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank1_0_idx = EMC_CH0(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_0);
			mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank1_1_idx = EMC_CH0(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_1);
			mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank1_2_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_2) : 0;
			mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank1_3_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_3) : 0;
		}

		if (needs_training_in_self_refresh)
		{
			mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte0_0_idx = EMC_CH0(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE0_0);
			mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte0_1_idx = EMC_CH0(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE0_1);
			mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte0_2_idx = EMC_CH0(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE0_2);
			mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte1_0_idx = EMC_CH0(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE1_0);
			mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte1_1_idx = EMC_CH0(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE1_1);
			mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte1_2_idx = EMC_CH0(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE1_2);
			mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte2_0_idx = EMC_CH0(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE2_0);
			mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte2_1_idx = EMC_CH0(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE2_1);
			mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte2_2_idx = EMC_CH0(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE2_2);
			mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte3_0_idx = EMC_CH0(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE3_0);
			mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte3_1_idx = EMC_CH0(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE3_1);
			mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte3_2_idx = EMC_CH0(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE3_2);
			mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte4_0_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE4_0) : 0;
			mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte4_1_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE4_1) : 0;
			mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte4_2_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE4_2) : 0;
			mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte5_0_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE5_0) : 0;
			mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte5_1_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE5_1) : 0;
			mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte5_2_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE5_2) : 0;
			mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte6_0_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE6_0) : 0;
			mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte6_1_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE6_1) : 0;
			mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte6_2_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE6_2) : 0;
			mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte7_0_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE7_0) : 0;
			mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte7_1_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE7_1) : 0;
			mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte7_2_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE7_2) : 0;

			if (dram_dev_num == TWO_RANK)
			{
				mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte0_0_idx = EMC_CH0(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE0_0);
				mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte0_1_idx = EMC_CH0(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE0_1);
				mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte0_2_idx = EMC_CH0(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE0_2);
				mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte1_0_idx = EMC_CH0(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE1_0);
				mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte1_1_idx = EMC_CH0(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE1_1);
				mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte1_2_idx = EMC_CH0(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE1_2);
				mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte2_0_idx = EMC_CH0(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE2_0);
				mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte2_1_idx = EMC_CH0(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE2_1);
				mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte2_2_idx = EMC_CH0(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE2_2);
				mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte3_0_idx = EMC_CH0(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE3_0);
				mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte3_1_idx = EMC_CH0(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE3_1);
				mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte3_2_idx = EMC_CH0(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE3_2);
				mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte4_0_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE4_0) : 0;
				mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte4_1_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE4_1) : 0;
				mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte4_2_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE4_2) : 0;
				mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte5_0_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE5_0) : 0;
				mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte5_1_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE5_1) : 0;
				mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte5_2_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE5_2) : 0;
				mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte6_0_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE6_0) : 0;
				mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte6_1_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE6_1) : 0;
				mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte6_2_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE6_2) : 0;
				mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte7_0_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE7_0) : 0;
				mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte7_1_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE7_1) : 0;
				mtc_table_entry->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte7_2_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE7_2) : 0;
			}
		}

		if (needs_rd_vref_training)
		{
			char ib_vref_dq_byte0_icr = (EMC(EMC_PMACRO_IB_VREF_DQ_0) & 0x7F) + (mtc_table_entry->save_restore_mod_regs[0] & 0x7F);
			if (mtc_table_entry->save_restore_mod_regs[0] & 0x80000000) // < 0 check.
				ib_vref_dq_byte0_icr = (EMC(EMC_PMACRO_IB_VREF_DQ_0) & 0x7F) - (mtc_table_entry->save_restore_mod_regs[0] & 0x7F);

			char ib_vref_dq_byte1_icr = ((EMC(EMC_PMACRO_IB_VREF_DQ_0) >> 8) & 0x7F) + (mtc_table_entry->save_restore_mod_regs[1] & 0x7F);
			if (mtc_table_entry->save_restore_mod_regs[1] & 0x80000000)
				ib_vref_dq_byte1_icr = ((EMC(EMC_PMACRO_IB_VREF_DQ_0) >> 8) & 0x7F) - (mtc_table_entry->save_restore_mod_regs[1] & 0x7F);

			char ib_vref_dq_byte2_icr = ((EMC(EMC_PMACRO_IB_VREF_DQ_0) >> 16) & 0x7F) + (mtc_table_entry->save_restore_mod_regs[2] & 0x7F);
			if (mtc_table_entry->save_restore_mod_regs[2] & 0x80000000)
				ib_vref_dq_byte2_icr = ((EMC(EMC_PMACRO_IB_VREF_DQ_0) >> 16) & 0x7F) - (mtc_table_entry->save_restore_mod_regs[2] & 0x7F);

			char ib_vref_dq_byte3_icr = ((EMC(EMC_PMACRO_IB_VREF_DQ_0) >> 24) & 0x7F) + (mtc_table_entry->save_restore_mod_regs[3] & 0x7F);
			if (mtc_table_entry->save_restore_mod_regs[3] & 0x80000000)
				ib_vref_dq_byte3_icr = ((EMC(EMC_PMACRO_IB_VREF_DQ_0) >> 24) & 0x7F) - (mtc_table_entry->save_restore_mod_regs[3] & 0x7F);

			mtc_table_entry->trim_regs.emc_pmacro_ib_vref_dq_0_idx = 
				 ((ib_vref_dq_byte0_icr & 0x7F)
				| (ib_vref_dq_byte1_icr & 0x7F) << 8)
				| ((ib_vref_dq_byte2_icr & 0x7F) << 16)
				| ((ib_vref_dq_byte3_icr & 0x7F) << 24);
			
			char ib_vref_dq_byte4_icr = (EMC(EMC_PMACRO_IB_VREF_DQ_1) & 0x7F) + (mtc_table_entry->save_restore_mod_regs[4] & 0x7F);
			if (mtc_table_entry->save_restore_mod_regs[4] & 0x80000000)
				ib_vref_dq_byte4_icr = (EMC(EMC_PMACRO_IB_VREF_DQ_1) & 0x7F) - (mtc_table_entry->save_restore_mod_regs[4] & 0x7F);

			char ib_vref_dq_byte5_icr = ((EMC(EMC_PMACRO_IB_VREF_DQ_1) >> 8) & 0x7F) + (mtc_table_entry->save_restore_mod_regs[5] & 0x7F);
			if (mtc_table_entry->save_restore_mod_regs[5] & 0x80000000)
				ib_vref_dq_byte5_icr = ((EMC(EMC_PMACRO_IB_VREF_DQ_1) >> 8) & 0x7F) - (mtc_table_entry->save_restore_mod_regs[5] & 0x7F);

			char ib_vref_dq_byte6_icr = ((EMC(EMC_PMACRO_IB_VREF_DQ_1) >> 16) & 0x7F) + (mtc_table_entry->save_restore_mod_regs[6] & 0x7F);
			if (mtc_table_entry->save_restore_mod_regs[6] & 0x80000000)
				ib_vref_dq_byte6_icr = ((EMC(EMC_PMACRO_IB_VREF_DQ_1) >> 16) & 0x7F) - (mtc_table_entry->save_restore_mod_regs[6] & 0x7F);

			char ib_vref_dq_byte7_icr = ((EMC(EMC_PMACRO_IB_VREF_DQ_1) >> 24) & 0x7F) + (mtc_table_entry->save_restore_mod_regs[7] & 0x7F);
			if (mtc_table_entry->save_restore_mod_regs[7] & 0x80000000)
				ib_vref_dq_byte7_icr = ((EMC(EMC_PMACRO_IB_VREF_DQ_1) >> 24) & 0x7F) - (mtc_table_entry->save_restore_mod_regs[7] & 0x7F);

			mtc_table_entry->trim_regs.emc_pmacro_ib_vref_dq_1_idx =
				 ((ib_vref_dq_byte4_icr & 0x7F)
				| (ib_vref_dq_byte5_icr & 0x7F) << 8)
				| ((ib_vref_dq_byte6_icr & 0x7F) << 16)
				| ((ib_vref_dq_byte7_icr & 0x7F) << 24);
		}
	}

	if (needs_wr_training)
	{
		mtc_table_entry->trim_perch_regs.emc0_data_brlshft_0_idx = EMC_CH0(EMC_DATA_BRLSHFT_0);
		mtc_table_entry->trim_perch_regs.emc1_data_brlshft_0_idx = channel1_enabled ? EMC_CH1(EMC_DATA_BRLSHFT_0) : 0;

		if (dram_dev_num == TWO_RANK)
		{
			mtc_table_entry->trim_perch_regs.emc0_data_brlshft_1_idx = EMC_CH0(EMC_DATA_BRLSHFT_1);
			mtc_table_entry->trim_perch_regs.emc1_data_brlshft_1_idx = channel1_enabled ? EMC_CH1(EMC_DATA_BRLSHFT_1) : 0;
		}

		mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_0_idx = EMC_CH0(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_0);
		mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_1_idx = EMC_CH0(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_1);
		mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_2_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_2) : 0;
		mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_3_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_3) : 0;

		if (dram_dev_num == TWO_RANK)
		{
			mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_long_dq_rank1_0_idx = EMC_CH0(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_0);
			mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_long_dq_rank1_1_idx = EMC_CH0(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_1);
			mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_long_dq_rank1_2_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_2) : 0;
			mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_long_dq_rank1_3_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_3) : 0;
		}

		if (needs_training_in_self_refresh)
		{
			mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte0_0_idx = EMC_CH0(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE0_0);
			mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte0_1_idx = EMC_CH0(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE0_1);
			mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte0_2_idx = EMC_CH0(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE0_2);
			mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte1_0_idx = EMC_CH0(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE1_0);
			mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte1_1_idx = EMC_CH0(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE1_1);
			mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte1_2_idx = EMC_CH0(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE1_2);
			mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte2_0_idx = EMC_CH0(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE2_0);
			mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte2_1_idx = EMC_CH0(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE2_1);
			mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte2_2_idx = EMC_CH0(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE2_2);
			mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte3_0_idx = EMC_CH0(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE3_0);
			mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte3_1_idx = EMC_CH0(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE3_1);
			mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte3_2_idx = EMC_CH0(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE3_2);
			mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte4_0_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_0) : 0;
			mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte4_1_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_1) : 0;
			mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte4_2_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_2) : 0;
			mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte5_0_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_0) : 0;
			mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte5_1_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_1) : 0;
			mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte5_2_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_2) : 0;
			mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte6_0_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_0) : 0;
			mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte6_1_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_1) : 0;
			mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte6_2_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_2) : 0;
			mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte7_0_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_0) : 0;
			mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte7_1_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_1) : 0;
			mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte7_2_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_2) : 0;

			if (dram_dev_num == TWO_RANK)
			{
				mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte0_0_idx = EMC_CH0(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE0_0);
				mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte0_1_idx = EMC_CH0(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE0_1);
				mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte0_2_idx = EMC_CH0(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE0_2);
				mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte1_0_idx = EMC_CH0(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE1_0);
				mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte1_1_idx = EMC_CH0(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE1_1);
				mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte1_2_idx = EMC_CH0(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE1_2);
				mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte2_0_idx = EMC_CH0(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE2_0);
				mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte2_1_idx = EMC_CH0(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE2_1);
				mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte2_2_idx = EMC_CH0(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE2_2);
				mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte3_0_idx = EMC_CH0(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE3_0);
				mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte3_1_idx = EMC_CH0(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE3_1);
				mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte3_2_idx = EMC_CH0(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE3_2);
				mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte4_0_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_0) : 0;
				mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte4_1_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_1) : 0;
				mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte4_2_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_2) : 0;
				mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte5_0_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_0) : 0;
				mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte5_1_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_1) : 0;
				mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte5_2_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_2) : 0;
				mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte6_0_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_0) : 0;
				mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte6_1_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_1) : 0;
				mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte6_2_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_2) : 0;
				mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte7_0_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_0) : 0;
				mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte7_1_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_1) : 0;
				mtc_table_entry->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte7_2_idx = channel1_enabled ? EMC_CH1(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_2) : 0;
			}
		}

		if (needs_wr_vref_training) // mode 12/13 (MRW).
		{
			u32 emc1_ranks_sub_partitions = 0;
			emc1_ranks_sub_partitions = channel1_enabled ? EMC_CH1(EMC_TRAINING_OPT_DQ_OB_VREF) : 0;

			u8 emc0_ib_vref_dq_byte8_modded_plus = mtc_table_entry->save_restore_mod_regs[8] + EMC_CH0(EMC_TRAINING_OPT_DQ_OB_VREF);
			if (mtc_table_entry->save_restore_mod_regs[8] & 0x80000000) // < 0 check.
				emc0_ib_vref_dq_byte8_modded_plus = EMC_CH0(EMC_TRAINING_OPT_DQ_OB_VREF) - mtc_table_entry->save_restore_mod_regs[8];

			u8 emc0_mrw12_op_sp1 = ((EMC_CH0(EMC_TRAINING_OPT_DQ_OB_VREF) & 0xFFFF) >> 8) + mtc_table_entry->save_restore_mod_regs[9];
			if (mtc_table_entry->save_restore_mod_regs[9] & 0x80000000)
				emc0_mrw12_op_sp1 = ((EMC_CH0(EMC_TRAINING_OPT_DQ_OB_VREF) & 0xFFFF) >> 8) - mtc_table_entry->save_restore_mod_regs[9];

			u8 emc0_mrw13_op_sp0 = ((EMC_CH0(EMC_TRAINING_OPT_DQ_OB_VREF) >> 16) & 0xFF) + mtc_table_entry->save_restore_mod_regs[8];
			if (mtc_table_entry->save_restore_mod_regs[8] & 0x80000000)
				emc0_mrw13_op_sp0 = ((EMC_CH0(EMC_TRAINING_OPT_DQ_OB_VREF) >> 16) & 0xFF) - mtc_table_entry->save_restore_mod_regs[8];

			u8 emc0_ib_vref_dq_byte9_modded_a_plus = mtc_table_entry->save_restore_mod_regs[9] + (EMC_CH0(EMC_TRAINING_OPT_DQ_OB_VREF) >> 24);
			if (mtc_table_entry->save_restore_mod_regs[9] & 0x80000000)
				emc0_ib_vref_dq_byte9_modded_a_plus = (EMC_CH0(EMC_TRAINING_OPT_DQ_OB_VREF) >> 24) - (u8)mtc_table_entry->save_restore_mod_regs[9];

			u8 emc0_ib_vref_dq_byte10_modded_plus = emc1_ranks_sub_partitions + mtc_table_entry->save_restore_mod_regs[10];
			if (mtc_table_entry->save_restore_mod_regs[10] & 0x80000000)
				emc0_ib_vref_dq_byte10_modded_plus = emc1_ranks_sub_partitions - mtc_table_entry->save_restore_mod_regs[10];

			u8 emc0_ib_vref_dq_byte11_modded_plus = ((emc1_ranks_sub_partitions & 0xFFFF) >> 8) + mtc_table_entry->save_restore_mod_regs[11];
			if (mtc_table_entry->save_restore_mod_regs[11] & 0x80000000)
				emc0_ib_vref_dq_byte11_modded_plus = ((emc1_ranks_sub_partitions & 0xFFFF) >> 8) - mtc_table_entry->save_restore_mod_regs[11];

			u8 emc1_mrw13_op_sp0 = ((emc1_ranks_sub_partitions >> 16) & 0xFF) + mtc_table_entry->save_restore_mod_regs[10];
			if (mtc_table_entry->save_restore_mod_regs[10] & 0x80000000)
				emc1_mrw13_op_sp0 = ((emc1_ranks_sub_partitions >> 16) & 0xFF) - mtc_table_entry->save_restore_mod_regs[10];

			u8 emc1_mrw13_op_sp1 = (emc1_ranks_sub_partitions >> 24) + mtc_table_entry->save_restore_mod_regs[11];
			if (mtc_table_entry->save_restore_mod_regs[11] & 0x80000000)
				emc1_mrw13_op_sp1 = (emc1_ranks_sub_partitions >> 24) - mtc_table_entry->save_restore_mod_regs[11];

			u32 mr13_dev_ext_cnt_sp_addr = 0xC80E0000;
			if (dram_dev_num == TWO_RANK)
				mr13_dev_ext_cnt_sp_addr = 0x480E0000;

			mtc_table_entry->burst_reg_per_ch.emc1_mrw12_idx = (u8)emc0_ib_vref_dq_byte10_modded_plus | 0x880E0000 | (emc0_ib_vref_dq_byte11_modded_plus << 8);
			mtc_table_entry->burst_reg_per_ch.emc0_mrw12_idx = emc0_ib_vref_dq_byte8_modded_plus | 0x880E0000 | (emc0_mrw12_op_sp1 << 8);
			mtc_table_entry->burst_reg_per_ch.emc0_mrw13_idx = emc0_ib_vref_dq_byte9_modded_a_plus << 8 | emc0_mrw13_op_sp0 | mr13_dev_ext_cnt_sp_addr;
			mtc_table_entry->burst_reg_per_ch.emc1_mrw13_idx = (emc1_mrw13_op_sp1 << 8) | emc1_mrw13_op_sp0 | mr13_dev_ext_cnt_sp_addr;
			
		}
	}
}

s32 _minerva_set_clock(emc_table_t *src_emc_entry, emc_table_t *dst_emc_entry, u32 needs_training, u32 selected_clk_src_emc)
{
	u32 emc_dbg_o;
	u32 emc_pin_o;
	u32 emc_cfg_pipe_clk_o;
	u32 emc_sel_dpd_ctrl;
	u32 emc_cfg;
	u32 emc_dbg_val;
	u32 emc_zq_cal = 0;
	u32 ramp_up_wait;
	u32 ramp_down_wait;
	u32 bg_regulator_mode_change;
	u32 mr13_flip_fspop = 0;
	u32 mr13_flip_fspwr = 0; //float
	u32 mr13_catr_enable; //float
	bool opt_zcal_en_cc;

	/* needs_training LOBYTE table var */
	/*
	 | bit | Description                |
	 |-----|----------------------------|
	 |  0  | Needs CA        training   |
	 |  1  | Needs CA_VREF   training   |
	 |  2  | Needs QUSE      training   |
	 |  3  | Needs QUSE_VREF training   |
	 |  4  | Needs WR        training   |
	 |  5  | Needs WR_VREF   training   |
	 |  6  | Needs RD        training   |
	 |  7  | Needs RD_VREF   training   |
	 */
	
	bool opt_dll_mode = false;
	bool is_lpddr3_dram = false;
	bool compensate_trimmer_applicable = false;
	bool needs_ca_or_cavref_training = (needs_training & 3) != 0;
	bool needs_tristate_training = (needs_training & 0xF7) != 0;
	bool needs_ca_training = needs_training & 1;
	bool needs_ca_vref_training = (needs_training >> 1) & 1;
	bool needs_quse_training = (needs_training >> 2) & 1;
	bool needs_quse_vref_training = (needs_training >> 3) & 1;
	bool needs_wr_training = (needs_training >> 4) & 1;
	bool needs_wr_vref_training = (needs_training >> 5) & 1;
	bool needs_rd_training = (needs_training >> 6) & 1;
	bool needs_rd_vref_training = (needs_training >> 7) & 1;
	bool needs_swap_rank_training = (needs_training >> 8) & 1;

	bool zcal_resistor_shared = (src_emc_entry->burst_regs.emc_zcal_wait_cnt_idx >> 31) & 1;
	bool enable_bg_regulator = (dst_emc_entry->burst_regs.emc_pmacro_bg_bias_ctrl_0_idx & 1) ^ 1;
	bool channel1_enabled = (src_emc_entry->burst_regs.emc_fbio_cfg7_idx >> 2) & 1;
	s32  dram_type = EMC(EMC_FBIO_CFG5) & 3;
	s32  dram_dev_num = (MC(MC_EMEM_ADR_CFG) & 1) + 1;
	
	float src_clock_period = 1000000.0 / (double)src_emc_entry->rate_khz;
	float dst_clock_period = 1000000.0 / (double)dst_emc_entry->rate_khz;

	fsp_for_src_freq = !fsp_for_src_freq;
	
	if (dst_emc_entry->burst_regs.emc_zcal_interval_idx && !src_emc_entry->burst_regs.emc_zcal_interval_idx)
		opt_zcal_en_cc = true;
	else
		opt_zcal_en_cc = dram_type == DRAM_TYPE_LPDDR4;

	switch(dram_type)
	{
	case DRAM_TYPE_DDR2:
	case DRAM_TYPE_LPDDR4:
		break;
	case DRAM_TYPE_DDR3:
		opt_dll_mode = (dst_emc_entry->emc_emrs & 1) ^ 1;
		break;
	
	case DRAM_TYPE_LPDDR2:
		if ((dst_emc_entry->burst_regs.emc_fbio_cfg5_idx >> 25) & 1) //LPDDR3_DRAM bit
			is_lpddr3_dram = true;
		break;
	}

	u32 tFC_lpddr4 = dst_emc_entry->dram_timings.t_fc_lpddr4;
	float tZQCAL_lpddr4 = 1000.0f;
	if (src_clock_period <= 2.0)
		tZQCAL_lpddr4 = (float)(1000 - tFC_lpddr4);
	s32 tZQCAL_lpddr4_fc_adj = (s32)(float)(tZQCAL_lpddr4 / dst_clock_period);

	// Step 1 - Pre DVFS SW sequence.
	EPRINTF("Step 1");
	emc_dbg_o = EMC(EMC_DBG);
	emc_pin_o = EMC(EMC_PIN);
	emc_cfg = dst_emc_entry->burst_regs.emc_cfg_idx & 0xFFFFFFF;
	emc_sel_dpd_ctrl = dst_emc_entry->emc_sel_dpd_ctrl & 0xFFFFFEC3;
	emc_cfg_pipe_clk_o = EMC(EMC_CFG_PIPE_CLK);
	_digital_dll_disable();

	// Step 1.2 - Disable AUTOCAL temporarily.
	EPRINTF("Step 1.2");
	EMC(EMC_AUTO_CAL_CONFIG) = (dst_emc_entry->emc_auto_cal_config & 0x7FFFF9FF) | 0x600;

	// Step 1.3 - Disable other power features.
	EPRINTF("Step 1.3");
	EMC(EMC_DBG) = emc_dbg_o | 2;
	EMC(EMC_CFG) = emc_cfg;
	EMC(EMC_SEL_DPD_CTRL) = emc_sel_dpd_ctrl;
	EMC(EMC_DBG) = emc_dbg_o;
	
	if (!needs_tristate_training && dst_emc_entry->periodic_training)
	{
		if (dram_dev_num == TWO_RANK)
		{
			_wait_emc_status(EMC_EMC_STATUS, IN_POWERDOWN_MASK, false, EMC_CH0);
			if (channel1_enabled)
				_wait_emc_status(EMC_EMC_STATUS, IN_POWERDOWN_MASK, false, channel1_enabled);
		}
		else
		{
			_wait_emc_status(EMC_EMC_STATUS, 0x10, false, 0);
			if (channel1_enabled)
				_wait_emc_status(EMC_EMC_STATUS, 0x10, false, channel1_enabled);
		}

		_wait_emc_status(EMC_EMC_STATUS, IN_SELF_REFRESH_MASK, false, EMC_CH0);
		if (channel1_enabled)
			_wait_emc_status(EMC_EMC_STATUS, IN_SELF_REFRESH_MASK, false, channel1_enabled);

		// Reset clock tree delays.
		dst_emc_entry->current_dram_clktree_c0d0u0 = dst_emc_entry->trained_dram_clktree_c0d0u0;
		dst_emc_entry->current_dram_clktree_c0d0u1 = dst_emc_entry->trained_dram_clktree_c0d0u1;
		dst_emc_entry->current_dram_clktree_c0d1u0 = dst_emc_entry->trained_dram_clktree_c0d1u0;
		dst_emc_entry->current_dram_clktree_c0d1u1 = dst_emc_entry->trained_dram_clktree_c0d1u1;
		dst_emc_entry->current_dram_clktree_c1d0u0 = dst_emc_entry->trained_dram_clktree_c1d0u0;
		dst_emc_entry->current_dram_clktree_c1d0u1 = dst_emc_entry->trained_dram_clktree_c1d0u1;
		dst_emc_entry->current_dram_clktree_c1d1u0 = dst_emc_entry->trained_dram_clktree_c1d1u0;
		dst_emc_entry->current_dram_clktree_c1d1u1 = dst_emc_entry->trained_dram_clktree_c1d1u1;

		u32 adel = _minerva_periodic_compensation_handler(src_emc_entry, dst_emc_entry, dram_dev_num, channel1_enabled, DVFS_SEQUENCE);

		if (((dst_emc_entry->rate_khz / 1000) << 7) * adel / 1000000 > dst_emc_entry->tree_margin)
			compensate_trimmer_applicable = true;
	}

	EMC(EMC_INTSTATUS) = CLKCHANGE_COMPLETE_INT;
	EMC(EMC_DBG) = emc_dbg_o | 2;
	EMC(EMC_CFG) = emc_cfg;
	EMC(EMC_SEL_DPD_CTRL) = emc_sel_dpd_ctrl;
	EMC(EMC_CFG_PIPE_CLK) = emc_cfg_pipe_clk_o | 1; // CLK_ALWAYS_ON.
	EMC(EMC_FDPD_CTRL_CMD_NO_RAMP) = dst_emc_entry->emc_fdpd_ctrl_cmd_no_ramp & 0xFFFFFFFE;

	bg_regulator_mode_change = src_emc_entry->burst_regs.emc_pmacro_bg_bias_ctrl_0_idx ^ dst_emc_entry->burst_regs.emc_pmacro_bg_bias_ctrl_0_idx;
	bg_regulator_mode_change = (bg_regulator_mode_change | (bg_regulator_mode_change >> 2)) & 1;

	if (bg_regulator_mode_change)
	{
		EMC(EMC_DBG) = emc_dbg_o | 2;
		if (enable_bg_regulator)
			EMC(EMC_PMACRO_BG_BIAS_CTRL_0) = src_emc_entry->burst_regs.emc_pmacro_bg_bias_ctrl_0_idx & 0xFFFFFFFE;
		else
			EMC(EMC_PMACRO_BG_BIAS_CTRL_0) = src_emc_entry->burst_regs.emc_pmacro_bg_bias_ctrl_0_idx & 0xFFFFFFFB;
	}

	// Check if we need to turn on VREF generator.
	if ((!(src_emc_entry->burst_regs.emc_pmacro_data_pad_tx_ctrl_idx & 0x100)
		&& (dst_emc_entry->burst_regs.emc_pmacro_data_pad_tx_ctrl_idx & 0x100))
		|| (!(src_emc_entry->burst_regs.emc_pmacro_data_pad_tx_ctrl_idx & 1) 
		&& (dst_emc_entry->burst_regs.emc_pmacro_data_pad_tx_ctrl_idx & 1)))
	{
		EMC(EMC_PMACRO_DATA_PAD_TX_CTRL) =
			(((dst_emc_entry->burst_regs.emc_pmacro_data_pad_tx_ctrl_idx & 1) | (src_emc_entry->burst_regs.emc_pmacro_data_pad_tx_ctrl_idx & 0xFFFFFFFE)) & 0xFFFFFEFF)
			| (((dst_emc_entry->burst_regs.emc_pmacro_data_pad_tx_ctrl_idx >> 8) & 0x1) << 8);
		_usleep(1);
	}
	else if (bg_regulator_mode_change)
		_usleep(1);

	EMC(EMC_DBG) = emc_dbg_o;

	// Step 2 - Prelock the DLL.
	EPRINTF("Step 2");
	if (dst_emc_entry->burst_regs.emc_cfg_dig_dll_idx & 1)
		_digital_dll_prelock(dst_emc_entry, needs_tristate_training, selected_clk_src_emc); //Prelock enabled for target frequency.
	else
	{
		_change_dll_src(dst_emc_entry, selected_clk_src_emc);
		_digital_dll_disable(); // Disabling DLL for target frequency.
	}

	// Step 3 - Prepare autocal for the clock change.
	EPRINTF("Step 3");
	EMC(EMC_AUTO_CAL_CONFIG) = (dst_emc_entry->emc_auto_cal_config & 0x7FFFF9FF) | 0x600;
	EMC(EMC_DBG) = emc_dbg_o | 2;
	EMC(EMC_AUTO_CAL_CONFIG2) = dst_emc_entry->emc_auto_cal_config2;
	EMC(EMC_AUTO_CAL_CONFIG3) = dst_emc_entry->emc_auto_cal_config3;
	EMC(EMC_AUTO_CAL_CONFIG4) = dst_emc_entry->emc_auto_cal_config4;
	EMC(EMC_AUTO_CAL_CONFIG5) = dst_emc_entry->emc_auto_cal_config5;
	EMC(EMC_AUTO_CAL_CONFIG6) = dst_emc_entry->emc_auto_cal_config6;
	EMC(EMC_AUTO_CAL_CONFIG7) = dst_emc_entry->emc_auto_cal_config7;
	EMC(EMC_AUTO_CAL_CONFIG8) = dst_emc_entry->emc_auto_cal_config8;
	EMC(EMC_DBG) = emc_dbg_o;
	EMC(EMC_AUTO_CAL_CONFIG) = (dst_emc_entry->emc_auto_cal_config & 0x7FFFF9FE) | 0x601;

	// Step 4 - Update EMC_CFG.
	EPRINTF("Step 4");
	if (src_clock_period <= 50.0f || dram_type != 1)
		EMC(EMC_CFG_2) = dst_emc_entry->emc_cfg_2;
	else
		_ccfifo_write(EMC_SELF_REF, 1, 0);

	// Step 5 - Prepare reference variables for ZQCAL regs.
	EPRINTF("Step 5");
	// u32 zq_wait_long = 0;
	// u32 zq_wait_short = 0;

	// if (dram_type == DRAM_TYPE_LPDDR4)
	// 	zq_wait_long = _fceil(1000.0f / dst_clock_period);
	// else if (is_lpddr3_dram || dram_type == DRAM_TYPE_LPDDR2)
	// 	zq_wait_long = _fceil(360.0f / dst_clock_period);
	// else if (!dram_type)
	// 	zq_wait_long = _fceil(320.0f / dst_clock_period);

	// if (is_lpddr3_dram || dram_type == DRAM_TYPE_LPDDR2)
	// 	zq_wait_short = _fceil(90.0f / dst_clock_period);
	// else if (!dram_type)
	// 	zq_wait_short = _fceil(80.0f / dst_clock_period);

	// Step 7 - Bug 200024907 - Patch RP R2P.
	EPRINTF("Step 7");
	if (needs_ca_or_cavref_training && dram_dev_num == TWO_RANK)
		EMC(EMC_PIN) = 0x107;

	if (dram_type == DRAM_TYPE_LPDDR4)
	{
		u32 R2P_war = 0;
		u32 TRPab_war = 0;
		u32 RP_war = 0;
		u32 W2P_war = 0;

		s32 nRTP = 8;  // <= 1066MHz.
		if (src_clock_period < 3.7593985           // 1000 / 266MHz.
				&& src_clock_period < 1.87617261   // 1000 / 533MHz.
				&& src_clock_period < 1.25         // 1000 / 800MHz.
				&& src_clock_period < 0.938086304) // 1000 / 1066MHz.
			nRTP = 10; // 1067MHz < x <= 1333MHz.
		if (src_clock_period < 0.750187547)        // 1000 / 1333MHz.
			nRTP = 12; // 1333MHz < x <= 1333MHz.
		if (src_clock_period < 0.625)              // 1000 / 1600MHz.
			nRTP = 14; // 1600MHz < x <= 1866MHz.
		if (src_clock_period < 0.535905681)        // 1000 / 1866MHz.
			nRTP = 16; // > 1866MHz
		
		float tRPST = (float)((src_emc_entry->emc_mrw >> 7) & 1) + 0.5f;

		s32 deltaTWATM = _fceil(7.5f / src_clock_period);
		if (deltaTWATM < 8)
			deltaTWATM = 8;

		u32 tRTM = (u32)_fceil((float)((((float)src_emc_entry->dram_timings.rl + _fceil(3.6f / src_clock_period) + (float)deltaTWATM) + tRPST) + (float)nRTP));

		if (tRTM <= src_emc_entry->burst_regs.emc_rp_idx + src_emc_entry->burst_regs.emc_r2p_idx)
		{
			TRPab_war = src_emc_entry->burst_regs.emc_trpab_idx;
			R2P_war = src_emc_entry->burst_regs.emc_r2p_idx;
			RP_war = src_emc_entry->burst_regs.emc_rp_idx;
		}
		else
		{
			R2P_war = tRTM - src_emc_entry->burst_regs.emc_rp_idx;
			TRPab_war = src_emc_entry->burst_regs.emc_trpab_idx;
			RP_war = src_emc_entry->burst_regs.emc_rp_idx;
			if (R2P_war > 63)
			{
				RP_war = tRTM - 63;
				R2P_war = 63;
				if (src_emc_entry->burst_regs.emc_trpab_idx < tRTM - 63)
					TRPab_war = tRTM - 63;
				else
					TRPab_war = src_emc_entry->burst_regs.emc_trpab_idx;
			}
		}

		if (RP_war >= deltaTWATM)
			W2P_war = src_emc_entry->burst_regs.emc_w2p_idx;
		else
		{
			u32 W2P_war_temp = deltaTWATM + src_emc_entry->burst_regs.emc_w2p_idx;
			W2P_war = W2P_war_temp - RP_war;
			if (W2P_war > 63)
			{
				RP_war = W2P_war_temp - 63;
				W2P_war = 63;
				if (TRPab_war < RP_war)
					TRPab_war = RP_war;
			}
		}

		if ( src_emc_entry->burst_regs.emc_w2p_idx != W2P_war
			|| src_emc_entry->burst_regs.emc_rp_idx != RP_war
			|| src_emc_entry->burst_regs.emc_r2p_idx != R2P_war
			|| src_emc_entry->burst_regs.emc_trpab_idx != TRPab_war)
		{
			EMC(EMC_DBG) = emc_dbg_o | 2;
			EMC(EMC_RP) = RP_war;
			EMC(EMC_R2P) = R2P_war;
			EMC(EMC_W2P) = W2P_war;
			EMC(EMC_TRPAB) = TRPab_war;
			EMC(EMC_DBG) = emc_dbg_o;
			_usleep(1);
		}
	}

	// Step 7.2 - Program FSP reference registers and send MRWs to new FSPWR.
	EPRINTF("Step 7.2");
	mr13_catr_enable = 0;
	if (fsp_for_src_freq)
	{
		mr13_flip_fspop = dst_emc_entry->emc_mrw3 | 0xC0;
		mr13_flip_fspwr = (dst_emc_entry->emc_mrw3 & 0xFFFFFF3F) | 0x40;
	}
	else
	{
		mr13_flip_fspop = dst_emc_entry->emc_mrw3 & 0xFFFFFF3F;
		mr13_flip_fspwr = mr13_flip_fspop | 0x80;
	}

	if (needs_ca_or_cavref_training && dram_dev_num == TWO_RANK)
	{
		if (needs_swap_rank_training)
		{
			mr13_flip_fspop = (mr13_flip_fspop & 0x3FFFFFFF) | 0x80000000;
			mr13_catr_enable = (mr13_flip_fspwr & 0x3FFFFFFF)| 0x40000001;
		}
		else
		{
			mr13_flip_fspop = (mr13_flip_fspop & 0x3FFFFFFF) | 0x40000000;
			mr13_catr_enable = (mr13_flip_fspwr & 0x3FFFFFFF) | 0x80000001;
		}
	}
	else if (dram_dev_num == TWO_RANK)
	{
		if (needs_swap_rank_training)
			mr13_catr_enable = (mr13_flip_fspwr & 0x3FFFFFFF) | 0x40000001;
		else
			mr13_catr_enable = (mr13_flip_fspwr & 0x3FFFFFFF) | 0x80000001;
	}
	else
		mr13_catr_enable = mr13_flip_fspwr | 1;

	if (dram_type == DRAM_TYPE_LPDDR4)
	{
		EMC(EMC_MRW3) = mr13_flip_fspwr;
		EMC(EMC_MRW) = dst_emc_entry->emc_mrw;
		EMC(EMC_MRW2) = dst_emc_entry->emc_mrw2;
	}

	// Step 8 - Program the shadow registers.
	EPRINTF("Step 8");
	// Writing burst_regs.
	u32 reg_addr = 0;
	u32 reg_val = 0;
	u32 reg_check = false;
	burst_regs_table_t *dst_burst_regs = (burst_regs_table_t *)&dst_emc_entry->burst_regs;

	for (u32 i = 0; dst_emc_entry->num_burst > i; i++)
	{
		reg_check = false;
		reg_addr = burst_regs_emc_addr_table[i];
		if (needs_tristate_training)
		{
			if (needs_ca_or_cavref_training)
				reg_val = dst_burst_regs->shadow_regs_ca_train[i];
			else if (needs_training & 0xC)
				reg_val = dst_burst_regs->shadow_regs_quse_train[i];
			else if (needs_training & 0xF0)
				reg_val = dst_burst_regs->shadow_regs_rdwr_train[i];
			else
				continue;
		}
		else
			reg_val = dst_burst_regs->burst_regs[i];

		if ((reg_addr & 0xFFF7) != EMC_MRW6
			&& (reg_addr - EMC_MRW7) & 0xFFFF7
			//&& (reg_addr & 0xEFF7) != 0x34B4 // EMC_MRW10.
			&& ((reg_addr & 0xEFFF) - 0x34B8) & 0xFFF7 // EMC_MRW11.
			&& reg_addr != EMC_TRAINING_CTRL
			&& reg_addr != EMC_MRW14
			&& reg_addr != EMC_MRW15)
		{
			reg_check = true;
		}

		if (reg_check && reg_addr == EMC_CFG)
		{
			if (dram_type == DRAM_TYPE_LPDDR4)
				reg_val &= 0xFFFFFFF;
			else
				reg_val &= 0xCFFFFFFF;
			EMC(reg_addr) = reg_val;
			continue;
		}
		if (!reg_check && dram_type != DRAM_TYPE_LPDDR4)
			continue;

		if (reg_addr != EMC_CFG)// EMC_CFG
		{
			if (reg_addr != EMC_ZCAL_INTERVAL || !opt_zcal_en_cc)
			{
				switch ( reg_addr )
				{
					case EMC_PMACRO_AUTOCAL_CFG_COMMON:
						reg_val |= 0x10000;
						break;
					case EMC_PMACRO_DATA_PAD_TX_CTRL:
						reg_val &= 0xFEFEFDFD;
						break;
					case EMC_PMACRO_CMD_PAD_TX_CTRL:
						reg_val = (reg_val & 0xFAFEFDFD) | 0x4000000;
						break;
					case EMC_PMACRO_BRICK_CTRL_RFU1:
						reg_val &= 0xF800F800;
						break;
					case EMC_PMACRO_COMMON_PAD_TX_CTRL:
						reg_val &= 0xFFFFFFF0;
						break;
					case EMC_TRAINING_CTRL:
						reg_val |= needs_swap_rank_training << 14;// bit15 is TR_IN_SELF_REFRESH
						break;
				}
			}
			else
				reg_val = 0;
		}
		else
			reg_val &= 0xFFFFFFF;

		EMC(reg_addr) = reg_val;
	}

	if (needs_tristate_training)
		EMC(EMC_MRW) = (src_emc_entry->run_clocks & 0xFF) | 0x170000;
	else
		EMC(EMC_MRW) = (dst_emc_entry->run_clocks & 0xFF) | 0x170000;

	// Writing burst_regs_per_ch.
	for (u32 i = 0; dst_emc_entry->num_burst_per_ch > i; i++)
	{
		reg_addr = burst_reg_per_ch_emc01_addr_table[i];
		if (reg_addr
			&& (((((reg_addr & 0xFFF) - 0x4B8) & 0xFFFFFFF7) //EMC0_MRW11
					&& (reg_addr & 0xFFF) != 0x4B4 //EMC0_MRW10 - ALways true, because of constant table.
					&& (reg_addr & 0xFFFFFFF7) != EMC_MRW6
					&& reg_addr != EMC_MRW15
					&& reg_addr != EMC_MRW14
					&& ((reg_addr - EMC_MRW7) & 0xFFFFFFF7))
				|| dram_type == DRAM_TYPE_LPDDR4)
			&& (channel1_enabled || ((reg_addr - 0x4000) > 0xFFF)))
		{
			EMC(reg_addr) = dst_burst_regs->burst_reg_per_ch[i];
		}
	}

	// Writing vref_regs.
	trim_regs_table_t *trim_regs_table = (trim_regs_table_t *)&dst_emc_entry->trim_regs;
	for (u32 i = 0; dst_emc_entry->vref_num > i; i++)
	{
		reg_addr = vref_perch_regs_emc01_addr_table[i];
		if (reg_addr && (channel1_enabled || (reg_addr - 0x4000) > 0xFFF))
			EMC(reg_addr) = trim_regs_table->vref_perch_regs[i];
	}

	// Writing training mod regs.
	if (needs_tristate_training)
	{
		for (u32 i = 0; dst_emc_entry->training_mod_num > i; i++)
		{
			reg_addr = training_mod_regs_emc01_addr_table[i];
			if (reg_addr && (channel1_enabled || (reg_addr - 0x4000) > 0xFFF))
				EMC(reg_addr) = dst_emc_entry->training_mod_regs[i];
		}
	}

	// Writing trim_regs
	for (u32 i = 0; dst_emc_entry->num_trim > i; i++)
	{
		reg_addr = trim_regs_emc_addr_table[i];
		if (reg_addr)
		{
			if (((reg_addr & 0xFFFFFFF3) == EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_0
				 || (reg_addr & 0xFFFFFFF3) == EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_0
				 || (reg_addr & 0xFFFFFFFB) == EMC_DATA_BRLSHFT_0)
				&& compensate_trimmer_applicable)
			{
				EMC(reg_addr) = _minerva_apply_periodic_compensation_trimmer(dst_emc_entry, reg_addr);
			}
			else
				EMC(reg_addr) = trim_regs_table->trim_regs[i];
		}
	}
															
	 // Writing trim_regs_per_ch
	reg_val = 0;
	for (u32 i = 0; dst_emc_entry->num_trim_per_ch > i; i++)
	{
		reg_addr = trim_perch_regs_emc01_addr_table[i];
		if (reg_addr && (channel1_enabled || reg_addr - 0x4000 > 0xFFF))
		{
			if (((reg_addr & 0xFFFFFFF3) == EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_0
				 || (reg_addr & 0xFFFFFFF3) == EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_0
				 || (reg_addr & 0xFFFFFFFB) == EMC_DATA_BRLSHFT_0)
				&& compensate_trimmer_applicable )
			{
				reg_val = _minerva_apply_periodic_compensation_trimmer(dst_emc_entry, reg_addr & 0xFFF);
			}
			else if (((reg_addr & 0xFFFFFFFB) == 0x3660
				 || (reg_addr & 0xFFFFFFDF) == 0x3648
				 || (reg_addr & 0xFFFFFFF7) == 0x3644
				 || reg_addr == 0x366C
				 || reg_addr == EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_0
				 || (reg_addr & 0xFFFFFFFB) == 0x3588)
				&& compensate_trimmer_applicable )
			{
				reg_val = _minerva_apply_periodic_compensation_trimmer(dst_emc_entry, reg_addr & 0xFFF);
			}
			else if (((reg_addr & 0xFFFFFFF3) == 0x4660
				 || (reg_addr & 0xFFFFFFF3) == 0x4640
				 || (reg_addr & 0xFFFFFFFB) == 0x4588)
				&& compensate_trimmer_applicable)
			{
				reg_val = _minerva_apply_periodic_compensation_trimmer(dst_emc_entry, reg_addr & 0xFFF);
			}
			else
			{
				reg_val = trim_regs_table->trim_perch_regs[i];
			}
			EMC(reg_addr) = reg_val;
		}
	}

	if (needs_tristate_training)
	{
		// Check delta wrt previous values (save value if margin exceeds what is set in table).
		if (needs_wr_training && dst_emc_entry->periodic_training)
			_minerva_periodic_compensation_handler(src_emc_entry, dst_emc_entry, dram_dev_num, channel1_enabled, WRITE_TRAINING_SEQUENCE);
	}
	else
	{
		// Writing burst_mc_regs.
		for (u32 i = 0; dst_emc_entry->num_mc_regs > i; i++)
			MC(burst_mc_regs_addr_table[i]) = dst_emc_entry->burst_mc_regs[i];
	}

	// Writing la_scale_regs.
	//if ((dst_emc_entry->rate_khz < src_emc_entry->rate_khz) && dst_emc_entry->num_up_down) //NEW TODO
	if ((dst_emc_entry->rate_khz < src_emc_entry->rate_khz) > needs_tristate_training)
	{
		for (u32 i = 0; dst_emc_entry->num_up_down > i; i++)
			MC(la_scale_regs_mc_addr_table[i]) = dst_emc_entry->la_scale_regs[i];
	}

	// Step 9 - LPDDR4.
	EPRINTF("Step 9");
	if (dram_type == DRAM_TYPE_LPDDR4)
	{
		EMC(EMC_ZCAL_INTERVAL) = src_emc_entry->burst_regs.emc_zcal_interval_idx & 0xFF000000;
		EMC(EMC_ZCAL_WAIT_CNT) = dst_emc_entry->burst_regs.emc_zcal_wait_cnt_idx & 0xFFFFF800;
		EMC(EMC_DBG) = emc_dbg_o | 0x40000002;
		EMC(EMC_ZCAL_INTERVAL) = src_emc_entry->burst_regs.emc_zcal_interval_idx & 0xFF000000;
		EMC(EMC_DBG) = emc_dbg_o;
		if (needs_tristate_training)
		{
			EMC(EMC_DBG) = emc_dbg_o | 2;
			EMC(EMC_PMACRO_AUTOCAL_CFG_COMMON) = dst_emc_entry->burst_regs.emc_pmacro_autocal_cfg_common_idx | 0x10000;
			if (needs_ca_or_cavref_training)
				EMC(EMC_FBIO_CFG5) = src_emc_entry->burst_regs.emc_fbio_cfg5_idx | 0x8000000;
			EMC(EMC_DBG) = emc_dbg_o;
			if (channel1_enabled)
				_ccfifo_write(EMC_CFG_SYNC, 0, 0);
			_ccfifo_write(EMC_DBG, (emc_dbg_o & 0xF3FFFFFF) | 0x4000000, 0);
		}
	}

	// Step 10 - Self refresh
	EPRINTF("Step 10");
	u32 emc_self_ref_val = 1;
	if (!opt_dll_mode && dram_type == DRAM_TYPE_DDR3)
		_ccfifo_write(EMC_EMRS, dst_emc_entry->emc_emrs, 0); 
	else if (dram_type == DRAM_TYPE_LPDDR4)
		emc_self_ref_val = 0x101;
						 
	_ccfifo_write(EMC_SELF_REF, emc_self_ref_val, 0);

	if (needs_ca_or_cavref_training < ((src_clock_period <= 2.0f) && dram_type == DRAM_TYPE_LPDDR4))
	{
		_ccfifo_write(EMC_MRW3, mr13_flip_fspwr ^ 0x40, 0); 
		_ccfifo_write(EMC_MRW6, (src_emc_entry->burst_regs.emc_mrw6_idx & 0xC0C0) | (dst_emc_entry->burst_regs.emc_mrw6_idx & 0xFFFF3F3F), 0); 
		_ccfifo_write(EMC_MRW14, (src_emc_entry->burst_regs.emc_mrw14_idx & 0x3838) | (dst_emc_entry->burst_regs.emc_mrw14_idx & 0xFFFF0707), 0); 
		if (dram_dev_num == TWO_RANK)
		{
			_ccfifo_write(EMC_MRW7, (src_emc_entry->burst_regs.emc_mrw7_idx & 0xC0C0) | (dst_emc_entry->burst_regs.emc_mrw7_idx & 0xFFFF3F3F), 0); 
			_ccfifo_write(EMC_MRW15, (src_emc_entry->burst_regs.emc_mrw15_idx & 0x3838) | (dst_emc_entry->burst_regs.emc_mrw15_idx & 0xFFFF0707), 0); 
		}
		if (opt_zcal_en_cc)
		{
			if (dram_dev_num == ONE_RANK || zcal_resistor_shared)
				emc_zq_cal = 0x80000001;
			else
				emc_zq_cal = 1;
			_ccfifo_write(EMC_ZQ_CAL, emc_zq_cal, 0);
		}
	}
	
	emc_dbg_val = emc_dbg_o;
	float tRP_src_timing = (float)((float)src_emc_entry->dram_timings.t_rp / src_clock_period);
	float tRFC_src_timing = (float)((float)src_emc_entry->dram_timings.t_rfc / src_clock_period);
	bool in_self_refresh = false;
	u32 ref_delay = 0;

	if (dram_type == DRAM_TYPE_LPDDR4)
	{
		if (needs_tristate_training)
		{
			emc_dbg_val = (emc_dbg_o & 0xF3FFFFFF) | 0x44000000;
			_ccfifo_write(EMC_DBG, emc_dbg_val, 0);
		}
		if (needs_ca_or_cavref_training)
		{
			_ccfifo_write(EMC_PMACRO_DATA_RX_TERM_MODE, src_emc_entry->burst_regs.emc_pmacro_data_rx_term_mode_idx & 0xFFFFFCCC, 0);
			if (dram_dev_num == TWO_RANK && needs_swap_rank_training)
			{
				_ccfifo_write(EMC_MRW3, mr13_flip_fspop | 8, (u32)tRP_src_timing);
				_ccfifo_write(EMC_MRW3, mr13_catr_enable | 8, 0);
			}
			else
				_ccfifo_write(EMC_MRW3, mr13_catr_enable | 8, (u32)tRP_src_timing);

			_ccfifo_write(EMC_TR_CTRL_0, 0x15A, 0);
			ref_delay = (u32)(1000.0 / src_clock_period);
		}
		else
		{
			_ccfifo_write(EMC_MRW3, mr13_flip_fspop | 8, (u32)tRP_src_timing);
			ref_delay = (u32)(float)((float)tFC_lpddr4 / src_clock_period);
		}
		_ccfifo_write(EMC_INTSTATUS, 0, ref_delay);
		_ccfifo_write(EMC_PIN, emc_pin_o & 0xFFFFFFF8, 30);
	}
	else
	{
		in_self_refresh = true;
		_ccfifo_write(EMC_SELF_REF, 0x1, 0);
	}

	// Step 11 - Ramp down.
	EPRINTF("Step 11");
	ref_delay = 0;
	if (dram_type != DRAM_TYPE_LPDDR4)
		ref_delay = (u32)(float)(tRP_src_timing + tRFC_src_timing + 20.0f);
	_ccfifo_write(EMC_CFG_SYNC, 0, ref_delay);
	_ccfifo_write(EMC_DBG, emc_dbg_val | 0x40000002, 0); // WRITE_MUX_ACTIVE | WRITE_ACTIVE_ONLY
	ramp_down_wait = _dvfs_power_ramp_down(false, src_emc_entry, dst_emc_entry, src_clock_period);

	// Step 12 - Trigger clock change.
	EPRINTF("Step 12");
	_ccfifo_write(EMC_STALL_THEN_EXE_AFTER_CLKCHANGE, 1, 0);
	if (!needs_tristate_training)
		_ccfifo_write(EMC_DBG, (emc_dbg_val & 0xBFFFFFFF) | 2, 0);

	// Step 13 - Ramp up.
	EPRINTF("Step 13");
	ramp_up_wait = _dvfs_power_ramp_up(false, src_emc_entry, dst_emc_entry, needs_training & 0xFF, dst_clock_period);
	_ccfifo_write(EMC_DBG, emc_dbg_val, 0);

	// Step 14 - Bringup CKE pins.
	EPRINTF("Step 14");
	if (dram_type == DRAM_TYPE_LPDDR4)
	{
		u32 emc_pin_val_final = 0;
		if (needs_ca_or_cavref_training)
		{
			emc_pin_val_final = emc_pin_o & 0xFFFFFFF8;
			if (dram_dev_num == TWO_RANK)
			{
				if (needs_swap_rank_training)
					emc_pin_val_final |= 5;
				else
					emc_pin_val_final |= 6;
			}
		}
		else if (dram_dev_num == TWO_RANK)
			emc_pin_val_final = emc_pin_o | 7;
		else
			emc_pin_val_final = (emc_pin_o & 0xFFFFFFF8) | 1;

		_ccfifo_write(EMC_PIN, emc_pin_val_final, 0);
	}

	// Step 15 - Zqlatch.
	EPRINTF("Step 15");
	if (dram_type == DRAM_TYPE_LPDDR4 && !needs_ca_or_cavref_training && opt_zcal_en_cc)
	{
		s32 zq_latch_dvfs_wait_time = 0;
		s32 T_PDEX_timing_final = 0;
		s32 T_PDEX_timing = _fceil((float)dst_emc_entry->dram_timings.t_pdex / dst_clock_period);

		if (src_clock_period > 2.0)
			zq_latch_dvfs_wait_time = (s32)(tZQCAL_lpddr4_fc_adj - T_PDEX_timing);
		else
			zq_latch_dvfs_wait_time =
				(s32)(tZQCAL_lpddr4_fc_adj - (s32)((float)(ramp_up_wait + ramp_down_wait) / dst_clock_period));

		if (dram_dev_num == ONE_RANK)
		{
			if (T_PDEX_timing < 0)
					T_PDEX_timing = 0;

			if (src_clock_period > 2.0)
				_ccfifo_write(EMC_ZQ_CAL, 0x80000001, T_PDEX_timing);

			if (!needs_tristate_training)
			{
				_ccfifo_write(EMC_MRW3, (mr13_flip_fspop & 0xF3FFFFF7) | 0xC000000, T_PDEX_timing);
				_ccfifo_write(EMC_SELF_REF, 0x100, 0);
				_ccfifo_write(EMC_REF, 0, 0);
			}
			emc_zq_cal = 0x80000002;
			if (zq_latch_dvfs_wait_time < 0)
				zq_latch_dvfs_wait_time = 0;
		}
		else if (zcal_resistor_shared)
		{
			if (src_clock_period > 2.0)
			{
				T_PDEX_timing_final = T_PDEX_timing;
				if (T_PDEX_timing < 0)
					T_PDEX_timing_final = 0;
				_ccfifo_write(EMC_ZQ_CAL, 0x80000001, T_PDEX_timing_final);
			}
			T_PDEX_timing_final = zq_latch_dvfs_wait_time + T_PDEX_timing;
			if ((zq_latch_dvfs_wait_time + T_PDEX_timing) < 0)
				T_PDEX_timing_final = 0;
			_ccfifo_write(EMC_ZQ_CAL, 0x80000002, T_PDEX_timing_final);
			_ccfifo_write(EMC_ZQ_CAL, 0x40000001, 0);
			if (!needs_tristate_training)
			{
				_ccfifo_write(EMC_MRW3, (mr13_flip_fspop & 0xF3FFFFF7) | 0xC000000, 0);
				_ccfifo_write(EMC_SELF_REF, 0x100, 0);
				_ccfifo_write(EMC_REF, 0, 0);
			}
			emc_zq_cal = 0x40000002;
			zq_latch_dvfs_wait_time = (s32)(float)(1000.0f / dst_clock_period);
		}
		else
		{
			if (T_PDEX_timing < 0)
				T_PDEX_timing = 0;

			if (src_clock_period > 2.0)
				_ccfifo_write(EMC_ZQ_CAL, 1, T_PDEX_timing);
			if (!needs_tristate_training)
			{
				_ccfifo_write(EMC_MRW3, (mr13_flip_fspop & 0xF3FFFFF7) | 0xC000000, T_PDEX_timing);
				_ccfifo_write(EMC_SELF_REF, 0x100, 0);
				_ccfifo_write(EMC_REF, 0, 0);
			}
			emc_zq_cal = 2;

			if (zq_latch_dvfs_wait_time < 0)
				zq_latch_dvfs_wait_time = 0;
		}
		_ccfifo_write(EMC_ZQ_CAL, emc_zq_cal, (u32)zq_latch_dvfs_wait_time);
	}
	
	_ccfifo_write(EMC_INTSTATUS, 0, 10); // WAR: delay for zqlatch.

	// Step 16 - LPDDR4 Conditional training kickoff.
	EPRINTF("Step 16");
	if (needs_tristate_training && dram_type == DRAM_TYPE_LPDDR4)
	{
		_ccfifo_write(EMC_INTSTATUS, 0, (u32)(1020.0f / dst_clock_period));

		u32 training_command = 0;

		if (needs_ca_training)
			training_command |= (1 << 1);  // CA: Initiates CA Training.
		if (needs_ca_vref_training)
			training_command |= (1 << 5);  // CA_VREF: Initiates CA_VREF Training.
		if (needs_quse_training)
			training_command |= (1 << 4);  // QUSE: Initiates QUSE Training.
		if (needs_quse_vref_training)
			training_command |= (1 << 8);  // QUSE_VREF: Initiates DQS_VREF Training.
		if (needs_wr_training)
			training_command |= (1 << 3);  // WR: Initiates WR Training.
		if (needs_wr_vref_training)
			training_command |= (1 << 6);  // WR_VREF: Initiates OB (wrire) DRAM_VREF Training.
		if (needs_rd_training)
			training_command |= (1 << 2);  // RD: Initiates RD Training.
		if (needs_rd_vref_training)
			training_command |= (1 << 7);  // RD_VREF: Initiates IB_DQ_VREF Training.
		training_command     |= (1 << 31); // GO: Start the Training.

		_ccfifo_write(EMC_TRAINING_CMD, training_command, 0);

		if (bg_regulator_mode_change)
		{
			if (enable_bg_regulator)
				_ccfifo_write(EMC_PMACRO_BG_BIAS_CTRL_0,
					src_emc_entry->burst_regs.emc_pmacro_bg_bias_ctrl_0_idx & 0xFFFFFFFE, 0);
			else
				_ccfifo_write(EMC_PMACRO_BG_BIAS_CTRL_0,
					src_emc_entry->burst_regs.emc_pmacro_bg_bias_ctrl_0_idx & 0xFFFFFFFB, 0);
		}

		_ccfifo_write(EMC_SWITCH_BACK_CTRL, 1, 0);

		if (!needs_ca_or_cavref_training || needs_swap_rank_training)
		{
			_ccfifo_write(EMC_MRW3, mr13_flip_fspop ^ 0xC0, 0);
			_ccfifo_write(EMC_INTSTATUS, 0, (u32)(1000.0f / dst_clock_period));
		}

		_ccfifo_write(EMC_PIN, emc_pin_o & 0xFFFFFFF8, 0);
		_ccfifo_write(EMC_CFG_SYNC, 0, 0);
		_ccfifo_write(EMC_DBG, emc_dbg_val | 0x40000002, 0);

		_dvfs_power_ramp_down(true, src_emc_entry, dst_emc_entry, dst_clock_period);

		_ccfifo_write(EMC_STALL_THEN_EXE_AFTER_CLKCHANGE, 1, 0);
		_ccfifo_write(EMC_DBG, (emc_dbg_val & 0xBFFFFFFF) | 2, 0);

		_dvfs_power_ramp_up(true, src_emc_entry, dst_emc_entry, needs_training, src_clock_period);

		_ccfifo_write(EMC_DBG, emc_dbg_val, 0);

		if (dram_dev_num == TWO_RANK)
			_ccfifo_write(EMC_PIN, emc_pin_o | 7, 0);
		else
			_ccfifo_write(EMC_PIN, (emc_pin_o & 0xFFFFFFF8) | 1, 0);

		if (needs_ca_or_cavref_training)
		{
			_ccfifo_write(EMC_TR_CTRL_0, 0x4A, (u32)(float)(200.0f / src_clock_period));
			_ccfifo_write(EMC_TR_CTRL_0, 0x40, (u32)(float)(1000.0f / src_clock_period));
			_ccfifo_write(EMC_MRW3, mr13_catr_enable & 0xFFFFFFFE, 0);
			_ccfifo_write(EMC_INTSTATUS, 0, (u32)(float)(1000.0f / src_clock_period));
			_ccfifo_write(EMC_PMACRO_DATA_RX_TERM_MODE, src_emc_entry->burst_regs.emc_pmacro_data_rx_term_mode_idx, 0);
		}
		_ccfifo_write(EMC_DBG, emc_dbg_o, 0);

		if (opt_zcal_en_cc)
		{
			_ccfifo_write(EMC_ZQ_CAL, 0x80000001, 0);
			_ccfifo_write(EMC_ZQ_CAL, 0x80000002, (u32)(float)(1000.0f / src_clock_period));

			if (zcal_resistor_shared && dram_dev_num == TWO_RANK)
			{
				if (!needs_ca_or_cavref_training || needs_swap_rank_training)
				{
					_ccfifo_write(EMC_ZQ_CAL, 0x40000001, 0);
					_ccfifo_write(EMC_ZQ_CAL, 0x40000002, (u32)(float)(1000.0f / src_clock_period));
					if (!needs_ca_or_cavref_training)
						_ccfifo_write(EMC_MRW3, ((mr13_flip_fspop ^ 0xC0) & 0xF3FFFFF7) | 0xC000000, 0);
				}

				_ccfifo_write(EMC_SELF_REF, 0x100, 0);

				goto step_19_2;
			}
			else if (dram_dev_num == TWO_RANK)
			{
				if (needs_ca_or_cavref_training && !needs_swap_rank_training)
				{
					_ccfifo_write(EMC_SELF_REF, 0x100, 0);

					goto step_19_2;
				}

				_ccfifo_write(EMC_ZQ_CAL, 0x40000001, 0);
				_ccfifo_write(EMC_ZQ_CAL, 0x40000002, (u32)(float)(1000.0f / src_clock_period));
			}
		}

		if (!needs_ca_or_cavref_training)
			_ccfifo_write(EMC_MRW3, ((mr13_flip_fspop ^ 0xC0) & 0xF3FFFFF7) | 0xC000000, 0);

		_ccfifo_write(EMC_SELF_REF, 0x100, 0);
	}

	if (dram_type != DRAM_TYPE_LPDDR4)
	{
		// Step 17 - MANSR exit self refresh.
		EPRINTF("Step 17");
		_ccfifo_write(EMC_SELF_REF, 0, 0);

		if (dram_type != DRAM_TYPE_LPDDR2)
		{
			if (dram_type == DRAM_TYPE_DDR3)
			{
				if (opt_dll_mode)
					_ccfifo_write(EMC_EMRS, dst_emc_entry->emc_emrs & 0xFBFFFFFF, 0);

				_ccfifo_write(EMC_EMRS2, dst_emc_entry->emc_emrs2 & 0xFBFFFFFF, 0);
				_ccfifo_write(EMC_MRS, dst_emc_entry->emc_mrs | 0x4000000, 0);

				if (opt_zcal_en_cc)
				{
					_ccfifo_write(EMC_ZQ_CAL, 0x80000001, 0);
					if (dram_dev_num == TWO_RANK)
						_ccfifo_write(EMC_ZQ_CAL, 0x40000001, 0);
				}
			}

			if (dram_type == DRAM_TYPE_LPDDR2 && opt_zcal_en_cc)
			{
				_ccfifo_write(EMC_MRW, 0x880A0056, 0);
				if (dram_dev_num == TWO_RANK)
					_ccfifo_write(EMC_MRW, 0x480A0056, 0);
			}

			goto step_19_2;
		}

		// Step 18 - Send MRWs to LPDDR3/DDR3.
		EPRINTF("Step 18");
		_ccfifo_write(EMC_MRW2, dst_emc_entry->emc_mrw2, 0);
		_ccfifo_write(EMC_MRW, dst_emc_entry->emc_mrw, 0);
		if (is_lpddr3_dram)
			_ccfifo_write(EMC_MRW4, dst_emc_entry->emc_mrw4, 0);

		// Step 19 - ZQCAL for LPDDR3/DDR3.
		EPRINTF("Step 19");
		if (opt_zcal_en_cc)
		{
			u32 zcal_wait_time_clocks = _fceil(90.0f / dst_clock_period);
			_ccfifo_write(EMC_MRS_WAIT_CNT2, ((zcal_wait_time_clocks & 0xB) << 16) | (zcal_wait_time_clocks & 0x3FF), 0); //WTFF
			_ccfifo_write(EMC_MRW, 0x880A0056, 0);
			if (dram_dev_num == TWO_RANK)
				_ccfifo_write(EMC_MRW, 0x480A0056, 0);
		}
	}

step_19_2:
	// Step 19.2.
	EPRINTF("Step 19.2");
	if (bg_regulator_mode_change)
	{
		_ccfifo_write(EMC_DBG, emc_dbg_o | 2, 0);

		u32 bg_regulator_switch_complete_wait_clks = 0;
		if (needs_tristate_training)
		{
			bg_regulator_switch_complete_wait_clks = (u32)(float)(1250.0f / src_clock_period);
			_ccfifo_write(EMC_PMACRO_BG_BIAS_CTRL_0,
				src_emc_entry->burst_regs.emc_pmacro_bg_bias_ctrl_0_idx, bg_regulator_switch_complete_wait_clks);
		}
		else
		{
			if (ramp_up_wait <= 1250)
				bg_regulator_switch_complete_wait_clks = (u32)(float)((float)((s32)1250 - ramp_up_wait) / dst_clock_period);
			_ccfifo_write(EMC_PMACRO_BG_BIAS_CTRL_0,
				dst_emc_entry->burst_regs.emc_pmacro_bg_bias_ctrl_0_idx, bg_regulator_switch_complete_wait_clks);
		}

		_ccfifo_write(EMC_DBG, emc_dbg_o, 0);
	}

	// Step 20 - Issue ref and optional QRST.
	EPRINTF("Step 20");
	if (needs_tristate_training || dram_type != DRAM_TYPE_LPDDR4)
		_ccfifo_write(EMC_REF, 0, 0);

	// Step 21 - Restore ZCAL and ZCAL interval.
	EPRINTF("Step 21");
	_ccfifo_write(EMC_DBG, emc_dbg_o | 2, 0);

	if (opt_zcal_en_cc)
	{
		if (needs_tristate_training)
			_ccfifo_write(EMC_ZCAL_INTERVAL, src_emc_entry->burst_regs.emc_zcal_interval_idx, 0);
		else if (dram_type != DRAM_TYPE_LPDDR4)
			_ccfifo_write(EMC_ZCAL_INTERVAL, dst_emc_entry->burst_regs.emc_zcal_interval_idx, 0);
	}

	_ccfifo_write(EMC_CFG, dst_emc_entry->burst_regs.emc_cfg_idx & 0xEFFFFFFF, 0);

	// Step 22 - Restore EMC_CFG_PIPE_CLK.
	EPRINTF("Step 22");
	if (needs_tristate_training && dram_type == DRAM_TYPE_LPDDR4)
		_ccfifo_write(EMC_SEL_DPD_CTRL, src_emc_entry->emc_sel_dpd_ctrl, 0);
	_ccfifo_write(EMC_DBG, emc_dbg_o, 0);
	_ccfifo_write(EMC_CFG_PIPE_CLK, emc_cfg_pipe_clk_o, 0);
	if (bg_regulator_mode_change)
	{
		if (enable_bg_regulator)
			EMC(EMC_PMACRO_BG_BIAS_CTRL_0) = dst_emc_entry->burst_regs.emc_pmacro_bg_bias_ctrl_0_idx & 0xFFFFFFFB;
		else
			EMC(EMC_PMACRO_BG_BIAS_CTRL_0) = dst_emc_entry->burst_regs.emc_pmacro_bg_bias_ctrl_0_idx & 0xFFFFFFFE;
	}

	// Step 23 - Clock Change.
	EPRINTF("Step 23");
	if (needs_tristate_training)
	{
		CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_EMC_SAFE) = (u32)CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_EMC);
		_change_dll_src(src_emc_entry, (u32)CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_EMC));  //TODO
	}
	EMC(EMC_CFG_DIG_DLL) = (EMC(EMC_CFG_DIG_DLL) & 0xFFFFFF24) | 0x88;
	
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_EMC) = selected_clk_src_emc;
	if (_wait_emc_status(EMC_INTSTATUS, CLKCHANGE_COMPLETE_INT, true, 0))
		return 4; // Clkchange handshake timeout error.

	// Step 24 - Save training results.
	EPRINTF("Step 24");
	if (needs_tristate_training)
	{
		emc_dbg_val = EMC(EMC_DBG);
		EMC(EMC_DBG) |= 1;

		_save_train_results(dst_emc_entry, needs_training, dram_dev_num, channel1_enabled);

		EMC(EMC_DBG) = emc_dbg_val;
	}

	// Step 25 - Program MC updown regs.
	EPRINTF("Step 25");
	//if (dst_emc_entry->rate_khz > src_emc_entry->rate_khz) //NEW TODO
	if ((dst_emc_entry->rate_khz > src_emc_entry->rate_khz) > needs_tristate_training)
	{
		for (u32 i = 0; dst_emc_entry->num_up_down > i; i++)
			MC(la_scale_regs_mc_addr_table[i]) = dst_emc_entry->la_scale_regs[i];

		bool dual_channel = (EMC(EMC_FBIO_CFG7) >> 1) & ((EMC(EMC_FBIO_CFG7) >> 2) & 1);
		if (_timing_update(dual_channel))
			return 4;
	}

	// Step 26 - Restore ZCAL regs.
	EPRINTF("Step 26");
	if (!in_self_refresh)
	{
		EMC(EMC_DBG) = emc_dbg_o | 2;
		EMC(EMC_ZCAL_WAIT_CNT) = dst_emc_entry->burst_regs.emc_zcal_wait_cnt_idx;
		EMC(EMC_ZCAL_INTERVAL) = dst_emc_entry->burst_regs.emc_zcal_interval_idx;
		EMC(EMC_DBG) = emc_dbg_o;
	}

	// Step 27 - Restore EMC_CFG, FDPD regs.
	EPRINTF("Step 27");
	EMC(EMC_DBG) = emc_dbg_o | 2;
	EMC(EMC_CFG) = dst_emc_entry->burst_regs.emc_cfg_idx;
	EMC(EMC_DBG) = emc_dbg_o;
	EMC(EMC_FDPD_CTRL_CMD_NO_RAMP) = dst_emc_entry->emc_fdpd_ctrl_cmd_no_ramp;
	EMC(EMC_SEL_DPD_CTRL) = dst_emc_entry->emc_sel_dpd_ctrl;

	// Step 28 - Training recover.
	EPRINTF("Step 28");
	if (needs_tristate_training && dram_type == DRAM_TYPE_LPDDR4)
	{
		EMC(EMC_DBG) = emc_dbg_o | 2;
		EMC(EMC_CFG) = dst_emc_entry->burst_regs.emc_cfg_idx;
		EMC(EMC_SEL_DPD_CTRL) = dst_emc_entry->emc_sel_dpd_ctrl;
		EMC(EMC_ZCAL_WAIT_CNT) = src_emc_entry->burst_regs.emc_zcal_wait_cnt_idx;
		EMC(EMC_ZCAL_INTERVAL) = src_emc_entry->burst_regs.emc_zcal_interval_idx;
		EMC(EMC_AUTO_CAL_CONFIG2) = src_emc_entry->emc_auto_cal_config2;
		EMC(EMC_AUTO_CAL_CONFIG3) = src_emc_entry->emc_auto_cal_config3;
		EMC(EMC_AUTO_CAL_CONFIG4) = src_emc_entry->emc_auto_cal_config4;
		EMC(EMC_AUTO_CAL_CONFIG5) = src_emc_entry->emc_auto_cal_config5;
		EMC(EMC_AUTO_CAL_CONFIG6) = src_emc_entry->emc_auto_cal_config6;
		EMC(EMC_AUTO_CAL_CONFIG7) = src_emc_entry->emc_auto_cal_config7;
		EMC(EMC_AUTO_CAL_CONFIG8) = src_emc_entry->emc_auto_cal_config8;
		EMC(EMC_DBG) = emc_dbg_o;
		EMC(EMC_TR_DVFS) = dst_emc_entry->burst_regs.emc_tr_dvfs_idx & 0xFFFFFFFE;
	}

	EMC(EMC_DBG) = emc_dbg_o | 2;
	EMC(EMC_PMACRO_AUTOCAL_CFG_COMMON) = dst_emc_entry->burst_regs.emc_pmacro_autocal_cfg_common_idx;
	EMC(EMC_DBG) = emc_dbg_o;

	// Step 29 - Power fix WAR.
	EPRINTF("Step 29");
	EMC(EMC_PMACRO_CFG_PM_GLOBAL_0) = 0xFF0000;
	EMC(EMC_PMACRO_TRAINING_CTRL_0) = CH0_TRAINING_E_WRPTR;
	EMC(EMC_PMACRO_TRAINING_CTRL_1) = CH0_TRAINING_E_WRPTR;
	EMC(EMC_PMACRO_CFG_PM_GLOBAL_0) = 0;

	// Step 30 - Re-enable autocal and Restore FSP to account for switch back (training).
	EPRINTF("Step 30");
	if (needs_tristate_training)
	{
		EMC(EMC_AUTO_CAL_CONFIG) = src_emc_entry->emc_auto_cal_config;
		fsp_for_src_freq = !fsp_for_src_freq;
	}
	else
	{
		if (dst_emc_entry->burst_regs.emc_cfg_dig_dll_idx & 1)
			_digital_dll_enable_rs(channel1_enabled);
		EMC(EMC_AUTO_CAL_CONFIG) = dst_emc_entry->emc_auto_cal_config;
	}

	return 0;
}

static void _minerva_train_patterns(emc_table_t *src_emc_entry, emc_table_t *dst_emc_entry, bool switch_rate, u32 selected_clk_src_emc)
{
	u32 needs_training_idx = 0;
	u32 emc_cfg_dig_dll_val = 0;
	u32 needs_training_emc_table[8] = {0};

	u32 needs_training = dst_emc_entry->needs_training;
	u32 dram_type = dst_emc_entry->burst_regs.emc_fbio_cfg5_idx & 3;
	bool dual_channel = (EMC(EMC_FBIO_CFG7) >> 1) & ((EMC(EMC_FBIO_CFG7) >> 2) & 1);

	 // Must start as true.
	if (train_ram_patterns)
	{
		u32 train_table_off = dst_emc_entry->training_pattern * 256;
		for (u32 i = 0; i < 256; i++)
		{
			EMC(EMC_TRAINING_PATRAM_DQ) = ram_pattern_dq_table[train_table_off + i];
			EMC(EMC_TRAINING_PATRAM_DMI) = ram_pattern_dmi_table[train_table_off + i] & 0xF;
			EMC(EMC_TRAINING_PATRAM_CTRL) = 0x80000000 + i;
		}
		train_ram_patterns = false;
	}

	if (needs_training && !dst_emc_entry->trained)
	{
		needs_training_idx = needs_training & 3;

		if (needs_training & 3)
		{
			needs_training_idx = 1;
			needs_training_emc_table[0] = needs_training & 0x203;
			if (MC(MC_EMEM_ADR_CFG) & 1) // if mapping W8 (1KB page).
			{
				needs_training_idx = 2;
				needs_training_emc_table[1] = needs_training & 0x303;
			}
		}

		if (MC(MC_EMEM_ADR_CFG) & 1 && needs_training & 0xC)
		{
			needs_training_emc_table[needs_training_idx] = needs_training & 0x20C;
			needs_training_emc_table[needs_training_idx + 1] = needs_training & 0x204;
			needs_training_idx += 2;
		}
		else if (needs_training & 0xC)
			needs_training_emc_table[needs_training_idx++] = needs_training & 0x20C;

		if (needs_training & 0xF0)
			needs_training_emc_table[needs_training_idx++] = needs_training & 0x2F0;

		for (u32 i = 0; needs_training_idx > i; i++) // Runs more than once for needs_training & 0xF
		{
			_minerva_set_clock(src_emc_entry, dst_emc_entry, needs_training_emc_table[i], selected_clk_src_emc);

			EMC(EMC_DBG) = (EMC(EMC_DBG) & 0xF3FFFFFF) | 0x8000000;
			EMC(EMC_CFG_UPDATE) = (EMC(EMC_CFG_UPDATE) & 0xFFFFFFF9) | 4;
			_timing_update(dual_channel);

			EMC(EMC_CFG_UPDATE) &= 0xFFFFFFF9;
			EMC(EMC_DBG) &= 0xF3FFFFFF;
			EMC(EMC_CFG_DIG_DLL) = (EMC(EMC_CFG_DIG_DLL) & 0xFFFFFF3E) | 0x80;
			_timing_update(dual_channel);

			emc_cfg_dig_dll_val = EMC(EMC_CFG_DIG_DLL) & 0xFFFFFFFE;
			if (dst_emc_entry->burst_regs.emc_cfg_dig_dll_idx == 1)
				emc_cfg_dig_dll_val = EMC(EMC_CFG_DIG_DLL) | 1;
			EMC(EMC_CFG_DIG_DLL) = (emc_cfg_dig_dll_val & 0xFFFFFF3F) | 0x80;
			_timing_update(dual_channel);

			while (!(EMC(EMC_DIG_DLL_STATUS) & 0x8000))
				;

			// Bug 200024907.
			if (dram_type == DRAM_TYPE_LPDDR4)
			{
				EMC(EMC_RP) = src_emc_entry->burst_regs.emc_rp_idx;
				EMC(EMC_R2P) = src_emc_entry->burst_regs.emc_r2p_idx;
				EMC(EMC_W2P) = src_emc_entry->burst_regs.emc_w2p_idx;
				EMC(EMC_TRPAB) = src_emc_entry->burst_regs.emc_trpab_idx;
			}
			_timing_update(dual_channel);

		}
		dst_emc_entry->trained = 1;
		EPRINTF("Trained");
	}

	if (switch_rate)
		_minerva_set_clock(src_emc_entry, dst_emc_entry, 0, selected_clk_src_emc);
}

u32 _minerva_do_periodic_compensation(emc_table_t *mtc_table_entry)
{
	if (mtc_table_entry && mtc_table_entry->periodic_training)
	{
		u32 val = 0;
		s32  dram_dev_num = (MC(MC_EMEM_ADR_CFG) & 1) + 1;
		bool channel1_enabled = (mtc_table_entry->burst_regs.emc_fbio_cfg7_idx >> 2) & 1;

		//u32 emc_dbg_o = EMC(EMC_DBG);
		u32 emc_cfg_o = EMC(EMC_CFG);
		u32 emc_cfg = emc_cfg_o & 0xFFFFFFF;

		// Step 1 - Disable other power features.
		EMC(EMC_CFG) = emc_cfg;

		_digital_dll_disable();

		if (dram_dev_num == TWO_RANK)
		{
			_wait_emc_status(EMC_EMC_STATUS, IN_POWERDOWN_MASK, 0, 0);
			if (channel1_enabled)
				_wait_emc_status(EMC_EMC_STATUS, IN_POWERDOWN_MASK, 0, channel1_enabled);
		}
		else
		{
			_wait_emc_status(EMC_EMC_STATUS, 0x10, 0, 0);
			if (channel1_enabled)
				_wait_emc_status(EMC_EMC_STATUS, 0x10, 0, channel1_enabled);
		}

		_wait_emc_status(EMC_EMC_STATUS, IN_SELF_REFRESH_MASK, 0, 0);
		if (channel1_enabled)
			_wait_emc_status(EMC_EMC_STATUS, IN_SELF_REFRESH_MASK, 0, channel1_enabled);

		_wait_emc_status(EMC_EMC_STATUS, REQ_FIFO_EMPTY, 0, 0); //v1.6
		if (channel1_enabled)
			_wait_emc_status(EMC_EMC_STATUS, REQ_FIFO_EMPTY, 0, channel1_enabled); //v1.6

		u32 emc_cfg_update = EMC(EMC_CFG_UPDATE);
		EMC(EMC_CFG_UPDATE) = (emc_cfg_update & 0xFFFFF9FF) | 0x400;

		// Step 2 - Osc kick off - this assumes training and dvfs have set correct MR23.
		_start_periodic_compensation();

		// Step 3 - Let dram capture its clock tree delays.
		_usleep(1000 * _actual_osc_clocks(mtc_table_entry->run_clocks) / mtc_table_entry->rate_khz + 1);

		// Step 4 - Check delta wrt previous values (save value if margin exceeds what is set in table).
		u32 adel = _minerva_update_clock_tree_delay(mtc_table_entry, mtc_table_entry, dram_dev_num, channel1_enabled, PERIODIC_TRAINING_UPDATE);

		// Step 5 - Apply compensation w.r.t. trained values (if clock tree has drifted more than the set margin).
		if (adel && ((mtc_table_entry->rate_khz / 1000) << 7) * adel / 1000000 > mtc_table_entry->tree_margin)
		{
			for (u32 i = 0; i < 10; i++)
			{
				val = _minerva_apply_periodic_compensation_trimmer(mtc_table_entry, periodic_training_addr[i]);
				EMC(periodic_training_addr[i]) = val;
			}
		}

		EMC(EMC_CFG) = emc_cfg_o;

		// Step 6 - Timing update to apply the new trimmers.
		_timing_update(channel1_enabled);

		// Step 6.1 - Restore the UPDATE_DLL_IN_UPDATE field.
		EMC(EMC_CFG_UPDATE) = emc_cfg_update;

		// Step 6.2 - Restore the DLL.
		_digital_dll_enable(channel1_enabled);
	}

	return 0;
}

s32 _minerva_set_rate(mtc_config_t *mtc_cfg)
{
	s32 src_emc_entry_idx = 0;
	s32 dst_emc_entry_idx = 999;
	s32 table_entry_rate;
	u32 selected_clk_src_emc;
	u32 selected_emc_2x_clk_src;
	bool freq_changed = false;
	emc_table_t *src_emc_entry;
	emc_table_t *dst_emc_entry;

	for (s32 i = 0; i < mtc_cfg->table_entries; i++)
	{
		table_entry_rate = mtc_cfg->mtc_table[i].rate_khz;
		if (mtc_cfg->rate_from == table_entry_rate)
			src_emc_entry_idx = i;
		if (mtc_cfg->rate_to == table_entry_rate)
			dst_emc_entry_idx = i;
	}
	src_emc_entry = (emc_table_t *)&mtc_cfg->mtc_table[src_emc_entry_idx];
	dst_emc_entry = (emc_table_t *)&mtc_cfg->mtc_table[dst_emc_entry_idx];
	s32 src_rate_khz = src_emc_entry->rate_khz;
	s32 dst_rate_khz = dst_emc_entry->rate_khz;
	u32 src_clk_src_emc = src_emc_entry->clk_src_emc;
	u32 dst_clk_src_emc = dst_emc_entry->clk_src_emc;
	if (mtc_cfg->table_entries > 900)
		return 4;

	freq_changed = _check_freq_changed(dst_rate_khz, dst_clk_src_emc, src_rate_khz, src_clk_src_emc);
	EPRINTFARGS("Requested freq change from %d to %d.", src_rate_khz, dst_rate_khz);

	if (freq_changed)
	{
		selected_emc_2x_clk_src = src_clk_src_emc >> EMC_2X_CLK_SRC_SHIFT;
		if (selected_emc_2x_clk_src & 3)
		{
			if (selected_emc_2x_clk_src - PLLMB_UD <= 1)
				emc_2X_clk_src_is_pllmb = 0;
		}
		else
		{
			emc_2X_clk_src_is_pllmb = !emc_2X_clk_src_is_pllmb;
		}
		selected_clk_src_emc = _pllm_clk_base_cfg(dst_rate_khz, 38400, dst_clk_src_emc, emc_2X_clk_src_is_pllmb);
	}
	else
	{
		selected_clk_src_emc = dst_clk_src_emc;
		selected_emc_2x_clk_src = selected_clk_src_emc >> EMC_2X_CLK_SRC_SHIFT;
		if (selected_emc_2x_clk_src != PLLMB_OUT0 && selected_emc_2x_clk_src)
		{
			if (selected_emc_2x_clk_src - PLLM_UD <= PLLC_OUT0 && emc_2X_clk_src_is_pllmb)
				selected_clk_src_emc = (selected_clk_src_emc & 0x1FFFFFFF) | (PLLMB_UD << EMC_2X_CLK_SRC_SHIFT);
		}
		else if (emc_2X_clk_src_is_pllmb)
		{
			selected_clk_src_emc = (selected_clk_src_emc & 0x1FFFFFFF) | (PLLMB_OUT0 << EMC_2X_CLK_SRC_SHIFT);
		}
	}

	switch (mtc_cfg->train_mode)
	{
	case OP_SWITCH:
		_minerva_set_clock(src_emc_entry, dst_emc_entry, 0, selected_clk_src_emc);
		mtc_cfg->current_emc_table = dst_emc_entry;
		mtc_cfg->rate_from = dst_emc_entry->rate_khz;
		return 0;
	case OP_TRAIN:
		_minerva_train_patterns(src_emc_entry, dst_emc_entry, false, selected_clk_src_emc);
		if (freq_changed)
			emc_2X_clk_src_is_pllmb = !emc_2X_clk_src_is_pllmb;
		return 0;
	case OP_TRAIN_SWITCH:
		_minerva_train_patterns(src_emc_entry, dst_emc_entry, true, selected_clk_src_emc);
		mtc_cfg->current_emc_table = dst_emc_entry;
		mtc_cfg->rate_from = dst_emc_entry->rate_khz;
		return 0;
	default:
		return 4;
	}
}

void _minerva_get_table(mtc_config_t *mtc_cfg)
{
	switch (mtc_cfg->sdram_id)
	{
	case 1:
		memcpy((void *)MTC_TABLE, nx_abca2_2_10NoCfgVersion_V9_8_7_V1_6, 49280);
		break;
	case 0:
	case 2:
	case 3:
	case 4:
	default:
		memcpy((void *)MTC_TABLE, nx_abca2_0_3_10NoCfgVersion_V9_8_7_V1_6, 49280);
		break;
	}

	mtc_cfg->mtc_table = (emc_table_t *)MTC_TABLE;

	mtc_cfg->table_entries = 10;
	mtc_cfg->rate_to = 0;
	mtc_cfg->rate_from = 0;
	mtc_cfg->train_mode = 0;
	mtc_cfg->current_emc_table = NULL;

	// Important!
	mtc_cfg->emc_2X_clk_src_is_pllmb = false;
	mtc_cfg->fsp_for_src_freq = false;
	mtc_cfg->train_ram_patterns = true;
}

void _minerva_init(mtc_config_t *mtc_cfg, void* bp)
{
	EPRINTF("-- Minerva Training Cell --");
	
	train_ram_patterns = mtc_cfg->train_ram_patterns;
	fsp_for_src_freq = mtc_cfg->fsp_for_src_freq;
	emc_2X_clk_src_is_pllmb = mtc_cfg->emc_2X_clk_src_is_pllmb;

	if (!mtc_cfg->mtc_table)
	{
		_minerva_get_table(mtc_cfg);
		return;
	}

	switch (mtc_cfg->train_mode)
	{
	case OP_SWITCH:
		EPRINTF("Switching..");
		break;
	case OP_TRAIN:
		EPRINTF("Training..");
		break;
	case OP_TRAIN_SWITCH:
		EPRINTF("Training and switching..");
		break;
	case OP_PERIODIC_TRAIN:
		EPRINTF("periodic training..");
		break;
	}

	if (mtc_cfg->train_mode != OP_PERIODIC_TRAIN)
		_minerva_set_rate(mtc_cfg);
	else
		_minerva_do_periodic_compensation(mtc_cfg->current_emc_table);

	mtc_cfg->train_ram_patterns = train_ram_patterns;
	mtc_cfg->fsp_for_src_freq = fsp_for_src_freq;
	mtc_cfg->emc_2X_clk_src_is_pllmb = emc_2X_clk_src_is_pllmb;
}
