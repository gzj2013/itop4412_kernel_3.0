/* linux/drivers/media/video/samsung/fimg2d4x/fimg2d4x_hw.c
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *	http://www.samsung.com/
 *
 * Samsung Graphics 2D driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/io.h>
#include <linux/sched.h>

#include "fimg2d.h"
#include "fimg2d4x.h"
#include "fimg2d_clk.h"

#undef SOFT_RESET_ENABLED
#undef FIMG2D_RESET_WA

static const int a8_rgbcolor		= (int)0x0;
static const int msk_oprmode		= (int)MSK_ARGB;
static const int alpha_oprmode		= (int)ALPHA_PERPIXEL_MUL_GLOBAL;
static const int premult_round_mode	= (int)PREMULT_ROUND_1;	/* (A+1)*B) >> 8 */
static const int blend_round_mode	= (int)BLEND_ROUND_0;	/* (A+1)*B) >> 8 */

void fimg2d4x_reset(struct fimg2d_control *info)
{
#ifdef SOFT_RESET_ENABLED
#ifdef FIMG2D_RESET_WA
	fimg2d_clk_save(info);
#endif
	writel(FIMG2D_SOFT_RESET, info->regs + FIMG2D_SOFT_RESET_REG);
#ifdef FIMG2D_RESET_WA
	fimg2d_clk_restore(info);
#endif
#else
	writel(FIMG2D_SFR_CLEAR, info->regs + FIMG2D_SOFT_RESET_REG);
#endif
	/* remove wince option */
	writel(0x0, info->regs + FIMG2D_BLEND_FUNCTION_REG);
}

void fimg2d4x_enable_irq(struct fimg2d_control *info)
{
	writel(FIMG2D_BLIT_INT_ENABLE, info->regs + FIMG2D_INTEN_REG);
}

void fimg2d4x_disable_irq(struct fimg2d_control *info)
{
	writel(0, info->regs + FIMG2D_INTEN_REG);
}

void fimg2d4x_clear_irq(struct fimg2d_control *info)
{
	writel(FIMG2D_BLIT_INT_FLAG, info->regs + FIMG2D_INTC_PEND_REG);
}

int fimg2d4x_is_blit_done(struct fimg2d_control *info)
{
	return readl(info->regs + FIMG2D_INTC_PEND_REG) & FIMG2D_BLIT_INT_FLAG;
}

int fimg2d4x_blit_done_status(struct fimg2d_control *info)
{
	volatile unsigned long sts;

	/* read twice */
	sts = readl(info->regs + FIMG2D_FIFO_STAT_REG);
	sts = readl(info->regs + FIMG2D_FIFO_STAT_REG);

	return (int)(sts & FIMG2D_BLIT_FINISHED);
}

void fimg2d4x_start_blit(struct fimg2d_control *info)
{
	writel(FIMG2D_START_BITBLT, info->regs + FIMG2D_BITBLT_START_REG);
}

void fimg2d4x_set_max_burst_length(struct fimg2d_control *info, enum max_burst_len len)
{
	unsigned long cfg;

	cfg = readl(info->regs + FIMG2D_AXI_MODE_REG);

	cfg &= ~FIMG2D_MAX_BURST_LEN_MASK;
	cfg |= len << FIMG2D_MAX_BURST_LEN_SHIFT;
}

void fimg2d4x_set_src_type(struct fimg2d_control *info, enum image_sel type)
{
	unsigned long cfg;

	if (type == IMG_MEMORY)
		cfg = FIMG2D_IMAGE_TYPE_MEMORY;
	else if (type == IMG_FGCOLOR)
		cfg = FIMG2D_IMAGE_TYPE_FGCOLOR;
	else
		cfg = FIMG2D_IMAGE_TYPE_BGCOLOR;

	writel(cfg, info->regs + FIMG2D_SRC_SELECT_REG);
}

void fimg2d4x_set_src_image(struct fimg2d_control *info, struct fimg2d_image *s)
{
	unsigned long cfg;

	writel(FIMG2D_ADDR(s->addr.start), info->regs + FIMG2D_SRC_BASE_ADDR_REG);
	writel(FIMG2D_STRIDE(s->stride), info->regs + FIMG2D_SRC_STRIDE_REG);

	if (s->order < ARGB_ORDER_END) {	/* argb */
		cfg = s->order << FIMG2D_RGB_ORDER_SHIFT;
		if (s->fmt == CF_A8)
			writel(a8_rgbcolor, info->regs + FIMG2D_SRC_A8_RGB_EXT_REG);
	} else if (s->order < P1_ORDER_END) {	/* YCbC1 1plane */
		cfg = (s->order - P1_CRY1CBY0) << FIMG2D_YCBCR_ORDER_SHIFT;
	} else {	/* YCbCr 2plane */
		cfg = (s->order - P2_CRCB) << FIMG2D_YCBCR_ORDER_SHIFT;
		cfg |= FIMG2D_YCBCR_2PLANE;

		writel(FIMG2D_ADDR(s->plane2.start),
				info->regs + FIMG2D_SRC_PLANE2_BASE_ADDR_REG);
	}

	cfg |= s->fmt << FIMG2D_COLOR_FORMAT_SHIFT;

	writel(cfg, info->regs + FIMG2D_SRC_COLOR_MODE_REG);
}

