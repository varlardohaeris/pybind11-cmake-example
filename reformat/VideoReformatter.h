// VideoReformatter.h

#ifndef VIDEOREFORMATTER_H
#define VIDEOREFORMATTER_H

extern "C" {
#include <libavutil/imgutils.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
}

#include <stdexcept>
#include <string>

class VideoReformatter {
public:
  VideoReformatter();
  ~VideoReformatter();

  /**
   * @brief 重新格式化视频帧
   *
   * @param srcFrame 源 AVFrame
   * @param dstWidth 目标宽度（如果为 0，则保持源宽度）
   * @param dstHeight 目标高度（如果为 0，则保持源高度）
   * @param dstPixelFormat 目标像素格式（如 AV_PIX_FMT_RGB24）
   * @param srcColorspace 源颜色空间（如 AVCOL_SPC_BT709）
   * @param dstColorspace 目标颜色空间（如 AVCOL_SPC_BT709）
   * @param srcColorRange 源颜色范围（如 AVCOL_RANGE_JPEG）
   * @param dstColorRange 目标颜色范围（如 AVCOL_RANGE_JPEG）
   * @param interpolation 插值方法（如 SWS_BILINEAR）
   * @return AVFrame* 转换后的新 AVFrame，需要调用 av_frame_free 释放
   */
  AVFrame *reformat(const AVFrame *srcFrame, int dstWidth = 0,
                    int dstHeight = 0,
                    AVPixelFormat dstPixelFormat = AV_PIX_FMT_NONE,
                    int srcColorspace = AVCOL_SPC_UNSPECIFIED,
                    int dstColorspace = AVCOL_SPC_UNSPECIFIED,
                    int srcColorRange = AVCOL_RANGE_UNSPECIFIED,
                    int dstColorRange = AVCOL_RANGE_UNSPECIFIED,
                    int interpolation = SWS_BILINEAR);

private:
  struct SwsContext *swsCtx;
  AVPixelFormat currentDstFormat;
  int currentDstWidth;
  int currentDstHeight;
  int currentSrcColorspace;
  int currentDstColorspace;
  int currentSrcColorRange;
  int currentDstColorRange;
  int currentInterpolation;

  /**
   * @brief 初始化 SwsContext
   *
   * @param srcWidth 源宽度
   * @param srcHeight 源高度
   * @param srcPixelFormat 源像素格式
   * @param dstWidth 目标宽度
   * @param dstHeight 目标高度
   * @param dstPixelFormat 目标像素格式
   * @param interpolation 插值方法
   * @return SwsContext*
   */
  SwsContext *initSwsContext(int srcWidth, int srcHeight,
                             AVPixelFormat srcPixelFormat, int dstWidth,
                             int dstHeight, AVPixelFormat dstPixelFormat,
                             int interpolation);
};

#endif // VIDEOREFORMATTER_H
