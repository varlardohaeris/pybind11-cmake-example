// VideoReformatter.cpp

#include "VideoReformatter.h"

VideoReformatter::VideoReformatter()
    : swsCtx(nullptr), currentDstFormat(AV_PIX_FMT_NONE), currentDstWidth(0),
      currentDstHeight(0), currentSrcColorspace(AVCOL_SPC_UNSPECIFIED),
      currentDstColorspace(AVCOL_SPC_UNSPECIFIED),
      currentSrcColorRange(AVCOL_RANGE_UNSPECIFIED),
      currentDstColorRange(AVCOL_RANGE_UNSPECIFIED),
      currentInterpolation(SWS_BILINEAR) {
}

VideoReformatter::~VideoReformatter() {
  if (swsCtx) {
    sws_freeContext(swsCtx);
    swsCtx = nullptr;
  }
}

SwsContext *VideoReformatter::initSwsContext(int srcWidth, int srcHeight,
                                             AVPixelFormat srcPixelFormat,
                                             int dstWidth, int dstHeight,
                                             AVPixelFormat dstPixelFormat,
                                             int interpolation) {
  // 释放旧的 SwsContext
  if (swsCtx) {
    sws_freeContext(swsCtx);
    swsCtx = nullptr;
  }

  // 初始化 SwsContext
  SwsContext *ctx =
      sws_getContext(srcWidth, srcHeight, srcPixelFormat, dstWidth, dstHeight,
                     dstPixelFormat, interpolation, nullptr, nullptr, nullptr);

  if (!ctx) {
    throw std::runtime_error("无法初始化 SwsContext");
  }

  return ctx;
}

AVFrame *VideoReformatter::reformat(const AVFrame *srcFrame, int dstWidth,
                                    int dstHeight, AVPixelFormat dstPixelFormat,
                                    int srcColorspace, int dstColorspace,
                                    int srcColorRange, int dstColorRange,
                                    int interpolation) {
  if (!srcFrame) {
    throw std::invalid_argument("源帧为空");
  }

  // 设置目标宽度和高度
  int targetWidth = (dstWidth > 0) ? dstWidth : srcFrame->width;
  int targetHeight = (dstHeight > 0) ? dstHeight : srcFrame->height;

  // 设置目标像素格式
  AVPixelFormat targetFormat =
      (dstPixelFormat != AV_PIX_FMT_NONE)
          ? dstPixelFormat
          : static_cast<AVPixelFormat>(srcFrame->format);

  // 检查是否需要重新初始化 SwsContext
  bool needInit = false;

  if (swsCtx == nullptr || targetFormat != currentDstFormat ||
      targetWidth != currentDstWidth || targetHeight != currentDstHeight ||
      srcColorspace != currentSrcColorspace ||
      dstColorspace != currentDstColorspace ||
      srcColorRange != currentSrcColorRange ||
      dstColorRange != currentDstColorRange ||
      interpolation != currentInterpolation) {
    needInit = true;
  }

  if (needInit) {
    swsCtx =
        initSwsContext(srcFrame->width, srcFrame->height,
                       static_cast<AVPixelFormat>(srcFrame->format),
                       targetWidth, targetHeight, targetFormat, interpolation);

    currentDstFormat = targetFormat;
    currentDstWidth = targetWidth;
    currentDstHeight = targetHeight;
    currentSrcColorspace = srcColorspace;
    currentDstColorspace = dstColorspace;
    currentSrcColorRange = srcColorRange;
    currentDstColorRange = dstColorRange;
    currentInterpolation = interpolation;

    // 设置颜色空间和颜色范围（如果需要）
    if (dstColorspace != AVCOL_SPC_UNSPECIFIED ||
        dstColorRange != AVCOL_RANGE_UNSPECIFIED) {
      // 注意： SwsContext 不直接支持颜色空间和范围的设置
      // 需要在转换前对源帧进行颜色空间和范围的调整
      // 这里可以根据具体需求进行实现
      // 例如，使用 sws_setColorspaceDetails
      // 但 libswscale 的 API 有所限制，详细实现较为复杂
      // 此处留空，建议使用 FFmpeg 其他工具进行颜色空间转换
    }
  }

  // 分配目标帧
  AVFrame *dstFrame = av_frame_alloc();
  if (!dstFrame) {
    throw std::runtime_error("无法分配目标帧");
  }

  dstFrame->format = targetFormat;
  dstFrame->width = targetWidth;
  dstFrame->height = targetHeight;

  // 分配缓冲区
  int ret = av_image_alloc(dstFrame->data, dstFrame->linesize, targetWidth,
                           targetHeight, targetFormat, 1);
  if (ret < 0) {
    av_frame_free(&dstFrame);
    throw std::runtime_error("无法分配图像缓冲区");
  }

  // 执行格式转换和缩放
  ret = sws_scale(swsCtx, srcFrame->data, srcFrame->linesize, 0,
                  srcFrame->height, dstFrame->data, dstFrame->linesize);

  if (ret <= 0) {
    av_freep(&dstFrame->data[0]);
    av_frame_free(&dstFrame);
    throw std::runtime_error("格式转换失败");
  }

  return dstFrame;
}
