//$gcc getmd5.c -lssl -lcrypto
//or
//$gcc getmd5.c -lcrypto
//openssl version : 1.1.1

//ISSUE : no such file or dirctory 떠서
//SOLVE : sudo apt-get install libssl-dev 했더니 해결.

//ISSUE : undefiend reference to 'MD5'
//아마 매크로로 버전관리 해주면 될 듯.
//SOLVE?? : gcc md5.c -lssl -lcrypto 하면 컴파일은 됌.

#include <openssl/md5.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

//argv에 해쉬값을 구할 파일 이름이 들어감.
unsigned char digest[16];
struct stat statbuf;
// char *file_name;
char *data;
char md5msg[40];

char *md5(char *pathname) {
    int i;
    int fd; 
	//구조체
    MD5_CTX lctx;
	/*
	struct MD5_CTX{
		ULONG i[2];
		ULONG buf[4];
		unsigned char in[64];
		unsigned char digest[16];
	}
	*/

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
        fprintf(stderr, "md5 error : \"%s\" is a directory\n", pathname);
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
    // printf ("%s\n", md5msg);
    // printf("strlen : %lu\n", strlen(md5msg));
    free(data);
    // printf("해시값 리턴 :  %s\n", md5msg);
    return md5msg;
    
}