void fimg2d4x_set_src_rect(struct fimg2d_control *info, struct fimg2d_rect *r)
{
	writel(FIMG2D_OFFSET(r->x1, r->y1), info->regs + FIMG2D_SRC_LEFT_TOP_REG);
	writel(FIMG2D_OFFSET(r->x2, r->y2), info->regs + FIMG2D_SRC_RIGHT_BOTTOM_REG);
}

void fimg2d4x_set_dst_type(struct fimg2d_control *info, enum image_sel type)
{
	unsigned long cfg;

	if (type == IMG_MEMORY)
		cfg = FIMG2D_IMAGE_TYPE_MEMORY;
	else if (type == IMG_FGCOLOR)
		cfg = FIMG2D_IMAGE_TYPE_FGCOLOR;
	else
		cfg = FIMG2D_IMAGE_TYPE_BGCOLOR;

	writel(cfg, info->regs + FIMG2D_DST_SELECT_REG);
}

/**
 * @d: set base address, stride, color format, order
*/
void fimg2d4x_set_dst_image(struct fimg2d_control *info, struct fimg2d_image *d)
{
	unsigned long cfg;

	writel(FIMG2D_ADDR(d->addr.start), info->regs + FIMG2D_DST_BASE_ADDR_REG);
	writel(FIMG2D_STRIDE(d->stride), info->regs + FIMG2D_DST_STRIDE_REG);

	if (d->order < ARGB_ORDER_END) {
		cfg = d->order << FIMG2D_RGB_ORDER_SHIFT;
		if (d->fmt == CF_A8)
			writel(a8_rgbcolor, info->regs + FIMG2D_DST_A8_RGB_EXT_REG);
	} else if (d->order < P1_ORDER_END) {
		cfg = (d->order - P1_CRY1CBY0) << FIMG2D_YCBCR_ORDER_SHIFT;
	} else {
		cfg = (d->order - P2_CRCB) << FIMG2D_YCBCR_ORDER_SHIFT;
		cfg |= FIMG2D_YCBCR_2PLANE;

		writel(FIMG2D_ADDR(d->plane2.start),
				info->regs + FIMG2D_DST_PLANE2_BASE_ADDR_REG);
	}

	cfg |= d->fmt << FIMG2D_COLOR_FORMAT_SHIFT;

	writel(cfg, info->regs + FIMG2D_DST_COLOR_MODE_REG);
}

void fimg2d4x_set_dst_rect(struct fimg2d_control *info, struct fimg2d_rect *r)
{
	writel(FIMG2D_OFFSET(r->x1, r->y1), info->regs + FIMG2D_DST_LEFT_TOP_REG);
	writel(FIMG2D_OFFSET(r->x2, r->y2), info->regs + FIMG2D_DST_RIGHT_BOTTOM_REG);
}

void fimg2d4x_enable_msk(struct fimg2d_control *info)
{
	unsigned long cfg;

	cfg = readl(info->regs + FIMG2D_BITBLT_COMMAND_REG);
	cfg |= FIMG2D_ENABLE_NORMAL_MSK;

	writel(cfg, info->regs + FIMG2D_BITBLT_COMMAND_REG);
}

void fimg2d4x_set_msk_image(struct fimg2d_control *info, struct fimg2d_image *m)
{
	unsigned long cfg;

	writel(FIMG2D_ADDR(m->addr.start), info->regs + FIMG2D_MSK_BASE_ADDR_REG);
	writel(FIMG2D_STRIDE(m->stride), info->regs + FIMG2D_MSK_STRIDE_REG);

	cfg = m->order << FIMG2D_MSK_ORDER_SHIFT;
	cfg |= (m->fmt - CF_MSK_1BIT) << FIMG2D_MSK_FORMAT_SHIFT;

	/* 16, 32bit mask only */
	if (m->fmt >= CF_MSK_16BIT_565) {
		if (msk_oprmode == MSK_ALPHA)
			cfg |= FIMG2D_MSK_TYPE_ALPHA;
		else if (msk_oprmode == MSK_ARGB)
			cfg |= FIMG2D_MSK_TYPE_ARGB;
		else
			cfg |= FIMG2D_MSK_TYPE_MIXED;
	}

	writel(cfg, info->regs + FIMG2D_MSK_MODE_REG);
}

void fimg2d4x_set_msk_rect(struct fimg2d_control *info, struct fimg2d_rect *r)
{
	writel(FIMG2D_OFFSET(r->x1, r->y1), info->regs + FIMG2D_MSK_LEFT_TOP_REG);
	writel(FIMG2D_OFFSET(r->x2, r->y2), info->regs + FIMG2D_MSK_RIGHT_BOTTOM_REG);
}

/**
 * If solid color fill is enabled, other blit command is ignored.
 * Color format of solid color is considered to be
 *	the same as destination color format
 * Channel order of solid color is A-R-G-B or Y-Cb-Cr
 */
void fimg2d4x_set_color_fill(struct fimg2d_control *info, unsigned long color)
{
	writel(FIMG2D_SOLID_FILL, info->regs + FIMG2D_BITBLT_COMMAND_REG);

	/* sf color */
	writel(color, info->regs + FIMG2D_SF_COLOR_REG);
}

