// add.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 	//dup(), getenv()
#include <fcntl.h>
#include <sys/stat.h>	// mkdir()
#include <sys/types.h>
#include <string.h>
#include <dirent.h> 	// scandir()
#include <stdbool.h>
#include <time.h>
#include <errno.h>
#include <sys/mman.h>	// mmap()

#include "start.h"

char addpath[PATH_SIZE];      	// 백업할 파일이 위치한 경로 저장  (ex. "/Users/yujimin/P1/a.txt")
char backuppath[PATH_SIZE];		//  파일이 백업 될 경로 저장  (ex. "/Users/yujimin/backup/P1/a.txt")
char *HASH;						// HASH 값을 공유 메모리로 사용하기 위한 변수
bool d_option;					// -d 옵션 입력되었을 경우 d_option = true;

void add_file(char *pathname, Node *head);	// 파일을 백업하기 위한 함수
void add_dir(char *pathname, Node *head);	// add <FILENAME> -d 로 명령어 실행할 경우 호출되는 함수. 디렉토리를 재귀적으로 탐색
char* getTime(void);						// UTC 기준 현재 시간을 리턴하는 함수
void copy_file(Node *head, int idx);		// 최종적으로 완성된 백업 파일 경로에 파일을 생성해서 백업을 수행하는 함수
bool check_hash_equal(char *dirname, char *filename);	// 파일 백업 시 해시값을 비교하는 함수
void concat_path(char *dest, char* src1, char* src2);	// 파일 경로를 이어 붙이는

int main(int argc, char* argv[])
{	
	Node *head;	// 파일 경로를 링크드리스트로 관리하기 위한 변수 선언
    int fd;		// 공유 메모리 오픈하기 위한 변수
	int opt;	// 옵션 처리 위한 변수
	
	// HASH 값을 전달 받기 위한 공유 메모리 오픈
	fd = shm_open("shared_memory", O_RDONLY, S_IRUSR | S_IWUSR);
    HASH = mmap(NULL, sizeof(char) * 10, PROT_READ, MAP_SHARED, fd, 0);

	// 링크드 리스트의 맨 처음 노드인 head 초기화
	head = (Node*)malloc(sizeof(Node));
	head->next = NULL;

	// 프로그램을 실행한 사용자의 HOME 경로와 PWD, 그리고 BACKUP 기본 경로 "$HOME/backup"를 전역변수로 저장
    strcpy(HOME, getenv("HOME"));
    strcpy(PWD, getenv("PWD"));
	concat_path(BACKUP, HOME, "backup");
	strcpy(backuppath, BACKUP);

	// 현재 작업 디렉토리를 HOME에서 PWD로 다시 전환
	if (chdir(PWD) < 0) {
        fprintf(stderr, "chdir error\n");
        exit(1);
    }

	// add 예외 처리 1 : add <FILENAME> 형태 혹은 add <FILENAME> [OPTION] 꼴이 아닐 경우 Usage 출력 후 종료
	if (argc < 2) {	
		printf("Usage : add <FILENAME> [OPTION]\n");
		printf("	-d add directory recursive\n");
		exit(1);
	}

	// add 예외 처리 1 : 인자 입력이 올바르지 않은 경우 중 add [OPTION]일 경우 Usage 출력 후 종료
	if (argc == 2) {
		if (strchr(argv[1], '-') != 0) {
			printf("Usage : add <FILENAME> [OPTION]\n");
			printf("	-d add directory recursive\n");
			exit(1);
		}
	}

	// add 예외 처리 2 : 첫 번째 인자로 입력 받은 경로가 길이 제한을 넘는 경우 에러 처리 후 프롬프트 재출력
	if (sizeof(argv[1]) > PATH_SIZE) {
    	fprintf(stderr, "\"%s\" : path size error\n", argv[1]);
    	exit(1);
	}

    // 옵션 처리. 올바르지 않은 인자 입력시 Usage 출력 후 프롬프르 재출력
    while ((opt = getopt(argc - 1, &argv[1], "d")) != -1) {
        switch (opt) {
            case 'd' :
				d_option = true;
				break;
			case '?':
				printf("Usage : add <FILENAME> [OPTION]\n");
				printf("	-d add directory recursive\n");
				exit(1);
        }
    }
	// -d 옵션 입력시 디렉토리 백업을 위한 함수 호출
	if (d_option == true) 
		add_dir(argv[1], head);
	// -d 옵션 입력되지 않았을 경우 파일 백업을 위한 함수 호출
	else	
		add_file(argv[1], head);

	exit(0);
}


