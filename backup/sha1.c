#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <openssl/sha.h>

#define BUFSIZE	1024*16

void do_fp(FILE *f);
void pt(unsigned char *md);
#ifndef _OSD_POSIX
int read(int, void *,unsigned int);
#endif

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
        fprintf(stderr, "sha1 : \"%s\" is a directory\n", pathname);
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
	//md에 해쉬값 저장됌
	for (i = 0; i < SHA_DIGEST_LENGTH ; i++)
		// printf("%02x",md[i]);
		sprintf(sha1msg + (i * 2), "%02x", md[i]);
	// printf("%s\n", sha1msg);
	// printf("strlen : %lu\n", strlen(sha1msg));
}