/**
 * set alpha-multiply mode for src, dst, pat read (pre-bitblt)
 * set alpha-demultiply for dst write (post-bitblt)
 */
void fimg2d4x_set_premultiplied(struct fimg2d_control *info)
{
	unsigned long cfg;

	cfg = readl(info->regs + FIMG2D_BITBLT_COMMAND_REG);
	cfg |= FIMG2D_PREMULT_ALL;

	writel(cfg, info->regs + FIMG2D_BITBLT_COMMAND_REG);
}

void fimg2d4x_src_premultiply(struct fimg2d_control *info)
{
	unsigned long cfg;

	cfg = readl(info->regs + FIMG2D_BITBLT_COMMAND_REG);
	cfg |= FIMG2D_SRC_PREMULT;

	writel(cfg, info->regs + FIMG2D_BITBLT_COMMAND_REG);
}

void fimg2d4x_dst_premultiply(struct fimg2d_control *info)
{
	unsigned long cfg;

	cfg = readl(info->regs + FIMG2D_BITBLT_COMMAND_REG);
	cfg |= FIMG2D_DST_RD_PREMULT;

	writel(cfg, info->regs + FIMG2D_BITBLT_COMMAND_REG);
}

void fimg2d4x_dst_depremultiply(struct fimg2d_control *info)
{
	unsigned long cfg;

	cfg = readl(info->regs + FIMG2D_BITBLT_COMMAND_REG);
	cfg |= FIMG2D_DST_WR_DEPREMULT;

	writel(cfg, info->regs + FIMG2D_BITBLT_COMMAND_REG);
}

/**
 * set transp/bluscr mode, bs color, bg color
 */
void fimg2d4x_set_bluescreen(struct fimg2d_control *info,
		struct fimg2d_bluscr *bluscr)
{
	unsigned long cfg;

	cfg = readl(info->regs + FIMG2D_BITBLT_COMMAND_REG);

	if (bluscr->mode == TRANSP)
		cfg |= FIMG2D_TRANSP_MODE;
	else if (bluscr->mode == BLUSCR)
		cfg |= FIMG2D_BLUSCR_MODE;
	else	/* opaque: initial value */
		return;

	writel(cfg, info->regs + FIMG2D_BITBLT_COMMAND_REG);

	/* bs color */
	if (bluscr->bs_color)
		writel(bluscr->bs_color, info->regs + FIMG2D_BS_COLOR_REG);

	/* bg color */
	if (bluscr->mode == BLUSCR && bluscr->bg_color)
		writel(bluscr->bg_color, info->regs + FIMG2D_BG_COLOR_REG);
}

/**
 * @c: destination clipping region
 */
void fimg2d4x_enable_clipping(struct fimg2d_control *info, struct fimg2d_clip *c)
{
	unsigned long cfg;

	cfg = readl(info->regs + FIMG2D_BITBLT_COMMAND_REG);
	cfg |= FIMG2D_ENABLE_CW;

	writel(cfg, info->regs + FIMG2D_BITBLT_COMMAND_REG);

	writel(FIMG2D_OFFSET(c->x1, c->y1), info->regs + FIMG2D_CW_LT_REG);
	writel(FIMG2D_OFFSET(c->x2, c->y2), info->regs + FIMG2D_CW_RB_REG);
}

void fimg2d4x_enable_dithering(struct fimg2d_control *info)
{
	unsigned long cfg;

	cfg = readl(info->regs + FIMG2D_BITBLT_COMMAND_REG);
	cfg |= FIMG2D_ENABLE_DITHER;

	writel(cfg, info->regs + FIMG2D_BITBLT_COMMAND_REG);
}


#define MAX_PRECISION 16

/**
 * scale_factor_to_fixed16 - convert scale factor to fixed pint 16
 * @n: numerator
 * @d: denominator
 */
inline static unsigned long scale_factor_to_fixed16(int n, int d)
{
	int i;
	u32 fixed16;

	fixed16 = (n/d) << 16;
	n %= d;

	for (i = 0; i < MAX_PRECISION; i++) {
		if (!n)
			break;
		n <<= 1;
		if (n/d)
			fixed16 |= 1 << (15-i);
		n %= d;
	}

	return fixed16;
}

void fimg2d4x_set_src_scaling(struct fimg2d_control *info, struct fimg2d_scale *s)
{
	int src_w, src_h, dst_w, dst_h;
	unsigned long wcfg, hcfg;
	unsigned long mode;

	/* scaling algorithm */
	if (s->mode == SCALING_NEAREST)
		mode = FIMG2D_SCALE_MODE_NEAREST;
	else
		mode = FIMG2D_SCALE_MODE_BILINEAR;

	writel(mode, info->regs + FIMG2D_SRC_SCALE_CTRL_REG);

	if (s->factor == SCALING_PERCENTAGE) {
		/*
		 * percental scaling factor
		 * e.g scale-up: 200% --> src scaling factor: 0.5 (0x000080000)
		 * e.g scale-down: 50% --> src scaling factor: 2.0 (0x00020000)
		 */
		src_w = 100;
		src_h = 100;

		dst_w = s->scale_w;
		dst_h = s->scale_h;
	} else {
		/*
		 * pixels scaling factor
		 * e.g scale-up: src(1,1)-->dst(2,2), src scaling factor: 0.5 (0x000080000)
		 * e.g scale-down: src(2,2)-->dst(1,1), src scaling factor: 2.0 (0x000200000)
		 */
		src_w = s->src_w * 100;
		src_h = s->src_h * 100;

		dst_w = s->dst_w * 100;
		dst_h = s->dst_h * 100;
	}

	/* inversed scaling factor: src is numerator */
	wcfg = scale_factor_to_fixed16(src_w, dst_w);
	hcfg = scale_factor_to_fixed16(src_h, dst_h);

	writel(wcfg, info->regs + FIMG2D_SRC_XSCALE_REG);
	writel(hcfg, info->regs + FIMG2D_SRC_YSCALE_REG);
}

