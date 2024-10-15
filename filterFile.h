#ifndef FILTERFILE_H
#define FILTERFILE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>
#include <libgen.h>
#include <errno.h>

#ifdef _WIN32
    #include <windows.h>
    #define PATH_SEPARATOR '\\'
#else
    #include <unistd.h>
    #include <sys/types.h>
    #define PATH_SEPARATOR '/'
#endif

typedef struct FileProcessor {
    time_t now;              // 현재 시간
    char home_dir[1024];      // 홈 디렉토리 경로
    char source_dir[1024];    // 파일 소스 경로
    char dest_dir[1024];      // 목적지 경로
    char extensions[256];     // 허용되는 확장자
    int days_threshold;       // 파일 이동 기준 일 수
    int year, month, day, hour, minute;  // 날짜/시간 입력값 저장
} FileProcessor;

extern FileProcessor file_processor;

const char *get_home_dir();
time_t get_last_modified_time(const char *path);
int move_file(const char *src, const char *dest);
int is_directory(const char *path);
int create_directory_if_not_exists(const char *path);
void process_files();
void write_log(const char *src, const char *dest, double diff_days, int is_duplicate);
void get_user_datetime_input();
void get_user_extensions_input();
void modify_paths();
int create_directory_recursively(const char *path);

#endif // FILTERFILE_H