/*
	void add_file(char *pathname, Node *head);
	파일을 백업하기 위한 함수
	첫 번째 인자 pathname : <FILENAME>. 백업할 파일의 상대경로 또는 절대경로인지를 판단해 맞다면 백업 진행, 아니라면 에러처리
	두 번째 인자 head : 백업할 파일을 링크드 리스트로 관리하기 위한 링크드 리스트 구조체 변수의 head
*/
void add_file(char *pathname, Node *head) {
	struct stat statbuf;			// 파일 혹은 디렉토리 정보를 받아오기 위한 구조체 변수
	char resolvedpath[PATH_SIZE];	// realpath()로 얻은 파일의 절대 경로 저장
	char *tmp;						// 임시로 경로 저장할 변수
	
	// 예외처리 (경로가 백업 디렉토리 포함한 경우)
	if (strstr(pathname, BACKUP) != NULL) {
		printf("\"%s\" can't be backuped\n", pathname);
		exit(0);
	}
	
	// 올바르지 않은 경로(해당 파일이나 디렉토리 없을 경우 포함) 경우 포함 입력 시 Usage 출력
	if(lstat(pathname, &statbuf) == -1) {
		printf("Usage : add <FILENAME> [OPTION]\n");
		printf("	-d add directory recursive\n");
		exit(1);
	}

	// 상대 경로 혹은 절대 경로인 경우 -> 백업 시도
	if (realpath(pathname, resolvedpath) != NULL) {

		// add 예외 처리 3 : 첫 번째로 입력 받은 경로가 사용자의 홈 디렉토리를 벗어나는 경우 예외 처리
		if (strstr(resolvedpath, HOME) == NULL) {
			printf("\"%s\" can't be backuped\n", pathname);
			exit(0);
		}

		// 백업할 파일의 절대경로 저장
		strcpy(addpath, resolvedpath);	

		// add 예외 처리 1 : 올바르지 않은 경로 ( -d 옵션 없이 디렉토리 입력할 경우 프로그램 종료 )
		if(S_ISDIR(statbuf.st_mode) == 1) {
			if (d_option == true) {
				add_dir(pathname, head);
				return;
			}
		}


		// 파일의 백업 경로 만들기 (ex. 최종적으로 "/Users/yujimin/backup/P1/a.txt_230402235017"가 backuppath에 저장됨)
		tmp = strtok(resolvedpath, "/");
		if ((tmp = strtok(NULL, "/")) == 0) {
			printf("\"%s\" can't be backuped\n", pathname);
			exit(0);
		}
		if ((tmp = strtok(NULL, "/")) == 0) {
			printf("\"%s\" can't be backuped\n", pathname);
			exit(0);
		}
		while (tmp != NULL) {
			strcat(backuppath, "/");
			strcat(backuppath, tmp);
			tmp = strtok(NULL, "/");
		}
		strcat(backuppath, "_");
		strcat(backuppath, getTime());
	
		// 백업할 파일 경로와 백업될 경로, 해시값을 링크드 리스트에 추가
		if (strcmp(HASH, "md5") == 0) 
			insert_node(head, addpath, backuppath, md5(addpath), ++idx);
		else if (strcmp(HASH, "sha1") == 0) 
			insert_node(head, addpath, backuppath, sha1(addpath), ++idx);

		// 파일 백업 실행
		// ex. "/Users/yujimin/P1/a.txt" 를 "/Users/yujimin/backup/P1/a.txt_230402235017 으로 copy
		copy_file(head, idx);		

		return;
	}

}