void fimg2d4x_set_msk_scaling(struct fimg2d_control *info, struct fimg2d_scale *s)
{
	int src_w, src_h, dst_w, dst_h;
	unsigned long wcfg, hcfg;
	unsigned long mode;

	/* scaling algorithm */
	if (s->mode == SCALING_NEAREST)
		mode = FIMG2D_SCALE_MODE_NEAREST;
	else
		mode = FIMG2D_SCALE_MODE_BILINEAR;

	writel(mode, info->regs + FIMG2D_MSK_SCALE_CTRL_REG);

	if (s->factor == SCALING_PERCENTAGE) {
		/*
		 * percental scaling factor
		 * e.g scale-up: 200% --> msk scaling factor: 0.5 (0x000080000)
		 * e.g scale-down: 50% --> msk scaling factor: 2.0 (0x00020000)
		 */
		src_w = 100;
		src_h = 100;

		dst_w = s->scale_w;
		dst_h = s->scale_h;
	} else {
		/*
		 * pixels scaling factor
		 * e.g scale-up: src(1,1)-->dst(2,2), msk scaling factor: 0.5 (0x000080000)
		 * e.g scale-down: src(2,2)-->dst(1,1), msk scaling factor: 2.0 (0x000200000)
		 */
		src_w = s->src_w * 100;
		src_h = s->src_h * 100;

		dst_w = s->dst_w * 100;
		dst_h = s->dst_h * 100;
	}

	/* inversed scaling factor: src is numerator */
	wcfg = scale_factor_to_fixed16(src_w, dst_w);
	hcfg = scale_factor_to_fixed16(src_h, dst_h);

	writel(wcfg, info->regs + FIMG2D_MSK_XSCALE_REG);
	writel(hcfg, info->regs + FIMG2D_MSK_YSCALE_REG);
}

void fimg2d4x_set_src_repeat(struct fimg2d_control *info, struct fimg2d_repeat *r)
{
	unsigned long cfg;

	if (r->mode == NO_REPEAT)
		cfg = FIMG2D_SRC_REPEAT_NONE;
	else
		cfg = (r->mode - REPEAT_NORMAL) << FIMG2D_SRC_REPEAT_SHIFT;

	writel(cfg, info->regs + FIMG2D_SRC_REPEAT_MODE_REG);

	/* src pad color */
	if (r->mode == REPEAT_PAD)
		writel(r->pad_color, info->regs + FIMG2D_SRC_PAD_VALUE_REG);
}

void fimg2d4x_set_msk_repeat(struct fimg2d_control *info, struct fimg2d_repeat *r)
{
	unsigned long cfg;

	if (r->mode == NO_REPEAT)
		return;

	cfg = (r->mode - REPEAT_NORMAL) << FIMG2D_MSK_REPEAT_SHIFT;

	writel(cfg, info->regs + FIMG2D_MSK_REPEAT_MODE_REG);

	/* mask pad color */
	if (r->mode == REPEAT_PAD)
		writel(r->pad_color, info->regs + FIMG2D_MSK_PAD_VALUE_REG);
}

void fimg2d4x_set_rotation(struct fimg2d_control *info, enum rotation rot)
{
	int rev_rot90;	/* counter clockwise, 4.1 specific */
	unsigned long cfg;
	enum addressing dirx, diry;

	rev_rot90 = 0;
	dirx = diry = FORWARD_ADDRESSING;

	switch (rot) {
	case ROT_90:	/* -270 degree */
		rev_rot90 = 1;	/* fall through */
	case ROT_180:
		dirx = REVERSE_ADDRESSING;
		diry = REVERSE_ADDRESSING;
		break;
	case ROT_270:	/* -90 degree */
		rev_rot90 = 1;
		break;
	case XFLIP:
		diry = REVERSE_ADDRESSING;
		break;
	case YFLIP:
		dirx = REVERSE_ADDRESSING;
		break;
	case ORIGIN:
	default:
		break;
	}

	/* destination direction */
	if (dirx == REVERSE_ADDRESSING || diry == REVERSE_ADDRESSING) {
		cfg = readl(info->regs + FIMG2D_DST_PAT_DIRECT_REG);

		if (dirx == REVERSE_ADDRESSING)
			cfg |= FIMG2D_DST_X_DIR_NEGATIVE;

		if (diry == REVERSE_ADDRESSING)
			cfg |= FIMG2D_DST_Y_DIR_NEGATIVE;

		writel(cfg, info->regs + FIMG2D_DST_PAT_DIRECT_REG);
	}

	/* rotation -90 */
	if (rev_rot90) {
		cfg = readl(info->regs + FIMG2D_ROTATE_REG);
		cfg |= FIMG2D_SRC_ROTATE_90;
		cfg |= FIMG2D_MSK_ROTATE_90;

		writel(cfg, info->regs + FIMG2D_ROTATE_REG);
	}
}

