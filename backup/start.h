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
#include <stdbool.h>
#include <openssl/sha.h>
#include <openssl/md5.h>

// 매크로
// #define INPUT_SIZE 4096   // ssu_backup 실행 시 입력할 명령어 길이
#define PATH_SIZE 4096       // exec() 류 함수로 이동할 때 입력 될 경로 길이

// 경로 이어붙이기
void concat_path(char *dest, char* src1, char* src2);

// 해시 함수
char* sha1(char *pathname);
char* md5(char *pathname);

// 파일에 대한 백업 경로 저장할 노드
typedef struct _node{
    char *realpath;  // 실제 파일이 존재하는 절대 경로
    char *backupedpath; // 백업 경로
    char *hash;
    int index;
    struct _node *next;
} Node;


char *HASH;
int idx;  // 링크드 리스트의 노드 인덱스 저장

char BACKUP[PATH_SIZE];	// 기본 백업 경로 $HOME/backup
char PWD[PATH_SIZE];
char HOME[PATH_SIZE];
char FNAME[PATH_SIZE]; // 인자로 전달 된 <FNAME>
char NEWFILE[PATH_SIZE];    // recover에서 -n옵션 사용했을 때

// Node* make_node();
// void init_list(Node *head);
void insert_node(Node *head, char *realpath, char *backuppath, char *hash, int index);
Node *search_node(Node *node, int index);
void remove_node(Node *head, int index);
void print_node(Node *head); 
void remove_all(Node *head);

// 새로운 노드 생성
Node* make_node() {
    Node *new_node = (Node*)malloc(sizeof(Node));
    new_node->next = NULL;
    return new_node;
}


// 노드 삽입 (앞쪽에)
void insert_node(Node *head, char *realpath, char *backupedpath, char *hash, int index) {
    Node *new_node = make_node();

    // 절대 경로 저장
    new_node->realpath = malloc(strlen(realpath) + 1);
    strcpy(new_node->realpath, realpath);
    // printf("절대 경로 ㅣ %s \t\t %s\n", new_node->realpath, realpath);

    // 백업 경로 저장
    new_node->backupedpath = malloc(strlen(backupedpath) + 1);
    strcpy(new_node->backupedpath, backupedpath);
    // printf("백업 경로 ㅣ %s \t\t %s\n", new_node->backupedpath, backupedpath);

    // 해시값 저장
    new_node->hash = malloc(strlen(hash) + 1);
    strcpy(new_node->hash, hash);
    // printf("해시값 ㅣ %s \t\t %s\n", new_node->hash, hash);

    // 인덱스 저장
    new_node->index = index;
    // printf("인덱스 : %d\n", index);

    Node *cur = head;
    while (cur != NULL) {
        if (cur->next == NULL) {
            new_node->next = cur->next;
            cur->next = new_node;
            break;
        }
        cur = cur->next;
    }
    
}

// 노드 탐색
Node *search_node(Node *head, int index) {
    Node *cur = head->next;
    while (cur != NULL) {
        if (cur->index == index) {
            // printf("saerch_node() 성공\nindex : %d\nbackuppedpath : %s\n", cur->index, cur->backupedpath);
            return cur;
        }
        cur = cur->next;
    }
    // printf("찾는 데이터가 존재하지 않음\n");
    return NULL;
}

// 노드 삭제
void remove_node(Node *head, int index) {
    Node *cur = head;
    while ((cur->next) != NULL) {
        if ((cur->next)->index == index) {
            cur->next = cur->next->next;
            free(cur->next); 
            return;
        }
    }
    // printf("찾는 데이터가 존재하지 않음 \n")
}

void remove_all(Node *head) {
	Node* cur = head->next;		//찾는 노드
	Node* deleteNode;			//지울 노드
	while (cur != NULL)			//리스트 끝까지 돈다.
	{
		deleteNode = cur;		//현재 노드를 지울 노드로 지정한다.
		cur = cur->next;		//현재 노드는 다음 노드로 넘어간다.
		free(deleteNode);		//지울 노드를 지워준다.
	}
	head->next = NULL;			//다 비었으니 head 다음은 tail
}