/*
	void add_dir(char *pathname, Node *head);
	add <FILENAME> -d 로 명령어 실행할 경우 호출되는 함수.
	디렉토리를 재귀적으로 탐색해 
	첫 번째 인자 pathname : <FILENAME>. 백업할 디렉토리의 상대경로 또는 절대경로인지를 판단해 맞다면 백업 진행, 아니라면 에러처리
	두 번째 인자 head : 백업할 디렉토리를 링크드 리스트로 관리하기 위한 링크드 리스트 구조체 변수의 head
*/
void add_dir(char *pathname, Node *head) {
	struct stat statbuf;		// 파일 혹은 디렉토리 정보를 받아오기 위한 구조체 변수
	// char* date;
	char resolvedpath[PATH_SIZE];	// realpath()로 얻은 디렉토리의 절대 경로 저장
	char dirpath[PATH_SIZE];	// realpath()로 얻은 디렉토리의 절대 경로 저장

	// add 예외 처리 1 : 첫 번째 인자로 올바르지 않은 경로 (디렉토리 없는 경우) Usage 출력
	if(lstat(pathname, &statbuf) == -1) {
		printf("Usage : add <FILENAME> [OPTION]\n");
		printf("	-d add directory recursive\n");
		exit(1);
	}

	// pathname에 디렉토리의 절대 경로 혹은 상대 경로가 정상적으로 들어왔을 경우 -> 백업 시도
	if (realpath(pathname, resolvedpath) != NULL) {
		strcpy(dirpath, resolvedpath);	// dirpath에 디렉토리의 절대 경로 저장됨
		struct dirent **namelist;	// 디렉토리의 파일에 접근하기 위한 dirent 구조체 변수
		int count;					// 디렉토리에 존재하는 파일 개수 저장
		int index;					// 디렉토리에 존재하는 파일 index
			
		// 디렉토리에 존재하는 파일 개수 count에 저장
		if((count = scandir(dirpath, &namelist, NULL, alphasort)) == -1){
			fprintf(stderr, "directory cannot search\n");
			exit(1);
		}
				
		//	디렉토리 탐색하면서 백업팔 파일의 절대 경로 완성해서 add_file() 호출해 파일 백업
		//	만약 디렉토리가 있다면 add_dir()을 재귀적으로 호출
		for(index = count - 1; index >= 0; index--){
			// "."나 "..", 그리고 "."로 시작하는 숨김파일은 백업하지 않음
			if(strcmp(namelist[index]->d_name, ".") == 0 || strcmp(namelist[index]->d_name, "..") == 0 || namelist[index]->d_name[0] == '.')
				continue;

			// 완성된 절대 경로를 tmp에 저장. 
			char tmp[PATH_SIZE];	
			strcpy(tmp, resolvedpath);
			strcat(tmp, "/");
			strcat(tmp, namelist[index]->d_name);

			// add_dir() 재귀 호출 대비해서 백업 경로 초기화(ex. "/Users/yujimin/backup"으로 돌아감)
			strcpy(backuppath, BACKUP);	

			// 디렉토리 여부를 판단해 디렉토리일 경우 add_dir()을 재귀적으로 호출, 파일이라면 add_file()을 호출해 백업
			lstat(tmp, &statbuf);
			if (S_ISDIR(statbuf.st_mode) == 1)
				add_dir(tmp, head);
			else 
				add_file(tmp, head);
		}
				
	}

}