void fimg2d4x_set_fgcolor(struct fimg2d_control *info, unsigned long fg)
{
	writel(fg, info->regs + FIMG2D_FG_COLOR_REG);
		writel(fg, info->regs + FIMG2D_FG_COLOR_REG);
}

void fimg2d4x_set_bgcolor(struct fimg2d_control *info, unsigned long bg)
{
	writel(bg, info->regs + FIMG2D_BG_COLOR_REG);
		writel(bg, info->regs + FIMG2D_BG_COLOR_REG);
}

void fimg2d4x_enable_alpha(struct fimg2d_control *info, unsigned char g_alpha)
{
	unsigned long cfg;

	/* enable alpha */
	cfg = readl(info->regs + FIMG2D_BITBLT_COMMAND_REG);
	cfg |= FIMG2D_ALPHA_BLEND_MODE;

	writel(cfg, info->regs + FIMG2D_BITBLT_COMMAND_REG);

	/*
	 * global(constant) alpha
	 * ex. if global alpha is 0x80, must set 0x80808080
	 */
	cfg = g_alpha;
	cfg |= g_alpha << 8;
	cfg |= g_alpha << 16;
	cfg |= g_alpha << 24;
	writel(cfg, info->regs + FIMG2D_ALPHA_REG);
}

/**
 * Four channels of the image are computed with:
 *	R = [ coeff(S)*Sc  + coeff(D)*Dc ]
 *	where
 *	Rc is result color or alpha
 *	Sc is source color or alpha
 *	Dc is destination color or alpha
 *
 * Caution: supposed that Sc and Dc are perpixel-alpha-premultiplied value
 *
 * MODE:             Formula
 * ----------------------------------------------------------------------------
 * FILL:
 * CLEAR:	     R = 0
 * SRC:		     R = Sc
 * DST:		     R = Dc
 * SRC_OVER:         R = Sc + (1-Sa)*Dc
 * DST_OVER:         R = (1-Da)*Sc + Dc
 * SRC_IN:	     R = Da*Sc
 * DST_IN:           R = Sa*Dc
 * SRC_OUT:          R = (1-Da)*Sc
 * DST_OUT:          R = (1-Sa)*Dc
 * SRC_ATOP:         R = Da*Sc + (1-Sa)*Dc
 * DST_ATOP:         R = (1-Da)*Sc + Sa*Dc
 * XOR:              R = (1-Da)*Sc + (1-Sa)*Dc
 * ADD:              R = Sc + Dc
 * MULTIPLY:         R = Sc*Dc
 * SCREEN:           R = Sc + (1-Sc)*Dc
 * DARKEN:           R = (Da*Sc<Sa*Dc)? Sc+(1-Sa)*Dc : (1-Da)*Sc+Dc
 * LIGHTEN:          R = (Da*Sc>Sa*Dc)? Sc+(1-Sa)*Dc : (1-Da)*Sc+Dc
 * DISJ_SRC_OVER:    R = Sc + (min(1,(1-Sa)/Da))*Dc
 * DISJ_DST_OVER:    R = (min(1,(1-Da)/Sa))*Sc + Dc
 * DISJ_SRC_IN:      R = (max(1-(1-Da)/Sa,0))*Sc
 * DISJ_DST_IN:      R = (max(1-(1-Sa)/Da,0))*Dc
 * DISJ_SRC_OUT:     R = (min(1,(1-Da)/Sa))*Sc
 * DISJ_DST_OUT:     R = (min(1,(1-Sa)/Da))*Dc
 * DISJ_SRC_ATOP:    R = (max(1-(1-Da)/Sa,0))*Sc + (min(1,(1-Sa)/Da))*Dc
 * DISJ_DST_ATOP:    R = (min(1,(1-Da)/Sa))*Sc + (max(1-(1-Sa)/Da,0))*Dc
 * DISJ_XOR:         R = (min(1,(1-Da)/Sa))*Sc + (min(1,(1-Sa)/Da))*Dc
 * CONJ_SRC_OVER:    R = Sc + (max(1-Sa/Da,0))*Dc
 * CONJ_DST_OVER:    R = (max(1-Da/Sa,0))*Sc + Dc
 * CONJ_SRC_IN:      R = (min(1,Da/Sa))*Sc
 * CONJ_DST_IN:      R = (min(1,Sa/Da))*Dc
 * CONJ_SRC_OUT:     R = (max(1-Da/Sa,0)*Sc
 * CONJ_DST_OUT:     R = (max(1-Sa/Da,0))*Dc
 * CONJ_SRC_ATOP:    R = (min(1,Da/Sa))*Sc + (max(1-Sa/Da,0))*Dc
 * CONJ_DST_ATOP:    R = (max(1-Da/Sa,0))*Sc + (min(1,Sa/Da))*Dc
 * CONJ_XOR:         R = (max(1-Da/Sa,0))*Sc + (max(1-Sa/Da,0))*Dc
 */
