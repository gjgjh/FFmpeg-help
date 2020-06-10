/*
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with FFmpeg; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/**
 * @file
 * A very simple external call back based video filter.
 */
#include <stdio.h>

#include "libavutil/common.h"
#include "libavutil/imgutils.h"
#include "libavutil/opt.h"
#include "libavutil/pixdesc.h"
#include "avfilter.h"
#include "formats.h"
#include "internal.h"
#include "video.h"

typedef struct DelogoContext {
    const AVClass *class;
    char *arg_;
    char *cb_;

    int (*cb)(AVFrame*, void *);
    void *arg;
}  DelogoContext;

#define OFFSET(x) offsetof(DelogoContext, x)
#define FLAGS AV_OPT_FLAG_FILTERING_PARAM|AV_OPT_FLAG_VIDEO_PARAM

static const AVOption crazy_options[]= {
    { "cb",   "crazy call back's function", OFFSET(cb_), AV_OPT_TYPE_STRING, { .str = "0" }, FLAGS },
    { "arg",  "crazy call back's argument", OFFSET(arg_), AV_OPT_TYPE_STRING, { .str = "0" }, FLAGS },
    { NULL }
};

AVFILTER_DEFINE_CLASS(crazy);

static int query_formats(AVFilterContext *ctx)
{
    static const enum AVPixelFormat pix_fmts[] = {
        AV_PIX_FMT_YUV444P,  AV_PIX_FMT_YUV422P,  AV_PIX_FMT_YUV420P,
        AV_PIX_FMT_YUV411P,  AV_PIX_FMT_YUV410P,  AV_PIX_FMT_YUV440P,
        AV_PIX_FMT_YUVA420P, AV_PIX_FMT_GRAY8,
        AV_PIX_FMT_NONE
    };
    AVFilterFormats *fmts_list = ff_make_format_list(pix_fmts);
    if (!fmts_list)
        return AVERROR(ENOMEM);
    return ff_set_common_formats(ctx, fmts_list);
}

static av_cold int init(AVFilterContext *ctx)
{
    DelogoContext *s = ctx->priv;

#define CHECK_UNSET_OPT(opt)                                            \
    if (s->opt == -1) {                                            \
        av_log(s, AV_LOG_ERROR, "Option %s was not set.\n", #opt); \
        return AVERROR(EINVAL);                                         \
    }
    CHECK_UNSET_OPT(arg);
    CHECK_UNSET_OPT(cb);

    s->cb = (int(*)(AVFrame *, void *))strtoll(s->cb_, NULL, 10);
    s->arg = (void *)strtoll(s->arg_, NULL, 10);
    return 0;
}

static int filter_frame(AVFilterLink *inlink, AVFrame *in)
{
    DelogoContext *s = inlink->dst->priv;
    AVFilterLink *outlink = inlink->dst->outputs[0];
    const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(inlink->format);
    AVFrame *out;
    int hsub0 = desc->log2_chroma_w;
    int vsub0 = desc->log2_chroma_h;
    int plane;

    out = ff_get_video_buffer(outlink, outlink->w, outlink->h);
    if (!out) {
        av_frame_free(&in);
        return AVERROR(ENOMEM);
    }

    av_frame_copy_props(out, in);

#if 0
    AVRational sar;
    sar = in->sample_aspect_ratio;
    /* Assume square pixels if SAR is unknown */
    if (!sar.num)
        sar.num = sar.den = 1;
#endif

    for (plane = 0; plane < desc->nb_components; plane++) {
        int hsub = plane == 1 || plane == 2 ? hsub0 : 0;
        int vsub = plane == 1 || plane == 2 ? vsub0 : 0;

        av_image_copy_plane(out->data[plane], out->linesize[plane],
                            in ->data[plane], in ->linesize[plane],
                            AV_CEIL_RSHIFT(inlink->w, hsub),
                            AV_CEIL_RSHIFT(inlink->h, vsub));
    }
    av_frame_free(&in);

    int ret = s->cb(out, s->arg);
    if ( ret == 0 ) {
        av_frame_free(&out);
        return 0;
    }

    return ff_filter_frame(outlink, out);
}

static const AVFilterPad avfilter_vf_crazy_inputs[] = {
    {
        .name         = "default",
        .type         = AVMEDIA_TYPE_VIDEO,
        .filter_frame = filter_frame
    },
    { NULL }
};

static const AVFilterPad avfilter_vf_crazy_outputs[] = {
    {
        .name = "default",
        .type = AVMEDIA_TYPE_VIDEO,
    },
    { NULL }
};

AVFilter ff_vf_crazy = {
    .name          = "crazy",
    .description   = NULL_IF_CONFIG_SMALL("Remove logo from input video."),
    .priv_size     = sizeof(DelogoContext),
    .priv_class    = &crazy_class,
    .init          = init,
    .query_formats = query_formats,
    .inputs        = avfilter_vf_crazy_inputs,
    .outputs       = avfilter_vf_crazy_outputs,
};