/*
	char *getTime(void);
	UTC 기준 현재 시간을 리턴하는 함수
*/
char *getTime(void) {
    struct tm *tm_p;   // time.h에 tm 구조체 정의. 날짜, 시간 정보가 멤버 변수로 선언됨
    time_t timer;   // timer에 time() 함수의 리턴값 저장
    static char time_arr[20];    // tm 구조체 멤버변수 값(년, 월, 일, 시, 분, 초) 저장
    int year, mon, mday, hour, min, sec;

    // time()은 UTC 기준 현재 시간 및 날짜에 해당하는 시간 값을 리턴하는 시스템 호출 함수
    // 달력시간 : UTC 기준 1970년 1월 1일 00:00:00 이후 현재까지 흐른 시간을 초 단위로 카운트 한 값
    timer = time(NULL);

    // 달력시간을 tm 구조체의 멤버변수(년, 월, 일, 시, 분, 초)에 분할해서 저장
    tm_p = localtime(&timer);

    year = tm_p -> tm_year % 100;     // 년도는 끝 두 자리만 필요
    mon  = tm_p -> tm_mon + 1;        // 0~11 까지의 값 존재하므로 +1 해줘야 함
    mday = tm_p -> tm_mday;
    hour = tm_p -> tm_hour;
    min = tm_p -> tm_min;
    sec = tm_p -> tm_sec;
    
    sprintf(time_arr, "%02d%02d%02d%02d%02d%02d", year, mon, mday, hour, min, sec);

    return time_arr;
}


/*
	void copy_file(Node *head, int idx);
	최종적으로 완성된 백업 파일 경로에 파일을 생성해서 백업을 수행하는 함수
	첫 번째 인자 head : 링크드 리스트의 시작 논드
	두 번째 인자 idx : 링크드 리스트에 저장된 노드 중, 백업할 파일 경로가 저장된 노스의 인덱스
*/
void copy_file(Node *head, int idx) {
	FILE* fpr;		// 백업할 원본 파일을 read 권한으로 열어 파일 복사
	FILE* fpw;		// 백업 폴더에 백업될 파일을 write 권한으로 열어 파일 복사
	// Node *node;		// 백업할 파일 경로가 저장된 노드를 리턴 받음
	DIR *dirp;
	// char *dirname;	// 해시값 중복 여부 확인 위한 변수 - 파일이 존재하는 디렉토리의 절대 경로 (ex. "/Users/yujimin/backup/P1/D1")
	// char *filename; // 해시값 중복 여부 확인 위한 변수 - 파일명 (ex. "a.txt")
	// char *dest;		// 백업할 파일 경로 node 변수로부터 받아옴
	char tmp[PATH_SIZE];	// 임시 변수
	char backuppath[PATH_SIZE];
	int a;
	
	// 백업할 파일 경로가 저장된 노드를 node 변수에 저장
	Node *node = search_node(head, idx);

	// 파일이 존재하는 디렉토리의 절대 경로 및 파일명 각각 변수에 저장
	char *dirname = malloc(sizeof(char) * PATH_SIZE);	
	char *filename = strrchr(node->realpath, '/');	
	filename = filename + 1;

	// 백업할 파일 경로 node 변수로부터 받아옴
	char *dest = node->backupedpath;
	strcpy(tmp, dest);

	// 백업 경로 초기화
	strcpy(backuppath, BACKUP);		


	// backuppath에 파일의 최종 백업 경로를 완성하면서, 백업 디렉토리의 하위 디렉토리가 존재하지 않을 경우 생성 후 백업 
	char *fname = strtok(tmp, "/");
	fname = strtok(NULL, "/");
	fname = strtok(NULL, "/");
	fname = strtok(NULL, "/");
	while (1) {
		strcat(backuppath, "/");
		strcat(backuppath, fname);
		if ((dirp = opendir(backuppath)) == NULL) {
			fname = strtok(NULL, "/");
			if (fname != NULL) {
				mkdir(backuppath, 0777);
				strcpy(dirname, backuppath);
			}
			else 
				break;		
		}
		else {
			strcpy(dirname, backuppath);
			fname = strtok(NULL, "/");
		}
	}
	if (check_hash_equal(dirname, filename) == true) {
		printf("\"%s\" is already backuped\n", backuppath);
		return;
	}

	// 백업할 파일(절대경로)를 읽기 모드로 열기	
	if ((fpr = fopen(addpath, "r")) == NULL) {
		fprintf(stderr, "fopen error for \"%s\"\n", addpath);
		exit(1);
	}
	// 백업될 파일(절대경로 + _날짜)를 쓰기 모드로 열기		
	else if ((fpw = fopen(backuppath, "w")) == NULL) {
		fprintf(stderr, "fopen error for \"%s\"\n", backuppath);
		exit(1);
	}

	while(1){
		a = fgetc(fpr);
		
		if(!feof(fpr)){
			fputc(a, fpw);
		}else{
			break;
		}
	}
		
	fclose(fpr);
	fclose(fpw);

	printf("\"%s\" backuped\n", backuppath);

	return;
}