static struct fimg2d_blend_coeff const coeff_table[MAX_FIMG2D_BLIT_OP] = {
	{ 0, 0, 0, 0 },		/* FILL */
	{ 0, COEFF_ZERO,	0, COEFF_ZERO },	/* CLEAR */
	{ 0, COEFF_ONE,		0, COEFF_ZERO },	/* SRC */
	{ 0, COEFF_ZERO,	0, COEFF_ONE },		/* DST */
	{ 0, COEFF_ONE,		1, COEFF_SA },		/* SRC_OVER */
	{ 1, COEFF_DA,		0, COEFF_ONE },		/* DST_OVER */
	{ 0, COEFF_DA,		0, COEFF_ZERO },	/* SRC_IN */
	{ 0, COEFF_ZERO,	0, COEFF_SA },		/* DST_IN */
	{ 1, COEFF_DA,		0, COEFF_ZERO },	/* SRC_OUT */
	{ 0, COEFF_ZERO,	1, COEFF_SA },		/* DST_OUT */
	{ 0, COEFF_DA,		1, COEFF_SA },		/* SRC_ATOP */
	{ 1, COEFF_DA,		0, COEFF_SA },		/* DST_ATOP */
	{ 1, COEFF_DA,		1, COEFF_SA },		/* XOR */
	{ 0, COEFF_ONE,		0, COEFF_ONE },		/* ADD */
	{ 0, COEFF_DC,		0, COEFF_ZERO },	/* MULTIPLY */
	{ 0, COEFF_ONE,		1, COEFF_SC },		/* SCREEN */
	{ 0, 0, 0, 0 },		/* DARKEN */
	{ 0, 0, 0, 0 },		/* LIGHTEN */
	{ 0, COEFF_ONE,		0, COEFF_DISJ_S },	/* DISJ_SRC_OVER */
	{ 0, COEFF_DISJ_D,	0, COEFF_ONE },		/* DISJ_DST_OVER */
	{ 1, COEFF_DISJ_D,	0, COEFF_ZERO },	/* DISJ_SRC_IN */
	{ 0, COEFF_ZERO,	1, COEFF_DISJ_S },	/* DISJ_DST_IN */
	{ 0, COEFF_DISJ_D,	0, COEFF_ONE },		/* DISJ_SRC_OUT */
	{ 0, COEFF_ZERO,	0, COEFF_DISJ_S },	/* DISJ_DST_OUT */
	{ 1, COEFF_DISJ_D,	0, COEFF_DISJ_S },	/* DISJ_SRC_ATOP */
	{ 0, COEFF_DISJ_D,	1, COEFF_DISJ_S },	/* DISJ_DST_ATOP */
	{ 0, COEFF_DISJ_D,	0, COEFF_DISJ_S },	/* DISJ_XOR */
	{ 0, COEFF_ONE,		1, COEFF_DISJ_S },	/* CONJ_SRC_OVER */
	{ 1, COEFF_DISJ_D,	0, COEFF_ONE },		/* CONJ_DST_OVER */
	{ 0, COEFF_CONJ_D,	0, COEFF_ONE },		/* CONJ_SRC_IN */
	{ 0, COEFF_ZERO,	0, COEFF_CONJ_S },	/* CONJ_DST_IN */
	{ 1, COEFF_CONJ_D,	0, COEFF_ZERO },	/* CONJ_SRC_OUT */
	{ 0, COEFF_ZERO,	1, COEFF_CONJ_S },	/* CONJ_DST_OUT */
	{ 0, COEFF_CONJ_D,	1, COEFF_CONJ_S },	/* CONJ_SRC_ATOP */
	{ 1, COEFF_CONJ_D,	0, COEFF_CONJ_D },	/* CONJ_DST_ATOP */
	{ 1, COEFF_CONJ_D,	1, COEFF_CONJ_S },	/* CONJ_XOR */
	{ 0, 0, 0, 0 },		/* USER */
	{ 1, COEFF_GA,		1, COEFF_ZERO },	/* USER_SRC_GA */
};

