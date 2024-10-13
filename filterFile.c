#include "filterFile.h"

FileProcessor file_processor; // 전역 변수로 구조체 선언

// 홈 디렉토리 가져오기
const char *get_home_dir() {
#ifdef _WIN32
    return getenv("USERPROFILE");
#else
    return getenv("HOME");
#endif
}

// 파일의 마지막 수정 시간을 반환하는 함수
time_t get_last_modified_time(const char *path) {
    struct stat attr;
    if (stat(path, &attr) == 0)
        return attr.st_mtime;
    return -1;
}

// 디렉토리 여부 확인하는 함수
int is_directory(const char *path) {
    struct stat path_stat;
    if (stat(path, &path_stat) != 0)
        return 0;
    return S_ISDIR(path_stat.st_mode);
}

// 파일 이동 함수
int move_file(const char *src, const char *dest) {
    return (rename(src, dest) == 0) ? 0 : -1;
}

// 디렉토리 생성 함수
int create_directory_if_not_exists(const char *path) {
    struct stat st = {0};
    if (stat(path, &st) == -1)
        return (mkdir(path, 0700) == 0) ? 0 : -1;
    return 0;
}

// 로그 작성 함수
void write_log(const char *src, const char *dest, double diff_days, int is_duplicate) {
    char buffer[4096];
    time_t rawtime = time(NULL);
    struct tm *timeinfo = localtime(&rawtime);

    snprintf(buffer, sizeof(buffer), "%s/Old_Folders_and_Files_%04d-%02d-%02d_%02d:%02d:%02d.log",
             file_processor.home_dir, timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
             timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

    FILE *log_file = fopen(buffer, "a");
    if (log_file == NULL) {
        perror("로그 파일 열기 실패");
        return;
    }

    fprintf(log_file, "%04d-%02d-%02d %02d:%02d:%02d   %s   %s   %s   %.2f일 차이   중복: %s\n",
            timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
            timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,
            src, dest, "파일 이동", diff_days, is_duplicate ? "예" : "아니오");

    fclose(log_file);
}

// 파일 처리 함수
void process_files() {
    DIR *dir;
    struct dirent *entry;

    if ((dir = opendir(file_processor.source_dir)) == NULL) {
        perror("디렉토리 열기 실패");
        return;
    }

    if (create_directory_if_not_exists(file_processor.dest_dir) != 0) {
        printf("폴더 생성 실패: %s\n", file_processor.dest_dir);
        closedir(dir);
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        char file_path[4096];
        snprintf(file_path, sizeof(file_path), "%s%c%s", file_processor.source_dir, PATH_SEPARATOR, entry->d_name);

        if (is_directory(file_path)) {
            continue;
        }

        time_t modified_time = get_last_modified_time(file_path);
        if (modified_time == -1) {
            printf("파일 정보를 가져오지 못함: %s\n", file_path);
            continue;
        }

        double diff_days = difftime(file_processor.now, modified_time) / (60 * 60 * 24);
        if (diff_days > file_processor.days_threshold) {
            char new_file_path[4096];
            snprintf(new_file_path, sizeof(new_file_path), "%s%c%s", file_processor.dest_dir, PATH_SEPARATOR, entry->d_name);

            int is_duplicate = (access(new_file_path, F_OK) == 0);
            if (move_file(file_path, new_file_path) == 0) {
                printf("파일 이동 성공: %s -> %s\n", file_path, new_file_path);
                write_log(file_path, new_file_path, diff_days, is_duplicate);
            } else {
                printf("파일 이동 실패: %s\n", file_path);
            }
        }
    }

    closedir(dir);
}

// 사용자로부터 날짜 및 시간 입력 받는 함수
void get_user_datetime_input() {
    printf("연(년): ");
    scanf("%d", &file_processor.year);
    printf("월(개월/달): ");
    scanf("%d", &file_processor.month);
    printf("일: ");
    scanf("%d", &file_processor.day);
    printf("시간: ");
    scanf("%d", &file_processor.hour);
    printf("분: ");
    scanf("%d", &file_processor.minute);
}

// 사용자로부터 확장자 입력 받는 함수
void get_user_extensions_input() {
    printf("확장자: ");
    getchar();  // 버퍼 비우기
    fgets(file_processor.extensions, sizeof(file_processor.extensions), stdin);
    file_processor.extensions[strcspn(file_processor.extensions, "\n")] = '\0';  // 개행 문자 제거
}

// 경로 수정 함수
void modify_paths() {
    char choice;
    
    printf("이전 폴더 경로를 수정하시겠습니까? (y/n): ");
    scanf(" %c", &choice);
    if (choice == 'y' || choice == 'Y') {
        printf("새 이전 폴더 경로: ");
        getchar();  // 버퍼 비우기
        fgets(file_processor.source_dir, sizeof(file_processor.source_dir), stdin);
        file_processor.source_dir[strcspn(file_processor.source_dir, "\n")] = '\0';  // 개행 문자 제거
    }

    printf("옮길 폴더 경로를 수정하시겠습니까? (y/n): ");
    scanf(" %c", &choice);
    if (choice == 'y' || choice == 'Y') 
    {
        printf("새 옮길 폴더 경로: ");
        getchar();  // 버퍼 비우기
        fgets(file_processor.dest_dir, sizeof(file_processor.dest_dir), stdin);
        file_processor.dest_dir[strcspn(file_processor.dest_dir, "\n")] = '\0';  // 개행 문자 제거
    }
}

// 메인 함수
int main(int argc, char *argv[]) 
{
    if (argc != 2 && argc != 3) // 명령 인수가 2개 또는 3개가 아니면 사용법 출력
    {
        printf("사용법: %s <일 수> [<목적지 경로>]\n", argv[0]);
        return 1;
    }

    file_processor.days_threshold = atoi(argv[1]);                                    // 이동 기준 일 수 설정
    const char *relative_dest_dir = (argc == 3) ? argv[2] : "default_dest_folder";    // 목적지 경로 설정
    file_processor.now = time(NULL);        // 현재 시간을 저장
    const char *home_dir = get_home_dir();  // 홈 디렉토리 경로 가져오기
    if (home_dir == NULL) 
    {
        printf("홈 디렉토리를 찾을 수 없습니다.\n");
        return 1;
    }

    // 홈 디렉토리 경로 설정
    snprintf(file_processor.home_dir, sizeof(file_processor.home_dir), "%s", home_dir);

    // 소스 및 목적지 경로 설정
    snprintf(file_processor.source_dir, sizeof(file_processor.source_dir), "%s%c%s", file_processor.home_dir, PATH_SEPARATOR, "source_folder");
    snprintf(file_processor.dest_dir, sizeof(file_processor.dest_dir), "%s%c%s", file_processor.home_dir, PATH_SEPARATOR, relative_dest_dir);

    // ModFF 명령에 대한 처리
    if (strcmp(argv[1], "ModFF") == 0) 
    {
        get_user_datetime_input();   // 사용자로부터 날짜 및 시간 입력받기
        get_user_extensions_input(); // 사용자로부터 확장자 입력받기
        modify_paths();              // 경로 수정
        process_files();             // 파일 처리
        return 0;
    }

    process_files(); // 일반 파일 처리
    return 0;
}