// 리스트 출력
void print_node(Node *head) {
    Node *cur = head->next;
    while (cur != NULL) {
        char *dateinfo = strrchr(cur->backupedpath, '_'); // 마지막 '_'의 포인터를 찾음
        if (dateinfo != NULL) {
            dateinfo = dateinfo + 1; // '_' 다음 위치의 포인터를 구함
        }
        // printf("dateinfo : %s\n", dateinfo);

        struct stat statbuf;
        lstat(cur->backupedpath, &statbuf);

        printf("%d. %s\t\t%lldbytes\n", cur->index, dateinfo, statbuf.st_size); 
        // printf("노드의 realpath : %s\n", cur->realpath);
        // printf("노드의 backupednode : %s\n", cur->backupedpath);

        // if (strcmp(HASH, "md5") == 0)
        //     printf("md5 : %s\n", cur->hash);
        // else if (strcmp(HASH, "sha1") == 0)
        //     printf("sha1 : %s\n", cur->hash);
        // printf("노드의 index : %d\n", cur->index);
        cur = cur -> next;
    }
    // printf("사이즈 : %d\n", size);
}

unsigned char digest[16];
struct stat statbuf;
// char *file_name;
char *data;
char md5msg[40];

char *md5(char *pathname) {
    int i;
    int fd; 
    MD5_CTX lctx;

    MD5_Init(&lctx);

    fd = open(pathname, O_RDONLY);
    if (fd < 0) {
		fprintf(stderr, "open error for %s\n", pathname);
		exit(1);
	}
    if ((fstat(fd, &statbuf)) < 0) {
        fprintf(stderr, "fstat error for %s", pathname);
        exit(1);
    }
    if (S_ISDIR(statbuf.st_mode)) {
        fprintf(stderr, "\"%s\" is a directory\n", pathname);
        exit(1);
    }

    data = (char *)malloc(statbuf.st_size);
    read (fd, data, statbuf.st_size);

    MD5_Update(&lctx, data, statbuf.st_size);
    MD5_Final(digest, &lctx);

    for (i = 0; i < sizeof(digest); ++i) {
	 	//16진수로 저장, 단 한 문자당 2칸을 할 당하고, 6인 경우 06으로 바꿔줌.
        sprintf(md5msg + (i * 2), "%02x", digest[i]);
    }

    free(data);
    return md5msg;
    
}



#define BUFSIZE	1024*16

void do_fp(FILE *f);
void pt(unsigned char *md);


char sha1msg[40];
struct stat statbuf;

char *sha1(char *pathname)
{
	// int err;
	FILE *fp;
	int fd;

	fp = fopen(pathname,"r");
	fd = fileno(fp);

	if (fp == NULL) {
		fprintf(stderr, "fopen error for %s\n", pathname);
		exit(1);
	}
	    if (fd < 0) {
		fprintf(stderr, "open error for %s\n", pathname);
		exit(1);
	}
    if ((fstat(fd, &statbuf)) < 0) {
        fprintf(stderr, "fstat error for %s", pathname);
        exit(1);
    }
    if (S_ISDIR(statbuf.st_mode)) {
        fprintf(stderr, "\"%s\" is a directory\n", pathname);
        exit(1);
    }
	
	do_fp(fp);
	fclose(fp);
	return sha1msg;
	// exit(err);
}

void do_fp(FILE *fp){
	
	//구조체
	SHA_CTX c;
	
	//SHA1()은 d에서 n바이트의 sha 암호를 계산하고 md에 배치
	//SHA_DIGEST_LENGTH == 20
	unsigned char md[SHA_DIGEST_LENGTH];
	unsigned char buf[BUFSIZE];
	int fd;
	int i;

	//파일 폰인터를 넘겨주면 파일 디스크립터를 돌려줌
	//밑에 read를 사용해서 파일 디스크립터를 받는 듯
	fd = fileno(fp);
	
	//SHA)CTX 구조를 초기화함
	SHA1_Init(&c);
	
	for (;;){
		i=read(fd,buf,BUFSIZE);
		if (i <= 0) break;
		
		//해시될 메시지 청크(data의 len 바이트)와 함께 반복적으로 호출 될 수 있음
		SHA1_Update(&c,buf,(unsigned long)i);
	}
	
	//md에 해쉬값(메시지 다이제스트)배치하고 구조체인 SHA_CTX 지움
	SHA1_Final(&(md[0]),&c);

	//해쉬값 출력을 위해 pt에 넘겨줌
	pt(md);

}


void pt(unsigned char *md){
	int i;

	//해쉬값 출력 양식맞춰줄려고 %02x 사용
	//md에 해쉬값 저장됨
	for (i = 0; i < SHA_DIGEST_LENGTH ; i++)
		// printf("%02x",md[i]);
		sprintf(sha1msg + (i * 2), "%02x", md[i]);

}