/*
	bool check_hash_equal(char *dirname, char *filename);
	파일 백업 시 해시값을 비교해 해시값이 동일할 경우 true를 리턴, 다를 경우 false를 리턴해
	완전히 동일한 파일을 백업하지 않도록 하는 함수
	첫 번째 인자 dirname : 파일의 상위 디렉토리 경로 (ex. "/Users/yujimin/backup/P1/D1")
	두 번째 인자 filename : 파일명 (ex. "a.txt")
*/
bool check_hash_equal(char *dirname, char *filename) {
	struct dirent **namelist;
	char tmp[PATH_SIZE];		// check 후보 파일 절대 경로를 tmp에 저장
    int count;
    int index;
	bool is_hash_equal = false;

	int i = strlen(backuppath) - 1;
	while (i >= 0 && backuppath[i] != '_') 
		i--;
	char *backupedpath = strndup(backuppath, i);


    // 입력 받은 디렉토리에 존재하는 파일 개수를 count에 저장
    if((count = scandir(dirname, &namelist, NULL, alphasort)) == -1) {
        fprintf(stderr, "directory cannot search\n");
        exit(1);
    }
    for(index = count - 1; index >= 0; index--) {
		char hash_addpath[PATH_SIZE];	// 백업할 파일의 해시값
		char hash_tmp[PATH_SIZE];		// 백업 되어있는 파일의 해시값
		int i = 0;

        // "." 와 "..", 숨김파일은 해시값 확인 대상 아님
        if(strcmp(namelist[index]->d_name, ".") == 0 || strcmp(namelist[index]->d_name, "..") == 0 || namelist[index]->d_name[0] == '.')    
            continue;

        // check 후보 파일 절대 경로를 tmp에 저장 (ex. "/Users/yujimin/backup/P1/a.txt_232323232323")
        concat_path(tmp, dirname, namelist[index]->d_name);
        // 폴더일 경우 pass
        lstat(tmp, &statbuf);
        if (S_ISDIR(statbuf.st_mode) == 1) 
            continue;
        
		// _시간 떼고 비교한 이름이 같아야 해시값 비교 대상
		i = strlen(tmp) - 1;
        while (i >= 0 && tmp[i] != '_') 
            i--;
        char *dateinfo = strndup(tmp, i);
		if (strcmp(backupedpath, dateinfo) != 0) 
			continue;
		concat_path(tmp, dirname, namelist[index]->d_name);

		// 해시값 비교해서 같을 경우 is_hash_equal = true; 해서 파일 백업을 진행하지 않도록 함
        if (strcmp(HASH, "md5") == 0) {
			strcpy(hash_addpath, md5(addpath));
			strcpy(hash_tmp, md5(tmp));
			if (strcmp(hash_addpath, hash_tmp) == 0) {
				is_hash_equal = true;
				break;
			}
		}
		else if (strcmp(HASH, "sha1") == 0) {
			strcpy(hash_addpath, md5(addpath));
			strcpy(hash_tmp, md5(tmp));
			if (strcmp(hash_addpath, hash_tmp) == 0) {
				is_hash_equal = true;
				break;
			}
		}
    }

    // namelist 메모리 해제
    for (index = 0; index < count; index++) {
        free(namelist[index]);
    }
    free(namelist); 

    if (is_hash_equal == true)
         return true;
    else 
        return false;
}


/*
	void concat_path(char *dest, char* src1, char* src2);
	파일 경로를 이어 붙이는 함수
	dest = src1 + "/" + src2
*/
void concat_path(char *dest, char* src1, char* src2){
        strcpy(dest,src1);
        strcat(dest,"/");
        strcat(dest,src2);
}