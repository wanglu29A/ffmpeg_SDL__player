#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <SDL2/SDL.h>
#include <zconf.h>

#define __STDC_CONSTANT_MACROS

#define MAX_AUDIO_FRAME_SIZE 88200

#define OUTPUT_PCM 1
#define USE_SDL 1

static Uint8 *audio_chunk;
static Uint32 audio_len;
static Uint8 *audio_pos;

void fill_audio(void *udata, Uint8 *stream, int len)
{
    SDL_memset(stream, 0, len);
    if(0 == audio_len)
    {

    }
    len = (len > audio_len? audio_len : len);

    SDL_MixAudio(stream, audio_pos, len, SDL_MIX_MAXVOLUME);
    audio_pos += len;
    audio_len -= len;
}

int main() {

    AVFormatContext *pFormatCtx;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;
    AVPacket *packet;
    AVFrame *pFrame;
    SDL_AudioSpec wanted_spec;

    uint8_t *out_buffer;
    int i, audiostream;
    int ret;
    uint32_t  len = 0;
    int got_picture;
    int index = 0;
    int64_t in_channel_layout;
    struct SwrContext *au_convert_ctx;

    FILE *pFile = NULL;
    char url[] = "../file/demo2.mp3";  //文件名


    ////全局网络库的初始化
    avformat_network_init();
    pFormatCtx = avformat_alloc_context();

    ////打开音频媒体文件
    if(0 != avformat_open_input(&pFormatCtx, url, NULL, NULL))
    {
        printf("打开音频文件失败\n");
    }

    ////读数据包获取信息
    if(0 > avformat_find_stream_info(pFormatCtx, NULL))
    {
        printf("不能找到流信息\n");
    }

    ////打印输入的上下文信息
    av_dump_format(pFormatCtx, 0, url, 0);

    audiostream = -1;

    ////pFormatCtx->nb_streams 为媒体流的数量
    ////循环是为了查看有没有音频流
    for(i=0; i<pFormatCtx->nb_streams; i++)
    {
        if(AVMEDIA_TYPE_AUDIO == pFormatCtx->streams[i]->codecpar->codec_type)
        {
            audiostream = i;
            break;
        }
    }
    if(-1 == audiostream)
    {
        printf("没有找到音频流\n");
        return -1;
    }

    ////根据codec_id获得一个解码器
    pCodecCtx = pFormatCtx->streams[audiostream]->codec;
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if(NULL == pCodec)
    {
        printf("找不到编码器\n");
    }
    ////打开解码器
    if(0 > avcodec_open2(pCodecCtx, pCodec, NULL))
    {
        printf("无法打开解码器\n");
    }

    pFile = fopen("demo1.pcm", "wb");

    ////给数据包分配空间，并初始化
    packet = (AVPacket*)av_malloc(sizeof(AVPacket));
    av_init_packet(packet);

    ////设置输出的声道布局
    uint64_t out_channel_layout = AV_CH_LAYOUT_STEREO;

    ////每一帧中的每个声道的采样数
    int out_nb_samples = pCodecCtx->frame_size;

    ////输出的采样格式
    enum AVSampleFormat  out_sample_fmt = AV_SAMPLE_FMT_S16;
    int out_sample_rate = 44100;
    int out_channels = av_get_channel_layout_nb_channels(out_channel_layout);

    ////根据参数提供缓冲的大小
    int out_buffer_size = av_samples_get_buffer_size(NULL, out_channels, out_nb_samples, out_sample_fmt, 1);

    out_buffer = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE*2);
    pFrame = av_frame_alloc();

    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
    {
        printf("不能初始化SDL\n");
    }

    wanted_spec.freq = out_sample_rate;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.channels = out_channels;
    wanted_spec.silence = 0;
    wanted_spec.samples = out_nb_samples;
    wanted_spec.callback = fill_audio;
    wanted_spec.userdata = pCodecCtx;

    if(0 > SDL_OpenAudio(&wanted_spec, NULL))
    {
        printf("不能打开音频\n");
    }

    in_channel_layout = av_get_default_channel_layout(pCodecCtx->channels);

    au_convert_ctx = swr_alloc();
    au_convert_ctx = swr_alloc_set_opts(au_convert_ctx, out_channel_layout, out_sample_fmt,
            out_sample_rate, in_channel_layout,pCodecCtx->sample_fmt, pCodecCtx->sample_rate, 0,  NULL);

    swr_init(au_convert_ctx);

    SDL_PauseAudio(0);
    //sleep(30);
    while(0 <= av_read_frame(pFormatCtx, packet))
    {
        if(audiostream == packet->stream_index)
        {
            ret = avcodec_decode_audio4(pCodecCtx, pFrame, &got_picture, packet);
            if(0 > ret)
            {
                printf("解码失败！");
            }
            if(0 < got_picture)
            {
                swr_convert(au_convert_ctx, &out_buffer, MAX_AUDIO_FRAME_SIZE, (const uint8_t **)pFrame->data , pFrame->nb_samples);
                printf("index:%5d\t pts:%lld\t packet size:%d\n",index, packet->pts, packet->size);
                fwrite(out_buffer, 1, out_buffer_size, pFile);
                index++;
            }

            while (0 < audio_len)
                SDL_Delay(1);

            audio_chunk = (Uint8 *)out_buffer;

            audio_len = out_buffer_size;
            audio_pos = audio_chunk;
        }
        av_packet_unref(packet);
    }
    swr_free(&au_convert_ctx);

    SDL_CloseAudio();
    SDL_Quit();

    fclose(pFile);

    av_free(out_buffer);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
    return 0;
}