#ifndef ASS2SUP_H
#define ASS2SUP_H
#ifdef __cplusplus
extern "C" {
#endif
typedef void (*ProgressCallback)(int progress);

// 注册回调函数
void ass2sup_reg_callback(ProgressCallback callback);

void ass2sup_stop();

int ass2sup_process(const char* assFile, const char* outFileName, const char* langCode, const char* video_format, const char* frame_rate);

#ifdef __cplusplus
}
#endif
#endif // ASS2SUP_H
