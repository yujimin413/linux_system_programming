#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>     // strcmp()
#include <dirent.h>     // opendir()
#include <errno.h>
#include <sys/wait.h>   // waitpid()

#include <sys/mman.h>   // mmap()

#include "start.h"

#define INPUT_SIZE 4096   // ssu_backup 실행 시 입력할 명령어 길이
#define PATH_SIZE 4096       // exec() 류 함수로 이동할 때 입력 될 경로 길이


int main(int argc, char* argv[]) {

    // 공유 메모리 생성
    int fd = shm_open("shared_memory", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    ftruncate(fd, sizeof(char) * 10);

    // 메모리 공간 매핑 
    char *HASH = mmap(NULL, sizeof(char) * 10, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    

    DIR *dirp;  // opendir() 리턴 값 저장. 성공 시 포인터, 에러 시 NULL 리턴하고 errno 설정

    // 인자 개수 예외 처리
    if (argc != 2) {
        fprintf(stderr,"Usage: ssu_backup <md5 | sha1>\n");
        exit(1);
    }
    // 인자 md5, sha1인지 구분 
    if (strcmp(argv[1], "md5") == 0)
        strcpy(HASH, "md5");
    else if (strcmp(argv[1], "sha1") == 0)
        strcpy(HASH, "sha1");
    else {
        fprintf(stderr, "Usage: ssu_backup <md5 | sha1>\n");
        exit(1);
    }



    /*
     디렉토리 중복 확인 어떻게?
     1. /Users/yujimin에 backup 디렉토리 생성해야하므로, 현재 작업 디렉토리 변경 -> chdir() 
     2. 변경 후 현재 작업 디렉토리(/Users/yujimin)에서 opendir()로 backup 디렉토리 있는지 확인. 
     3. opendir() 함수가 NULL리턴하고 errno가 ENOENT(no such file or directory)인 경우 디렉토리가 존재하지 않는 것
     4. backup 디렉토리 존재하지 않으면 생성하기
     */
    // 1. 현재 작업 디렉토리 HOME으로 변경
    if (chdir(getenv("HOME")) < 0) {
        fprintf(stderr, "chdir error\n");
        exit(1);
    }
    // 2. backup 디렉토리 존재 여부 확인
    
    // TODO: ENOENT인지도 확인할 수 있도록 수정하기
    if((dirp = opendir("backup")) == NULL) { // 3. backup 디렉토리 존재하지 않을 경우
#ifdef DEBUG
        printf("backup directory is not existed\n");
#endif
        mkdir("backup", 0777);      // 4. backup 디렉토리 생성
#ifdef DEBUG
        printf("backup directory is created\n");
#endif
    }
    else {
#ifdef DEBUG
        printf("backup directory existed\n");
#endif
    }

    // 학번> 출력, backup위한 내장명령어 입력 대기.
    pid_t pid;
    int status;
    int i;
    while(1) {
        printf("20211431>");
        char input[INPUT_SIZE] = {(char)0, };
        fgets(input, INPUT_SIZE, stdin);    // 개행문자를 입력 받지 않는 scanf() 대신 fgets() 사용. gets()는 안정성 문제로 사용 지양
        if (strcmp(&input[0], "\n") == 0) continue; // 개행만 입력 시 프롬프트 창 재출력 하기 위해 continue
        input[strlen(input) - 1] = '\0';    // fgets()는 개행 문자를 버퍼에 그대로 보관하므로 지워줘야 함

        // strcat(input, " ");
        // strcat(input, HASH);

#ifdef DEBUG
        printf("input : %s\n", input);
#endif

        // 명령어 입력 받고 해당 포인터 배열 초기화
        char *args[INPUT_SIZE];
        for (int i = 0 ; i < INPUT_SIZE ; i++) 
            args[i] = (char *)0;

        char *ret_ptr;      // strtok_r 리턴 값. 분리된 문자열의 시작 위치
        char *next_ptr;     // 남은 문자열의 시작 위치
        ret_ptr = strtok_r(input, " ", &next_ptr);
        for (i = 0 ; ret_ptr ; i++) {
            args[i] = ret_ptr;
#ifdef DEBUG
            // printf("args[%d] : %s\n", i, args[i]);
#endif
            ret_ptr = strtok_r(NULL, " ", &next_ptr);
        }


        if (strcmp(args[0], "exit") == 0) {   // exit : 프로그램 종료
            exit(0);       
        }
        if ((pid = fork()) < 0) {   // error
            fprintf(stderr, "fork error\n");
            exit(1);
        }
        else if (pid == 0) {    // 자식 프로세스
            if (strcmp(args[0], "add") == 0) {

                if (execvp("add", args) < 0) {
                    fprintf(stderr, "add execv error\n");
                    exit(1);
                }
            }
            else if (strcmp(args[0], "recover") == 0) {
                if (execvp("recover", args) < 0) {
                    fprintf(stderr, "recover execv error\n");
                    exit(1);
                }
            }
            else if (strcmp(args[0], "help") == 0) {
                if (execvp("help", args) < 0) {
                    fprintf(stderr, "help execl error\n");
                    exit(1);
                }
            }
            else {
                if (execvp("help", args) < 0) {
                    fprintf(stderr, "help execl error\n");
                    exit(1);
                }
            }

        }
        else    // 부모 프로세스
            // 자식 프로세스 끝날 때까지 wait
            //  프로세스가 exit() 호출로 정상 종료 될 경우 exit() 인자의 하위 8비트는 wait()의 status 인자로 지정됨
            if (wait(&status) != pid) {
                fprintf(stderr, "wait error\n");
                exit(1);
            } 
    } 
}


