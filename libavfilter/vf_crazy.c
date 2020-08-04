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

typedef struct CrazyContext {
    const AVClass *class;
    char *arg_;
    char *cb_;

    int (*cb)(void *, AVFrame *inOut);
    void *arg;
}  CrazyContext;

#define OFFSET(x) offsetof(CrazyContext, x)
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
    CrazyContext *s = ctx->priv;

#define CHECK_UNSET_OPT(opt)                                            \
    if (s->opt == -1) {                                            \
        av_log(s, AV_LOG_ERROR, "Option %s was not set.\n", #opt); \
        return AVERROR(EINVAL);                                         \
    }
    CHECK_UNSET_OPT(arg);
    CHECK_UNSET_OPT(cb);

    s->cb = (int(*)(void *, AVFrame *))strtoll(s->cb_, NULL, 10);
    s->arg = (void *)strtoll(s->arg_, NULL, 10);
    return 0;
}

static int filter_frame(AVFilterLink *inlink, AVFrame *in)
{
    CrazyContext *s = inlink->dst->priv;
    AVFilterLink *outlink = inlink->dst->outputs[0];
    AVFrame *out;

    out = ff_get_video_buffer(outlink, outlink->w, outlink->h);
    if (!out) {
        av_frame_free(&in);
        return AVERROR(ENOMEM);
    }

    av_frame_copy_props(out, in);
    av_frame_copy(out, in);
    av_frame_free(&in);

    int ret = s->cb(s->arg, out);

    if ( ret < 0 ) {
        av_frame_free(&out);
        return AVERROR(EINVAL);
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
    .description   = NULL_IF_CONFIG_SMALL("Crazy video filter by pass function pointer."),
    .priv_size     = sizeof(CrazyContext),
    .priv_class    = &crazy_class,
    .init          = init,
    .query_formats = query_formats,
    .inputs        = avfilter_vf_crazy_inputs,
    .outputs       = avfilter_vf_crazy_outputs,
};
