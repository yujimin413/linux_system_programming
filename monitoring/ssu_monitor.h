#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <sys/wait.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <signal.h>
#include <syslog.h>



#define BUFLEN 256
FILE *log_fp;
char log_path[BUFLEN];	      // log.txt 파일 경로 저장
char *monitor_list = "monitor_list.txt";
volatile sig_atomic_t signal_received = 0;


typedef struct tree {
    char path[256];      // 경로를 저장하는 문자열 배열
    bool isEnd;          // 마지막 노드 여부를 나타내는 불리언 값
    mode_t mode;         // 파일의 모드 정보를 저장하는 변수
    time_t mtime;        // 수정 시간을 저장하는 변수
    off_t size;          // 파일 또는 디렉토리의 크기를 저장하는 변수
    struct tree *next;   // 다음 형제 노드를 가리키는 포인터
    struct tree *prev;   // 이전 형제 노드를 가리키는 포인터
    struct tree *child;  // 첫 번째 자식 노드를 가리키는 포인터
    struct tree *parent; // 부모 노드를 가리키는 포인터
} tree;

// 프로그램 실행 및 명령어 관련 함수
void ssu_monitor(int argc, char *argv[]);     // ssu_monitor 프로그램 실행
void ssu_prompt(void);                        // 프롬프트 출력 
int execute_command(int argc, char *argv[]);  // 명령어 실행
void execute_add(int argc, char *argv[]);     // add 실행
void execute_delete(int argc, char *argv[]);  // delete 실행
void execute_tree(int argc, char *argv[]);    // tree 실행
void execute_help(int argc, char *argv[]);    // help 실행

void init_daemon(char *dirpath, time_t mn_time);    // daemon 프로세스 초기화
pid_t make_daemon(FILE *fp, char *dirpath);         // daemon 프로세스 생성
bool is_already_monitored(char *dirpath);           // "monitor_list.txt"에 이미 모니터링 중인 디렉토리의 경로의 포괄 여부 확인 

// void log_event(char *filepath, char *eventtype);  //    로그 파일에 이벤트 정보 기록
// void execute_monitor(char *dirpath);    // 디렉토리와 하위 디렉토리들을 재귀적으로 모니터링하는 함수
int scandir_filter(const struct dirent *file);  // 파일 이름을 필터링하여 특정 파일들을 무시하는 함수

// tree 관련 함수
// tree *create_node(char *path, mode_t mode, time_t mtime);
tree *create_node(char *path, mode_t mode, time_t mtime);   // 노드 생성
void free_tree(tree *node);                                 // tree 노드 메모리 해제
void make_tree(tree *dir, char *path);                          // 모리터링 위한 트리 생성 함수
void execute_tree_re(tree *parent, const char *parentpath);   // 재귀적으로 디렉토리 트리를 생성
void print_tree_re(tree *node, int depth, int is_last);       // tree 구조를 재귀적으로 호출하며 출력 
void append_log(char *log);                                   // log.txt에 로그 기록
void compare_tree(tree *old, tree *new);    // 재귀적으로 tree 비교하며 변경사항 확인

void signal_handler(int signum);    // 시그널 핸들러 함수

void print_usage(void); // usage 출력 

char **GetSubstring(char *str, int *cnt, char *del);  
char *Tokenize(char *str, char *del); // 입력된 문자열(str)을 구분자(del)를 기준으로 토큰으로 분리하여 반환
char *QuoteCheck(char **str, char del); // 입력된 문자열(str) 내의 따옴표를 확인하고 처리