/*
 * coefficient table with global (constant) alpha
 * replace COEFF_ONE with COEFF_GA
 *
 * MODE:             Formula with Global Alpha (Ga is multiplied to both Sc and Sa)
 * ----------------------------------------------------------------------------
 * FILL:
 * CLEAR:	     R = 0
 * SRC:		     R = Ga*Sc
 * DST:		     R = Dc
 * SRC_OVER:         R = Ga*Sc + (1-Sa*Ga)*Dc
 * DST_OVER:         R = (1-Da)*Ga*Sc + Dc --> (W/A) 1st:Ga*Sc, 2nd:DST_OVER
 * SRC_IN:	     R = Da*Ga*Sc
 * DST_IN:           R = Sa*Ga*Dc
 * SRC_OUT:          R = (1-Da)*Ga*Sc --> (W/A) 1st: Ga*Sc, 2nd:SRC_OUT
 * DST_OUT:          R = (1-Sa*Ga)*Dc
 * SRC_ATOP:         R = Da*Ga*Sc + (1-Sa*Ga)*Dc
 * DST_ATOP:         R = (1-Da)*Ga*Sc + Sa*Ga*Dc --> (W/A) 1st: Ga*Sc, 2nd:DST_ATOP
 * XOR:              R = (1-Da)*Ga*Sc + (1-Sa*Ga)*Dc --> (W/A) 1st: Ga*Sc, 2nd:XOR
 * ADD:              R = Ga*Sc + Dc
 * MULTIPLY:         R = Ga*Sc*Dc --> (W/A) 1st: Ga*Sc, 2nd: MULTIPLY
 * SCREEN:           R = Ga*Sc + (1-Ga*Sc)*Dc --> (W/A) 1st: Ga*Sc, 2nd: SCREEN
 * DARKEN:           R = (W/A) 1st: Ga*Sc, 2nd: OP
 * LIGHTEN:          R = (W/A) 1st: Ga*Sc, 2nd: OP
 * DISJ_SRC_OVER:    R = (W/A) 1st: Ga*Sc, 2nd: OP
 * DISJ_DST_OVER:    R = (W/A) 1st: Ga*Sc, 2nd: OP
 * DISJ_SRC_IN:      R = (W/A) 1st: Ga*Sc, 2nd: OP
 * DISJ_DST_IN:      R = (W/A) 1st: Ga*Sc, 2nd: OP
 * DISJ_SRC_OUT:     R = (W/A) 1st: Ga*Sc, 2nd: OP
 * DISJ_DST_OUT:     R = (W/A) 1st: Ga*Sc, 2nd: OP
 * DISJ_SRC_ATOP:    R = (W/A) 1st: Ga*Sc, 2nd: OP
 * DISJ_DST_ATOP:    R = (W/A) 1st: Ga*Sc, 2nd: OP
 * DISJ_XOR:         R = (W/A) 1st: Ga*Sc, 2nd: OP
 * CONJ_SRC_OVER:    R = (W/A) 1st: Ga*Sc, 2nd: OP
 * CONJ_DST_OVER:    R = (W/A) 1st: Ga*Sc, 2nd: OP
 * CONJ_SRC_IN:      R = (W/A) 1st: Ga*Sc, 2nd: OP
 * CONJ_DST_IN:      R = (W/A) 1st: Ga*Sc, 2nd: OP
 * CONJ_SRC_OUT:     R = (W/A) 1st: Ga*Sc, 2nd: OP
 * CONJ_DST_OUT:     R = (W/A) 1st: Ga*Sc, 2nd: OP
 * CONJ_SRC_ATOP:    R = (W/A) 1st: Ga*Sc, 2nd: OP
 * CONJ_DST_ATOP:    R = (W/A) 1st: Ga*Sc, 2nd: OP
 * CONJ_XOR:         R = (W/A) 1st: Ga*Sc, 2nd: OP
 */
static struct fimg2d_blend_coeff const ga_coeff_table[MAX_FIMG2D_BLIT_OP] = {
	{ 0, 0, 0, 0 },		/* FILL */
	{ 0, COEFF_ZERO,	0, COEFF_ZERO },	/* CLEAR */
	{ 0, COEFF_GA,		0, COEFF_ZERO },	/* SRC */
	{ 0, COEFF_ZERO,	0, COEFF_ONE },		/* DST */
	{ 0, COEFF_GA,		1, COEFF_SA },		/* SRC_OVER */
	{ 1, COEFF_DA,		0, COEFF_ONE },		/* DST_OVER (use W/A) */
	{ 0, COEFF_DA,		0, COEFF_ZERO },	/* SRC_IN */
	{ 0, COEFF_ZERO,	0, COEFF_SA },		/* DST_IN */
	{ 1, COEFF_DA,		0, COEFF_ZERO },	/* SRC_OUT (use W/A) */
	{ 0, COEFF_ZERO,	1, COEFF_SA },		/* DST_OUT */
	{ 0, COEFF_DA,		1, COEFF_SA },		/* SRC_ATOP */
	{ 1, COEFF_DA,		0, COEFF_SA },		/* DST_ATOP (use W/A) */
	{ 1, COEFF_DA,		1, COEFF_SA },		/* XOR (use W/A) */
	{ 0, COEFF_GA,		0, COEFF_ONE },		/* ADD */
	{ 0, COEFF_DC,		0, COEFF_ZERO },	/* MULTIPLY (use W/A) */
	{ 0, COEFF_ONE,		1, COEFF_SC },		/* SCREEN (use W/A) */
	{ 0, 0, 0, 0 },		/* DARKEN (use W/A) */
	{ 0, 0, 0, 0 },		/* LIGHTEN (use W/A) */
	{ 0, COEFF_ONE,		0, COEFF_DISJ_S },	/* DISJ_SRC_OVER (use W/A) */
	{ 0, COEFF_DISJ_D,	0, COEFF_ONE },		/* DISJ_DST_OVER (use W/A) */
	{ 1, COEFF_DISJ_D,	0, COEFF_ZERO },	/* DISJ_SRC_IN (use W/A) */
	{ 0, COEFF_ZERO,	1, COEFF_DISJ_S },	/* DISJ_DST_IN (use W/A) */
	{ 0, COEFF_DISJ_D,	0, COEFF_ONE },		/* DISJ_SRC_OUT (use W/A) */
	{ 0, COEFF_ZERO,	0, COEFF_DISJ_S },	/* DISJ_DST_OUT (use W/A) */
	{ 1, COEFF_DISJ_D,	0, COEFF_DISJ_S },	/* DISJ_SRC_ATOP (use W/A) */
	{ 0, COEFF_DISJ_D,	1, COEFF_DISJ_S },	/* DISJ_DST_ATOP (use W/A) */
	{ 0, COEFF_DISJ_D,	0, COEFF_DISJ_S },	/* DISJ_XOR (use W/A) */
	{ 0, COEFF_ONE,		1, COEFF_DISJ_S },	/* CONJ_SRC_OVER (use W/A) */
	{ 1, COEFF_DISJ_D,	0, COEFF_ONE },		/* CONJ_DST_OVER (use W/A) */
	{ 0, COEFF_CONJ_D,	0, COEFF_ONE },		/* CONJ_SRC_IN (use W/A) */
	{ 0, COEFF_ZERO,	0, COEFF_CONJ_S },	/* CONJ_DST_IN (use W/A) */
	{ 1, COEFF_CONJ_D,	0, COEFF_ZERO },	/* CONJ_SRC_OUT (use W/A) */
	{ 0, COEFF_ZERO,	1, COEFF_CONJ_S },	/* CONJ_DST_OUT (use W/A) */
	{ 0, COEFF_CONJ_D,	1, COEFF_CONJ_S },	/* CONJ_SRC_ATOP (use W/A) */
	{ 1, COEFF_CONJ_D,	0, COEFF_CONJ_D },	/* CONJ_DST_ATOP (use W/A) */
	{ 1, COEFF_CONJ_D,	1, COEFF_CONJ_S },	/* CONJ_XOR (use W/A) */
	{ 0, 0, 0, 0 },		/* USER */
	{ 1, COEFF_GA,		1, COEFF_ZERO },	/* USER_SRC_GA */
};

