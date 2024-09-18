// main.cpp

#include "VideoReformatter.h"
#include <fstream>
#include <iostream>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <stdexcept>

int saveFrameAsPNG(AVFrame *frame, const std::string &filename) {
  AVCodec *pngCodec = avcodec_find_encoder(AV_CODEC_ID_PNG);
  if (!pngCodec) {
    std::cerr << "PNG 编码器未找到" << std::endl;
    return -1;
  }

  AVCodecContext *codecCtx = avcodec_alloc_context3(pngCodec);
  if (!codecCtx) {
    std::cerr << "无法分配编码器上下文" << std::endl;
    return -1;
  }

  codecCtx->bit_rate = 400000;
  codecCtx->width = frame->width;
  codecCtx->height = frame->height;
  codecCtx->time_base = AVRational{1, 25};
  codecCtx->framerate = AVRational{25, 1};
  codecCtx->gop_size = 10;
  codecCtx->max_b_frames = 1;
  codecCtx->pix_fmt = static_cast<AVPixelFormat>(frame->format);

  if (avcodec_open2(codecCtx, pngCodec, nullptr) < 0) {
    std::cerr << "无法打开编码器" << std::endl;
    avcodec_free_context(&codecCtx);
    return -1;
  }

  AVPacket *pkt = av_packet_alloc();
  if (!pkt) {
    std::cerr << "无法分配 AVPacket" << std::endl;
    avcodec_free_context(&codecCtx);
    return -1;
  }

  int ret = avcodec_send_frame(codecCtx, frame);
  if (ret < 0) {
    std::cerr << "发送帧到编码器失败" << std::endl;
    av_packet_free(&pkt);
    avcodec_free_context(&codecCtx);
    return -1;
  }

  ret = avcodec_receive_packet(codecCtx, pkt);
  if (ret < 0) {
    std::cerr << "从编码器接收包失败" << std::endl;
    av_packet_free(&pkt);
    avcodec_free_context(&codecCtx);
    return -1;
  }

  // 将编码后的数据写入文件
  std::ofstream outFile(filename, std::ios::binary);
  if (!outFile) {
    std::cerr << "无法打开输出文件: " << filename << std::endl;
    av_packet_free(&pkt);
    avcodec_free_context(&codecCtx);
    return -1;
  }

  outFile.write(reinterpret_cast<char *>(pkt->data), pkt->size);
  outFile.close();

  // 清理
  av_packet_free(&pkt);
  avcodec_free_context(&codecCtx);

  return 0;
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::cerr << "用法: " << argv[0] << " input.mp4 output.png" << std::endl;
    return -1;
  }

  const char *inputFilename = argv[1];
  const char *outputFilename = argv[2];

  av_register_all();

  AVFormatContext *fmtCtx = nullptr;
  if (avformat_open_input(&fmtCtx, inputFilename, nullptr, nullptr) != 0) {
    std::cerr << "无法打开输入文件: " << inputFilename << std::endl;
    return -1;
  }

  if (avformat_find_stream_info(fmtCtx, nullptr) < 0) {
    std::cerr << "无法找到流信息" << std::endl;
    avformat_close_input(&fmtCtx);
    return -1;
  }

  // 查找第一个视频流
  AVCodec *codec = nullptr;
  int videoStreamIndex = -1;
  for (unsigned int i = 0; i < fmtCtx->nb_streams; ++i) {
    if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      videoStreamIndex = i;
      codec = avcodec_find_decoder(fmtCtx->streams[i]->codecpar->codec_id);
      break;
    }
  }

  if (videoStreamIndex == -1) {
    std::cerr << "未找到视频流" << std::endl;
    avformat_close_input(&fmtCtx);
    return -1;
  }

  if (!codec) {
    std::cerr << "未找到对应的解码器" << std::endl;
    avformat_close_input(&fmtCtx);
    return -1;
  }

  AVCodecContext *codecCtx = avcodec_alloc_context3(codec);
  if (!codecCtx) {
    std::cerr << "无法分配解码器上下文" << std::endl;
    avformat_close_input(&fmtCtx);
    return -1;
  }

  if (avcodec_parameters_to_context(
          codecCtx, fmtCtx->streams[videoStreamIndex]->codecpar) < 0) {
    std::cerr << "无法复制解码器参数到上下文" << std::endl;
    avcodec_free_context(&codecCtx);
    avformat_close_input(&fmtCtx);
    return -1;
  }

  if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
    std::cerr << "无法打开解码器" << std::endl;
    avcodec_free_context(&codecCtx);
    avformat_close_input(&fmtCtx);
    return -1;
  }

  AVFrame *frame = av_frame_alloc();
  AVPacket *pkt = av_packet_alloc();

  if (!frame || !pkt) {
    std::cerr << "无法分配帧或包" << std::endl;
    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&codecCtx);
    avformat_close_input(&fmtCtx);
    return -1;
  }

  VideoReformatter reformatter;
  AVFrame *rgbFrame = nullptr;

  // 读取帧
  while (av_read_frame(fmtCtx, pkt) >= 0) {
    if (pkt->stream_index == videoStreamIndex) {
      if (avcodec_send_packet(codecCtx, pkt) == 0) {
        while (avcodec_receive_frame(codecCtx, frame) == 0) {
          // 使用 VideoReformatter 转换帧格式
          try {
            rgbFrame = reformatter.reformat(
                frame,
                0, // 保持原宽度
                0, // 保持原高度
                AV_PIX_FMT_RGB24,
                frame->colorspace != AVCOL_SPC_UNSPECIFIED ? frame->colorspace
                                                           : AVCOL_SPC_BT709,
                AVCOL_SPC_BT709,
                frame->color_range != AVCOL_RANGE_UNSPECIFIED
                    ? frame->color_range
                    : AVCOL_RANGE_MPEG,
                AVCOL_RANGE_JPEG, SWS_BILINEAR);
          } catch (const std::exception &e) {
            std::cerr << "转换失败: " << e.what() << std::endl;
            av_frame_free(&rgbFrame);
            rgbFrame = nullptr;
            break;
          }

          if (rgbFrame) {
            // 保存为 PNG
            if (saveFrameAsPNG(rgbFrame, outputFilename) == 0) {
              std::cout << "第一帧已保存为 " << outputFilename << std::endl;
            } else {
              std::cerr << "保存图像失败" << std::endl;
            }
            av_frame_free(&rgbFrame);
            rgbFrame = nullptr;
            break; // 只处理第一帧
          }
        }
      }
    }
    av_packet_unref(pkt);
    if (rgbFrame) {
      break;
    }
  }

  // 清理
  av_frame_free(&frame);
  av_packet_free(&pkt);
  avcodec_free_context(&codecCtx);
  avformat_close_input(&fmtCtx);

  return 0;
}
