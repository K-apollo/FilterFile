#include "filterFile.h"

FileProcessor file_processor;

const char *get_home_dir() 
{
#ifdef _WIN32
    return getenv("USERPROFILE");
#else
    return getenv("HOME");
#endif
}

time_t get_last_modified_time(const char *path) 
{
    struct stat attr;
    if (stat(path, &attr) == 0)
        return attr.st_mtime;
    return -1;
}

int is_directory(const char *path) 
{
    struct stat path_stat;
    if (stat(path, &path_stat) != 0)
        return 0;
    return S_ISDIR(path_stat.st_mode);
}

int move_file(const char *src, const char *dest) 
{
    return (rename(src, dest) == 0) ? 0 : -1;
}

int create_directory_if_not_exists(const char *path) {
    struct stat st = {0};
    if (stat(path, &st) == -1)
        return (mkdir(path, 0700) == 0) ? 0 : -1;
    return 0;
}

void write_log(const char *src, const char *dest, double diff_days, int is_duplicate) 
{
    char buffer[4096];
    time_t rawtime = time(NULL);
    struct tm *timeinfo = localtime(&rawtime);

    snprintf(buffer, sizeof(buffer), "%s/Old_Folders_and_Files_%04d-%02d-%02d_%02d:%02d:%02d.log",
             file_processor.home_dir, timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
             timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

    FILE *log_file = fopen(buffer, "a");
    if (log_file == NULL) 
    {
        perror("로그 파일 열기 실패");
        return;
    }

    fprintf(log_file, "%04d-%02d-%02d %02d:%02d:%02d   %s   %s   %s   %.2f일 차이   중복: %s\n", 
            timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, 
            timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, 
            src, dest, "파일 이동", diff_days, is_duplicate ? "예" : "아니오");

    fclose(log_file);
}

void process_files() 
{
    DIR *dir;
    struct dirent *entry;

    if ((dir = opendir(file_processor.source_dir)) == NULL) 
    {
        perror("디렉토리 열기 실패");
        return;
    }

    if (create_directory_if_not_exists(file_processor.dest_dir) != 0) 
    {
        printf("폴더 생성 실패: %s\n", file_processor.dest_dir);
        closedir(dir);
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        char file_path[4096];
        snprintf(file_path, sizeof(file_path), "%s%c%s", file_processor.source_dir, PATH_SEPARATOR, entry->d_name);

        if (is_directory(file_path))
            continue;

        time_t modified_time = get_last_modified_time(file_path);
        if (modified_time == -1) 
        {
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

void get_user_datetime_input() 
{
    printf("연(년): ");       scanf("%d", &file_processor.year);
    printf("월(개월/달): ");  scanf("%d", &file_processor.month);
    printf("일: ");           scanf("%d", &file_processor.day);
    printf("시간: ");         scanf("%d", &file_processor.hour);
    printf("분: ");           scanf("%d", &file_processor.minute);
}

void get_user_extensions_input() 
{
    printf("확장자: ");
    getchar();  // 버퍼 비우기
    fgets(file_processor.extensions, sizeof(file_processor.extensions), stdin);
    file_processor.extensions[strcspn(file_processor.extensions, "\n")] = '\0';  // 개행 문자 제거
}

void modify_paths() 
{
    char choice;
    
    printf("이전 폴더 경로를 수정하시겠습니까? (y/n): ");
    scanf(" %c", &choice);
    if (choice == 'y' || choice == 'Y') 
    {
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

int create_directory_recursively(const char *path) 
{
    char temp_path[4096];
    strncpy(temp_path, path, sizeof(temp_path));
    temp_path[sizeof(temp_path) - 1] = '\0'; // 경로 길이를 초과하지 않도록 처리

    char *dir = dirname(temp_path);  // 상위 디렉토리 경로 가져오기

    if (access(dir, F_OK) != 0)
        create_directory_recursively(dir); // 상위 디렉토리 생성

    if (mkdir(path, 0700) == -1) 
    {
        if (errno != EEXIST) // 디렉토리가 이미 존재하는 경우가 아니라면
        {
            perror("디렉토리 생성 실패");
            return -1;
        }
    }

    return 0;
}

int create_directory_if_not_exists(const char *path) {
    struct stat st = {0};
    if (stat(path, &st) == -1)
        return create_directory_recursively(path);  // 재귀적으로 디렉토리 생성
    return 0;
}

int main(int argc, char *argv[]) 
{
    if (argc != 2 && argc != 3) // 명령 인수가 2개 또는 3개가 아니면 사용법 출력
    {
        printf("사용법: %s <일 수> [<목적지 경로>]\n", argv[0]);
        return 1;
    }

    // 인자에서 일 수를 가져와서 days_threshold에 저장
    file_processor.days_threshold = atoi(argv[1]);

    // 목적지 경로가 제공되면 dest_dir에 저장
    if (argc == 3) 
    {
        strncpy(file_processor.dest_dir, argv[2], sizeof(file_processor.dest_dir));
        file_processor.dest_dir[sizeof(file_processor.dest_dir) - 1] = '\0'; // 안전한 문자열 복사
    } 
    else 
    {
        snprintf(file_processor.dest_dir, sizeof(file_processor.dest_dir), "%s/Old_Files", file_processor.home_dir); // 기본 경로 설정
    }

    // 홈 디렉토리 가져오기
    strncpy(file_processor.home_dir, get_home_dir(), sizeof(file_processor.home_dir));
    file_processor.home_dir[sizeof(file_processor.home_dir) - 1] = '\0'; // 안전한 문자열 복사

    // 사용자로부터 날짜 및 확장자 입력받기
    get_user_datetime_input();
    get_user_extensions_input();
    
    // 현재 시간 설정
    struct tm timeinfo = {0};
    timeinfo.tm_year = file_processor.year - 1900; // 연도는 1900부터 시작
    timeinfo.tm_mon = file_processor.month - 1;    // 월은 0부터 시작
    timeinfo.tm_mday = file_processor.day;
    timeinfo.tm_hour = file_processor.hour;
    timeinfo.tm_min = file_processor.minute;
    file_processor.now = mktime(&timeinfo);

    // 경로 수정
    modify_paths();

    // 파일 처리
    process_files();

    return 0;
}
