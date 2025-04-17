#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <iostream>
#include <fstream>
#include <winsock2.h>
#include <ws2tcpip.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

#pragma comment(lib, "ws2_32.lib")

#define PORT 5002
#define BUFFER_SIZE 65536
#define DEBUG_SAVE_H264 1

void decrypt(uint8_t* data, int size) {
    int header_len = (size > 3 && data[0] == 0 && data[1] == 0 && data[2] == 1) ? 3 :
        (size > 4 && data[0] == 0 && data[1] == 0 && data[2] == 0 && data[3] == 1) ? 4 : 0;

    for (int i = header_len; i < size; i++) {
        data[i] ^= 0xAA;
    }
}

#undef main
extern "C" int main(int argc, char* argv[]) {
    // 启用FFmpeg详细日志
    av_log_set_level(AV_LOG_DEBUG);

    // 初始化网络
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed: " << WSAGetLastError() << std::endl;
        return -1;
    }

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return -1;
    }

    sockaddr_in recv_addr{};
    recv_addr.sin_family = AF_INET;
    recv_addr.sin_port = htons(PORT);
    recv_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sock, (sockaddr*)&recv_addr, sizeof(recv_addr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return -1;
    }

    // 初始化SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return -1;
    }

    // 创建窗口前强制测试SDL渲染
    SDL_Window* window = SDL_CreateWindow("Receiver",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        640, 480, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        closesocket(sock);
        WSACleanup();
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        closesocket(sock);
        WSACleanup();
        return -1;
    }

    // 使用IYUV格式纹理（兼容性更好）
    SDL_Texture* texture = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_IYUV,
        SDL_TEXTUREACCESS_STREAMING,
        640, 480);
    if (!texture) {
        std::cerr << "Texture creation failed: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        closesocket(sock);
        WSACleanup();
        return -1;
    }

    // 初始化解码器
    const AVCodec* video_codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!video_codec) {
        std::cerr << "H.264 decoder not found" << std::endl;
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        closesocket(sock);
        WSACleanup();
        return -1;
    }

    AVCodecContext* video_ctx = avcodec_alloc_context3(video_codec);
    if (!video_ctx) {
        std::cerr << "Failed to allocate codec context" << std::endl;
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        closesocket(sock);
        WSACleanup();
        return -1;
    }

    // 启用低延迟解码模式
    video_ctx->flags |= AV_CODEC_FLAG_LOW_DELAY;

    // 正确的参数设置方式
    AVDictionary** opts = (AVDictionary**)&video_ctx->priv_data;
    av_dict_set(opts, "tune", "zerolatency", 0);
    av_dict_set(opts, "preset", "ultrafast", 0);  // 可选的额外参数


    if (avcodec_open2(video_ctx, video_codec, nullptr) < 0) {
        std::cerr << "Failed to open codec" << std::endl;
        avcodec_free_context(&video_ctx);
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        closesocket(sock);
        WSACleanup();
        return -1;
    }

    // 准备资源
    AVPacket* pkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    SwsContext* sws_ctx = nullptr;
    uint8_t buffer[BUFFER_SIZE];
    std::ofstream debug_file;

#if DEBUG_SAVE_H264
    debug_file.open("received.h264", std::ios::binary);
    if (!debug_file.is_open()) {
        std::cerr << "Failed to create debug file" << std::endl;
    }
#endif

    // 主循环
    while (true) {
        // 接收数据
        sockaddr_in src_addr;
        int addr_len = sizeof(src_addr);
        int recv_len = recvfrom(sock, (char*)buffer, BUFFER_SIZE, 0,
            (sockaddr*)&src_addr, &addr_len);

        if (recv_len <= 0) {
            std::cerr << "recvfrom error: " << WSAGetLastError() << std::endl;
            continue;
        }

#if DEBUG_SAVE_H264
        if (debug_file.is_open()) {
            debug_file.write((char*)buffer, recv_len);
            debug_file.flush();
        }
#endif

        // 打印NAL单元类型（调试用）
        if (recv_len > 4) {
            uint8_t nal_type = buffer[4] & 0x1F;
            std::cout << "Received NAL unit type: " << (int)nal_type
                << (nal_type == 7 ? " (SPS)" :
                    nal_type == 8 ? " (PPS)" :
                    nal_type == 5 ? " (IDR)" : "") << std::endl;
        }

        // 解密
        decrypt(buffer, recv_len);

        // 发送到解码器
        pkt->data = buffer;
        pkt->size = recv_len;
        int send_ret = avcodec_send_packet(video_ctx, pkt);
        if (send_ret < 0) {
            std::cerr << "Error sending packet: " << av_err2str(send_ret) << std::endl;
            continue;
        }

        // 接收解码帧
        while (true) {
            int recv_ret = avcodec_receive_frame(video_ctx, frame);
            if (recv_ret == AVERROR(EAGAIN) || recv_ret == AVERROR_EOF) {
                break;
            }
            else if (recv_ret < 0) {
                std::cerr << "Error receiving frame: " << av_err2str(recv_ret) << std::endl;
                break;
            }

            // 初始化转换上下文（如果需要）
            if (!sws_ctx) {
                sws_ctx = sws_getContext(
                    frame->width, frame->height, (AVPixelFormat)frame->format,
                    frame->width, frame->height, AV_PIX_FMT_YUV420P,
                    SWS_BILINEAR, nullptr, nullptr, nullptr);
                if (!sws_ctx) {
                    std::cerr << "Failed to create SWS context" << std::endl;
                    break;
                }
            }

            // 分配目标帧内存
            AVFrame* yuv_frame = av_frame_alloc();
            yuv_frame->format = AV_PIX_FMT_YUV420P;
            yuv_frame->width = frame->width;
            yuv_frame->height = frame->height;
            if (av_frame_get_buffer(yuv_frame, 0) < 0) {
                std::cerr << "Failed to allocate frame buffer" << std::endl;
                av_frame_free(&yuv_frame);
                break;
            }

            // 转换像素格式
            sws_scale(sws_ctx,
                frame->data, frame->linesize,
                0, frame->height,
                yuv_frame->data, yuv_frame->linesize);

            // 更新SDL纹理
            SDL_UpdateYUVTexture(texture, nullptr,
                yuv_frame->data[0], yuv_frame->linesize[0], // Y
                yuv_frame->data[1], yuv_frame->linesize[1], // U
                yuv_frame->data[2], yuv_frame->linesize[2]  // V
            );

            // 渲染
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, nullptr, nullptr);
            SDL_RenderPresent(renderer);

            av_frame_free(&yuv_frame);
        }

        // 处理SDL事件
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                goto cleanup;
            }
        }
    }

cleanup:
    // 释放资源
#if DEBUG_SAVE_H264
    if (debug_file.is_open()) {
        debug_file.close();
    }
#endif
    av_packet_free(&pkt);
    av_frame_free(&frame);
    sws_freeContext(sws_ctx);
    avcodec_free_context(&video_ctx);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    closesocket(sock);
    WSACleanup();
    return 0;
}