#ifndef ASS2SUP_H
#define ASS2SUP_H
#ifdef __cplusplus
extern "C" {
#endif
typedef void (*ProgressCallback)(int progress);

void ass2sup_reg_callback(ProgressCallback callback);

void ass2sup_stop();

int ass2sup_process_ass(const char* assFile, const char* sup_path, const char* xml_path, const char* langCode, const char* video_format, const char* frame_rate, int num_threads);

int ass2sup_process_avs(const char* avsFile, const char* sup_path, const char* xml_path, const char* langCode, const char* video_format, const char* frame_rate, int num_threads);

#ifdef __cplusplus
}
#endif
#endif // ASS2SUP_H