void fimg2d4x_set_alpha_composite(struct fimg2d_control *info,
		enum blit_op op, unsigned char g_alpha)
{
	unsigned long cfg = 0;
	struct fimg2d_blend_coeff const *tbl;

	switch (op) {
	case BLIT_OP_SOLID_FILL:
	case BLIT_OP_CLR:
		/* nop */
		return;
	case BLIT_OP_DARKEN:
		cfg |= FIMG2D_DARKEN;
		break;
	case BLIT_OP_LIGHTEN:
		cfg |= FIMG2D_LIGHTEN;
		break;
	case BLIT_OP_USER_COEFF:
		/* TODO */
		return;
	default:
		if (g_alpha < 0xff)	/* with global alpha */
			tbl = &ga_coeff_table[op];
		else
			tbl = &coeff_table[op];

		/* src coefficient */
		cfg |= tbl->s_coeff << FIMG2D_SRC_COEFF_SHIFT;

		cfg |= alpha_oprmode << FIMG2D_SRC_COEFF_SA_SHIFT;
		cfg |= alpha_oprmode << FIMG2D_SRC_COEFF_DA_SHIFT;

		if (tbl->s_coeff_inv)
			cfg |= FIMG2D_INV_SRC_COEFF;

		/* dst coefficient */
		cfg |= tbl->d_coeff << FIMG2D_DST_COEFF_SHIFT;

		cfg |= alpha_oprmode << FIMG2D_DST_COEFF_DA_SHIFT;
		cfg |= alpha_oprmode << FIMG2D_DST_COEFF_SA_SHIFT;

		if (tbl->d_coeff_inv)
			cfg |= FIMG2D_INV_DST_COEFF;

		break;
	}

	writel(cfg, info->regs + FIMG2D_BLEND_FUNCTION_REG);

	/* round mode: depremult round mode is not used */
	cfg = readl(info->regs + FIMG2D_ROUND_MODE_REG);

	/* premult */
	cfg &= ~FIMG2D_PREMULT_ROUND_MASK;
	cfg |= premult_round_mode << FIMG2D_PREMULT_ROUND_SHIFT;

	/* blend */
	cfg &= ~FIMG2D_BLEND_ROUND_MASK;
	cfg |= blend_round_mode << FIMG2D_BLEND_ROUND_SHIFT;

	writel(cfg, info->regs + FIMG2D_ROUND_MODE_REG);
}

void fimg2d4x_dump_regs(struct fimg2d_control *info)
{
	int i, offset;
	unsigned long table[][2] = {
		/* start, end */
		{0x0000, 0x0030},	/* general */
		{0x0080, 0x00a0},	/* host dma */
		{0x0100, 0x0110},	/* commands */
		{0x0200, 0x0210},	/* rotation & direction */
		{0x0300, 0x0340},	/* source */
		{0x0400, 0x0420},	/* dest */
		{0x0500, 0x0550},	/* pattern & mask */
		{0x0600, 0x0710},	/* clip, rop, alpha and color */
		{0x0, 0x0}
	};

	for (i = 0; table[i][1] != 0x0; i++) {
		offset = table[i][0];
		do {
			printk(KERN_INFO "[0x%04x] 0x%08x 0x%08x 0x%08x 0x%08x\n", offset,
				readl(info->regs + offset),
				readl(info->regs + offset+0x4),
				readl(info->regs + offset+0x8),
				readl(info->regs + offset+0xc));
			offset += 0x10;
		} while (offset < table[i][1]);
	}
}
