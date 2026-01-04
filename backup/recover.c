#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     //dup(), getenv()
#include <fcntl.h>
#include <sys/stat.h>	// mkdir()
#include <sys/types.h>
#include <string.h>
#include <dirent.h>     //scandir()
#include <stdbool.h>
#include <time.h>

#include <sys/mman.h>	// mmap()

#include "start.h"

char recoverpath[PATH_SIZE];        // 최종적으로 백업 복구할 경로 저장 (ex. "/Users/yujimin/P1/a.txt")
char backupedpath[PATH_SIZE];       // 최종적으로 백업되어있는 경로_날짜 저장     (ex. "/Users/yujimin/backup/P1/a.txt_23232323")
char *HASH;						    // HASH 값을 공유 메모리로 사용하기 위한 변수
bool d_option;                      // -d 옵션 입력되었을 경우 d_option = true;
bool n_option;                      // -n 옵션 입력되었을 경우 n_option = true;
bool is_file_only_in_backupdir;     // 백업 복구하려는 파일이 사용자의 홈 디렉토리에 없지만 백업 디렉토리에는 백업파일이 있는 경우 true 저장

void recover_file(char *pathname, Node *head);  // 백업 되어있는 파일 경로 찾아줌
void recover_dir(char *backuped_dir_path, char *recover_dir_path, Node *head);   // -d 옵션 입력했을 때, 백업 되어있는 파일 경로 찾아줌
void recover(char *backuppedpath, char *recoverpath, Node *head);   // recover_file or recover_dir로부터 백업 되어있는 파일 경로를 넘겨 받아 recover 진행
char *check_file_only_in_backupdir(char *subpath, char *pathname); // (/Users/yujimin/backup/P1, a.txt)
void concat_path(char *dest, char* src1, char* src2);

int main(int argc, char *argv[])
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
    concat_path(BACKUP, HOME, "backup");    // /Users/yujimin/backup
    strcpy(backupedpath, BACKUP);       // backupedpath : /Users/yujimin/backup

#ifdef DEBUG
    printf("###recover 명령어 실행\n");
#endif

    // 현재 작업 디렉토리를 HOME에서 PWD로 다시 전환
	if (chdir(PWD) < 0) {
        fprintf(stderr, "chdir error\n");
        exit(1);
    }

    // recover 예외 처리 1 : recover <FILENAME> 형태 혹은 recover <FILENAME> [OPTION] 꼴이 아닐 경우 Usage 출력 후 종료
	if (argc < 2) {
        printf("Usage : <FILENAME> [OPTION]\n");
        printf("    -d : recover directory recursize\n");
        printf("    -n <NEWNAME> : recover filse with new name\n");
		exit(1);
	}

    // recover 예외 처리 1 : 인자 3개는 recover <FILENAME> -d 만 가능
	if (argc == 2) {	
		if (strchr(argv[1], '-') != 0) {
            printf("Usage : <FILENAME> [OPTION]\n");
            printf("    -d : recover directory recursize\n");
            printf("    -n <NEWNAME> : recover filse with new name\n");
            exit(1);
		}
	}

    /*
        옵션 처리
        recover [FILENAME] <OPTION> 형식을 잘 지켰다고 가정
        [FILENAME] 없이 recover <OPTION> 형식으로 입력된 경우는 고려하지 않음
    */
    while ((opt = getopt(argc - 1, &argv[1], "dn:")) != -1) {
        // printf("opt : %c, optopt : %d\n", opt, optopt);
        switch (opt) {
            case 'd' :
                // printf("recover <FILANAME> -d entered\n");
				d_option = true;
				break;
            case 'n' :
                // printf("recover <FILENAME> -n <NEWNAME> entered\n");
				n_option = true;
                strcpy(NEWFILE, optarg);
				break;
			case '?' :
                printf("Usage : <FILENAME> [OPTION]\n");    
                printf("    -d : recover directory recursize\n");
                printf("    -n <NEWNAME> : recover filse with new name\n");
                exit(1);
        }
    }
    // -d 옵션 입력시 디렉토리 복구를 위한 함수 호출
    if (d_option == true) {
        // case 1 : -d, -n 입력
        if (n_option == true) { 
            // printf("d && n\n");
        }
        // printf("d\n");

        // case 2 : -d만 입력
        // 절대 경로를 완성한 뒤,  recover_dir() 호풀
        // <FILENAME>에 절대 경로 입력된 경우
        if (argv[1][0] == '/')   {              
            strcpy(recoverpath, argv[1]);       // (ex. recoverpath = "/Users/yujimin/P1/a")
            char tmp[PATH_SIZE];
            strcpy(tmp, recoverpath);           // (ex. tmp = "/Users/yujimin/P1/a")
            char *ptr = strtok(tmp, "/");       // (ex. pathmame = "/Users/yujimin/P1/a")
            ptr = strtok(NULL, "/");
            ptr = strtok(NULL, "/");
            while (ptr != NULL) {               // (ex. backupedpath = "/Users/yujimin/backup/P1/a")
                strcat(backupedpath, "/");
                strcat(backupedpath, ptr);
                ptr = strtok(NULL, "/");
            }
        }   
        // <FILENAME>에 절대 경로가 입력되지 않은 경우, 우선은 상대 경로로 가정하고 절대경로 완성
        else {      
            char tmp[PATH_SIZE];
            strcpy(tmp, PWD);                    // (ex. tmp = "/Users/yujimin/P1")
            strcat(tmp, "/");      
            strcat(tmp, argv[1]);               // (ex. tmp = "/Users/yujimin/P1/a")
            strcpy(recoverpath, tmp);           // (ex. recoverpath = "/Users/yujimin/P1/a")
            char *ptr = strtok(tmp, "/");       // (ex. tmp = "/Users/yujimin/P1/a")
            ptr = strtok(NULL, "/");
            ptr = strtok(NULL, "/");       
            while (ptr != NULL) {               // (ex. backupedpath = "/Users/yujimin/backup/P1/a")
                strcat(backupedpath, "/");
                strcat(backupedpath, ptr);
                ptr = strtok(NULL, "/");
            }

        }
        recover_dir(backupedpath, recoverpath, head);
    }
    // case 3 : -n만 입력
    else if (n_option == true) {
        recover_file(argv[1], head);
        // printf("n\n");
    }
    // case 4 : 옵션 없이 <FILENAME>만 입력
    else {
        // printf("<FILENAME>만 입력\n");
        recover_file(argv[1], head);
     
    }
        
	exit(0);
}

/*
    void recover_file(char *pathname, Node *head);
    백업되어 있는 파일을 다시 복구하기 위한 함수
    첫 번째 인자 pathname : <FILENAME>. 복구할 경로를 입력 받아 파일 복구 진행
*/
void recover_file(char *pathname, Node *head) { 
    int idx = 0;                    // 링크드리스트 노드 개수
    struct stat statbuf;            // pathname의 파일 정보 저장하는 구조체
    char resolvedpath[PATH_SIZE];	// realpath()로 얻은 파일의 절대 경로 저장
    char dirpath[PATH_SIZE];	    // check_backuped_exist 함수 첫 번쩨 인자 : 파일명 직전 디렉토리명 저장
    char *fname;                    // check_backuped_exist 함수 두 번쩨 인자 : 파일명 저장
    char absolutepath[PATH_SIZE];   // dirpath + fname에서 backup 빠진 경로     // /Users/yujimin/P1/a.txt

    strcpy(FNAME, pathname);        // 전역변수인 FNAME에 입력받은 경로 저장
    strcpy(backupedpath, BACKUP);   // 백업 경로 초기화


    // absolutepath에 파일의 절대 경로 저장
    // 절대 경로 입력된 경우
    if (pathname[0] == '/')     
        strcpy(absolutepath, pathname);
    // 상대 경로 입력된 경우
    else {     
        char tmp[PATH_SIZE];
        strcpy(tmp, PWD);
        strcat(tmp, "/");
        strcat(tmp, pathname);
        strcpy(absolutepath, tmp);  // (ex. absolutepath =  "/Users/yujimin/P1/a.txt")
    }

#ifdef DEBUG
    printf("absolutepath : %s\n", absolutepath);
#endif


#ifdef DEBUG
    printf("### recover_file() 실행\n");
#endif

    // CASE 1 : <FILENAME>이 (절대경로 || 상대경로) 에 존재하지 "않는" 경우
    if (lstat(pathname, &statbuf) == -1) {
        char tmp[PATH_SIZE];
        char *ptr;

#ifdef DEBUG
        printf("BIG CASE 1 : <FILENAME>이 (절대경로 || 상대경로) 에 존재하지 \"않는\" 경우\n");
        printf("백업디렉토리에는 존재하는지 확인 필요\n");
        printf("pathname : %s\n", pathname);
#endif

        // 입력된 경로의 최 하위, 즉 파일 이름만 저장 (ex. fname = "a.txt")
        fname = strrchr(pathname, '/');
        if (fname == NULL)
	        fname = FNAME;
	    else
	        fname = fname + 1;
	    
#ifdef DEBUG
        printf("===============\npathname : %s\t fname  %s\n", pathname, fname);
#endif
        
        // dirpath에 파일으 바로 상위 디렉토리의 절대 경로 저장
        if (strcmp(fname, pathname) == 0) {
            ptr = strtok(PWD, HOME);
        }
        else
            ptr = strtok(pathname, HOME); // ptr : P1
        while (ptr != NULL) {
            strcat(backupedpath, "/");
            strcat(backupedpath, ptr); 
            ptr = strtok(NULL, "/");  
            if (ptr != NULL) {
                strcpy(dirpath, backupedpath);
            }
        }
        if (strcmp(fname, pathname) == 0)
            strcpy(dirpath, backupedpath);
#ifdef DEBUG
        printf("\ndir : %s\nfname : %s\n", dirpath, fname);
#endif

        // 입력한 파일이 backup 폴더 안에 존재하는지 확인, 만약 존재한다면 CASE 2로 넘어감. 아니라면 종료
        if ((pathname = check_file_only_in_backupdir(dirpath, fname)) == NULL) {
            fprintf(stderr, "\"%s\" can't be backuped\n", absolutepath);
            exit(1);
        }
    }

#ifdef DEBUG
    printf("case2\n");
    printf("pathname : %s\n", pathname);
#endif

    /* 
        CASE 2 : <FILENAME>이 (절대경로 || 상대경로) 에 존재하는 경우
        or (<FILENAME>이 (절대경로 || 상대경로) 에 존재하지 않지만 백업디렉토리에는 백업파일이 있는 경우) || (복구하려는 파일이 백업디렉토리에만 존재)
    */
	if (realpath(pathname, resolvedpath) != NULL || is_file_only_in_backupdir == true) {
        struct dirent **namelist;
        int count;
        int index;
        bool is_hash_equal = false;         // 백업 디렉토리에 백업된 파일과 홈 디렉토리에 있는 파일 해시값이 일치하는지 판단하는 변수. (같으면 recover하지 않음)
        char without_date[PATH_SIZE];       // 날짜 제외하고 완성된, 백업된 파일 경로 저장
#ifdef DEBUG
        printf("resolved : %s\n", resolvedpath);
#endif
        // realpath()로 완성된 절대 경로 정보를 받아옴. 디렉토리 일 경우 에러 처리
        lstat(resolvedpath, &statbuf);
        if(S_ISDIR(statbuf.st_mode) == 1) {
            if (d_option == true) {
#ifdef DEBUG
                printf("1111디렉토리 경로 입력.이거 나오면 안됨...\n");exit(0);
#endif
                return;
            }
            fprintf(stderr, "\"%s\" is a directory file\n", resolvedpath);
            exit(1);
        }

#ifdef DEBUG
		printf("CASE 2 : 파일명 or 절대 경로 or 상대 경로\n");
#endif
        // 최종적으로 복구될 경로 저장. resolvedpath는 경로 손상 가능하므로 전역 변수인 recoverpath에 복사 (ex. recoverpath = "/Users/yujimin/P1/a.txt")
		strcpy(recoverpath, resolvedpath);	

#ifdef DEBUG
			printf("recover할 파일 기본 위치 backupedpath : %s\n", backupedpath);	// /Users/yujimin/backup/
			printf("recover될 파일 위치 recoverpath : %s\n", recoverpath);		// /Users/yujimin/P1/a.txt
#endif
        // if(백업 복구하려는 파일이 사용자의 홈 디렉토리에 있는 경우) 백업디렉토리의 백업 파일 경로가 필요함. else(이미 백업 파일 경로 완성된 상태)
        if (is_file_only_in_backupdir == false) {
            char *tmp = strtok(resolvedpath, "/"); // tmp : P1
            tmp = strtok(NULL, "/");
	        tmp = strtok(NULL, "/");
            while (tmp != NULL) {
                strcat(backupedpath, "/");
                strcat(backupedpath, tmp);
                tmp = strtok(NULL, "/");
                // 완성 중인 backupedpath가 폴더일 경우 dirpath에 저장. 백업된 파일 리스트 추출 위함
                if (tmp != NULL) {
                    strcpy(dirpath, backupedpath);
                }
            }
        }
#ifdef DEBUG
        printf("날짜 빼고 완성된 backupedpath : %s\n", backupedpath);
        printf("백업할 파일 바로 상위 폴더 경로 : %s\n", dirpath);
#endif
            
        // 날짜 제외하고 완성된, 백업된 파일 경로 저장 (ex. "/Users/yujimin/backup/P1/a/c.txt")
        strcpy(without_date, backupedpath);     
            
        //  날짜 제외하고 완성된, 백업된 파일 경로의 상위 디렉토리에 존재하는 파일 개수를 count에 저장
        // recover 예외 처리 1 : 해당 백업 파일이 존재하지 않을 경우 Usage 출력
        if((count = scandir(dirpath, &namelist, NULL, alphasort)) == -1) {
            printf("Usage : <FILENAME> [OPTION]\n");
            printf("    -d : recover directory recursize\n");
            printf("    -n <NEWNAME> : recover filse with new name\n");
            exit(1);
        }

#ifdef DEBUG
        printf("cnt : %d\n", count);
#endif
        
        //    백업된 디렉토리 탐색하면서 recover할 대상이 되는 파일을 찾아 링크드 리스트에 저장
        for(index = count - 1; index >= 0; index--) {
            char hash_recoverpath[PATH_SIZE];
            char hash_backupedpath[PATH_SIZE];

            // "." 와 "..", 그리고 숨김 파일은 recover할 대상이 아님
            if(strcmp(namelist[index]->d_name, ".") == 0 || strcmp(namelist[index]->d_name, "..") == 0 || namelist[index]->d_name[0] == '.')
                continue;
            concat_path(backupedpath, dirpath, namelist[index]->d_name);

            // 폴더일 경우 해당 노드는 링크드 리스트에 넣지 않음
            lstat(backupedpath, &statbuf);
            if (S_ISDIR(statbuf.st_mode) == 1)
                continue;

            // namelist[index]->d_name 에서 날짜 뗀 파일명만 저장
            char dname_without_date[PATH_SIZE];
            strcpy(dname_without_date, namelist[index]->d_name);
            char *date = strrchr(dname_without_date, '_');
            if (date != NULL) {
            *date = '\0'; 
            }

            // recoverpath에서 마지막 파일 이름만 저장
            char *fname_recoverpath = strrchr(recoverpath, '/');
            fname_recoverpath += 1;

#ifdef DEBUG
            printf("%s\t", fname_recoverpath);
            printf("%s\n", dname_without_date);
#endif
            /*
                해시 값을 비교하여 백업 되어있는 파일과 백업하려는 파일의 해시 값이 같지 않고,
                동시에 파일명(백업된 파일의 경우 "_날짜"를 제외한 순수 파일명)을 비교하여 같은 경우만 recover의 후보가 됨
                위 두 조건을 && 조건으로 충족할 경우, 링크드 리스트에 해당 파일 정보(백업 된 경로, 복구할 경로, 해시 값)을 저장
            */
            if (strcmp(HASH, "md5") == 0) {
                strcpy(hash_recoverpath, md5(recoverpath));
                strcpy(hash_backupedpath, md5(backupedpath));
                if (strcmp(hash_recoverpath, hash_backupedpath) != 0 && strcmp(fname_recoverpath, dname_without_date) == 0) { // 해시값 다르고 && 이름 같으면 insert
                    insert_node(head, recoverpath, backupedpath, hash_backupedpath, ++idx);
                }
		    }
		    else if (strcmp(HASH, "sha1") == 0) {
                strcpy(hash_recoverpath, sha1(recoverpath));
                strcpy(hash_backupedpath, sha1(backupedpath));
                if (strcmp(hash_recoverpath, hash_backupedpath) != 0 && strcmp(fname_recoverpath, dname_without_date)) { // 해시값 다르고 && 이름 같으면 insert
                    insert_node(head, recoverpath, backupedpath, hash_backupedpath, ++idx);
                }
		    }
#ifdef DEBUG
            // printf("%s insert_node 완료\n============\n", backupedpath);
#endif

            
#ifdef DEBUG
            // printf("without_date : %s\n", without_date);
            // printf("backuppedpath : %s\n", backupedpath);
#endif
        }
        ////////////////for문 끝////////////////////////

#ifdef DEBUG
printf("\n\n\n");
        print_node(head);
printf("\n\n\n");
#endif
        /*
            위 for문을 빠져나온 뒤, 만약 링크드 리스트에 저장된 노드가 2개 이상(idx > 1)인 경우, 백업된 파일이 여러개라는 뜻임.
            (즉, 파일명은 같지만 내용이 달라 "_날짜" 정보가 다른 파일이 두 개 이상 존재)
            따라서 그 경우, 백업 파일 리스트를 출력해 사용자로부터 번호를 입력 받아 선택된 백업 파일을 복구함.
        */
        int input_num = 0;
        while (1) {
            if (idx > 1) {
                printf("backup file list of \"%s\"\n", recoverpath);
                printf("0. exit\n");
                print_node(head);
                printf("Choose file to recover\n>> ");
                
                scanf("%d", &input_num);

                // 0번 입력 시 명령어 실행 종료 후 프롬프트 재출럭
                if (input_num == 0) 
                    break;
            }
            else input_num = 1;

            // printf("inputnum : %d\n\n", input_num);
            Node *selected = search_node(head, input_num);
            if (selected == NULL) {
                printf("nothing to recover\n");
                break;
            }
            if ((selected->backupedpath) == NULL) {
                fprintf(stderr, "search error\n");
                break;
            }
            recover(selected->backupedpath, recoverpath, head);
            remove_all(head);
            break;
        }

        // namelist 메모리 해제
        for (index = 0; index < count; index++) {
            free(namelist[index]);
        }
        free(namelist);
	}
}

// 20211431> recover pathname -d
// pathname : /Users/yujimin/backup/P1/a 처럼 backup 경로가 들어오는 걸로..

/*
    void recover_dir(char *backuped_dir_path, char *recover_dir_path, Node *head);
    -d 옵션을 시용해 recover 명령어를 실행했을 때 호출되는 함수
    첫 번째 인자 backuped_dir_path : 백업 되어 있는 디렉토리의 절대 경로 (이하 백업 디렉토리라고 칭함)
    두 번째 인자 recover_dir_path : 복구할 파일의 절대 경로
    세 번째 인자 head : 링크드 리스트의 첫 번째 노드
*/
void recover_dir(char *backuped_dir_path, char *recover_dir_path, Node *head) {

#ifdef DEBUG
    printf("\n### recover_dir() 실행\n");
    printf("첫 번째 인자 backuped_dir_path는 완성된 백업디렉터리의 폴더 절대경로\n");
    printf("두 번째 인자 recover_dir_path는 완성된 복구할 폴더 절대경로\n");
    printf(" =>=> %s\t\t %s\n", backuped_dir_path, recover_dir_path);
    printf("recover_ : %s\n", recover_dir_path);
#endif

    struct stat statbuf;
	char resolvedpath[PATH_SIZE];	// realpath() 결과 저장  (ex. "/Users/yujimin/P1/a")
    char *dname;                    // 최하위 폴더명 저장
    char backupedpath[PATH_SIZE];    // dirpath + fname     (ex. "/Users/yujimin/backup/P1/a")
    char absolutepath[PATH_SIZE];   // dirpath + fname에서 backup 빠진 경로     (ex. "/Users/yujimin/P1/a")

    // recover 예외 처리 1 : 해당 백업 디렉토리가 없을 경우 -> Usage 출력
	if(lstat(backuped_dir_path, &statbuf) == -1) {
        printf("Usage : <FILENAME> [OPTION]\n");
        printf("    -d : recover directory recursize\n");
        printf("    -n <NEWNAME> : recover filse with new name\n");
        exit(1);
	}

    // 존재하는 백업 디렉토리일 경우 
	if (realpath(backuped_dir_path, resolvedpath) != NULL || is_file_only_in_backupdir == true) {
        char *recover_tmp_without_date = malloc(sizeof(char) * PATH_SIZE);
        struct dirent **namelist;
        char *filelist[PATH_SIZE];          // 백업파일끼리 중복 있는지 확인하기 위한 변수
        char *duplicated_list[PATH_SIZE];   // 백업파일끼리 중복 있는지 확인하기 위한 변수
        int count = 0;
        int index = 0;
        // if(lstat(backuped_dir_path, &statbuf) > 0) {           // 존재할 때
#ifdef DEBUG
		printf("\n\n=========\n존재하는 백업 디렉터리\n");
        // printf("recover할 디렉토리 절대 경로 : %s\n", recoverpath);    // /Users/yujimin/P1/a
        // printf("recover할 디렉토리 백업 경로 기본 : %s\n", backupedpath);	// /Users/yujimin/backup/a
        // printf("resolved == absolute : %s\n", absolutepath);		// /Users/yujimin/P1/a
        printf("Directory Lists\n");
#endif
            
        // 백업 디렉토리에 존재하는 파일or폴더 개수 count에 저장
        if((count = scandir(backuped_dir_path, &namelist, NULL, alphasort)) == -1){
            printf("Usage : <FILENAME> [OPTION]\n");
            printf("    -d : recover directory recursize\n");
            printf("    -n <NEWNAME> : recover filse with new name\n");
            exit(0);
        }
#ifdef DEBUG
        printf("cnt : %d\n", count);
#endif

        
        // duplicated_list 배열 메모리 할당
        for (int k = 0; k < count; k++) {
            duplicated_list[k] = malloc(sizeof(char) * PATH_SIZE);
        }

        // 백업 디렉토리에 존재하는 파일들을 배열에 저장 (중복 백업된 파일 리스트 확인하기 위함)
        for (index = count - 1; index >= 0; index--) {
            filelist[index] = malloc(sizeof(char) * PATH_SIZE);
            strcpy(filelist[index], namelist[index]->d_name);
            // printf("laskdfjklsdjflkasdf : %s\n", filelist[index]);
        }

/////////////////////////////
        /* for 문 시작 *///////////////////
///////////////////////////
        for (index = count - 1; index >= 0; index--) {
            bool is_duplicated = false;     // 백업 디렉토리 내부에 중복된 파일이 존재하는지 여부 저장
            char recover_tmp[PATH_SIZE];     // 복구할 파일 경로 저장할 임시 변수
            char backuped_tmp[PATH_SIZE];   // 백업된 파일 경로 저장할 임시 변수
            idx = 0;
#ifdef DEBUG
            printf("===\nfor문 index : %d, 파일or디렉명 : %s\n", index, namelist[index]->d_name);
#endif
            // "." 와 "..", 그리고 숨김 파일은 중복 확인 대상 아님
            if(strcmp(namelist[index]->d_name, ".") == 0 || strcmp(namelist[index]->d_name, "..") == 0 || namelist[index]->d_name[0] == '.')
                continue;
            
            // recover_tmp_without_date에 "_날짜"를 제외한 절대 경로 저장
            strcpy(recover_tmp, recover_dir_path);
            strcat(recover_tmp, "/");
            strcat(recover_tmp, namelist[index]->d_name);
            int i = strlen(recover_tmp) - 1;
            while (i >= 0 && recover_tmp[i] != '_') 
                i--;
            recover_tmp_without_date = strndup(recover_tmp, i);


            // backuped_tmp에 폴더 탐색하며 발견한 파일명 붙여 절대 경로 환성
            strcpy(backuped_tmp, backuped_dir_path);
            strcat(backuped_tmp, "/");
            strcat(backuped_tmp, namelist[index]->d_name);

#ifdef DEBUG
            printf("recover_tmp_without_date : %s\n", recover_tmp_without_date);
            printf("backuped_tmp : %s\n", backuped_tmp);
#endif

            // 백업 디렉토리끼리 중복 있는지 확인 -> 있으면 선택
            char namelist_check[PATH_SIZE];
            strcpy(namelist_check, namelist[index]->d_name);
            i = strlen(namelist_check) - 1;
            while (i >= 0 && namelist_check[i] != '_') 
                i--;
            char *namelist_without_date = strndup(namelist_check, i);

            // 중복 체크된 파일인지 확인. 중복이면 continue
            bool dup_flag = false;
            for (int k = 0; k < count; k++) {
                if (strcmp(duplicated_list[k], namelist_without_date) == 0) {
                    // printf("이미 중복처리됨. pass\n");
                    dup_flag = true;
                    break;
                }
            }
            if (dup_flag == true) continue;
            
            /*
                for문을 돌며, 백업 디렉토리 내에 존재하는 중복된 파일을 찾음. 만약 중복된 파일이 존재할 경우,
                is_duplicated = true; 를 저장하여 추후 사용자가 중복 파일 중 복구할 파일을 직접 선택할 수 있도록 리스트를 출력함
            */
            for (i = 0; i < count; i++) {
                char filelist_check[PATH_SIZE];     // 중복된 파일1 ("_날짜" 포함) 저장
                char file_backuped_tmp[PATH_SIZE];      // 백업된 파일의 절대 경로 저장

                strcpy(filelist_check, filelist[i]);       
                int j = strlen(filelist_check) - 1;
                while (j >= 0 && filelist_check[j] != '_') 
                    j--;
                char *filelist_without_date = strndup(filelist_check, j);   // 중복된 파일1 ("_날짜" 미포함) 저장

                // 백업된 파일의 절대 경로 저장
                strcpy(file_backuped_tmp, backuped_dir_path);
                strcat(file_backuped_tmp, "/");
                strcat(file_backuped_tmp, filelist_check);

#ifdef DEBUG
                printf("########\ni : %s -> %s\nj : %s->%s\n##########\n", namelist_check, namelist_without_date, filelist_check, filelist_without_date);
#endif
               
                // 날짜까지 똑같은 동일 파일이 아니고, 날짜만 다르게 백업된 중복된 백업파일임을 확인했을 때 -> 선택해야함
                if (strcmp(namelist_check, filelist_check) != 0 && strcmp(namelist_without_date, filelist_without_date) == 0) {
                    is_duplicated = true;
                    strcpy(duplicated_list[i], namelist_without_date);
                    
                    // for문을 돌며 발견된 중복 백업 파일들을 링크드 리스트에 저장
                    if (strcmp(HASH, "md5") == 0) {
                        insert_node(head, recover_tmp_without_date, file_backuped_tmp, md5(backuped_tmp), ++idx);
                    }
                    else if (strcmp(HASH, "sha1") == 0) {
                        insert_node(head, recover_tmp_without_date, file_backuped_tmp, sha1(backuped_tmp), ++idx);
                    }
                }
            }
            // 중복된 백업 파일들이 발견 되었을 경우, 비교 기준이 된 중복 파일도 링크드 리스트에 저장
            if (is_duplicated == true) {
                if (strcmp(HASH, "md5") == 0) {
                        insert_node(head, recover_tmp_without_date, backuped_tmp, md5(backuped_tmp), ++idx);
                }
                else if (strcmp(HASH, "sha1") == 0) {
                    insert_node(head, recover_tmp_without_date, backuped_tmp, sha1(backuped_tmp), ++idx);
                }
            }                
            
            // backuped_tmp가 디렉토리 일 경우, 추가 탐색이 필요하므로 recover_dir()을 재귀적으로 호출
            lstat(backuped_tmp, &statbuf);
            if (S_ISDIR(statbuf.st_mode) == 1) {
#ifdef DEBUG
               printf("\"%s\"는 디렉이므로 recover_dir() 재귀 호출\n", backuped_tmp);
               
#endif
                recover_dir(backuped_tmp, recover_dir_path, head);
            }
            // backuped_tmp가 파일일 경우 일 경우, 중복 파일이 존재할 경우 선택해서 recover() 호출해 최종 복구
            else {
#ifdef DEBUG
               printf("\"%s\"는 파일이므로 바로 중복 확인 후, 골라서, recover() 호출\n", recover_tmp_without_date);
               printf("backuped : %s\n", backuped_tmp);
#endif

                // 복구할 경로에 파일이 존재하지 않을 경우, 생성해야하므로 is_file_only_in_backupdir 설정해주기
                FILE* fpr;
                if ((fpr = fopen(recover_tmp_without_date, "r")) == NULL) {
                    is_file_only_in_backupdir = true;
                }

                if (is_duplicated == false)  {
                    recover(backuped_tmp, recover_tmp_without_date, head);
                }
                // 백업 디렉토리에 중복된 백업 파일 존재하는 경우 최종적으로 복구할 파일 선택
                else {  
                    int input_num = 0;
                    printf("backup file list of \"%s\"\n", recover_tmp_without_date);
                    printf("0. exit\n");
                    print_node(head);
                    printf("Choose file to recover\n>> ");
                    
                    scanf("%d", &input_num);

                    // 0번 입력 시 명령어 실행 종료 후 프롬프트 재출럭
                    if (input_num != 0) {
                        Node *selected = search_node(head, input_num);
                        if (selected == NULL) {
                            printf("nothing to recover\n");
                        }
                        if ((selected->backupedpath) == NULL) {
                            fprintf(stderr, "search error\n");
                        }
                        recover(selected->backupedpath, recover_tmp_without_date, head);
                        // 사용한 링크드 리스트 초기화
                        remove_all(head);
                    }
                }
            }
        }

        // namelist 메모리 해제
        for (index = 0; index < count; index++) {
            free(namelist[index]);  
        }
        free(namelist);
	}

}

/*
    void recover(char *backuppedpath, char *recoverpath, Node *head);
    최종적으로 존재 여부를 확인한, 백업되어있는 절대 경로만을 전달 받아 복구 진행
    첫 번째 인자 backuppedpath : 백업 디렉토리에 존재하는, 복구할 파일의 절대 경로
    두 번째 인자 recoverpath : 복구 될 파일의 절대 경로 (덮어 쓰여지거나, 새로 생성됨)
*/
void recover(char *backuppedpath, char *recoverpath, Node *head) {
    FILE* fpr; 
	FILE* fpw;

    // case 1 : 백업 복구하려는 파일이 사용자의 홈 디렉토리에 있고, 백업디렉토리에 백업파일이 있는 경우
    if (is_file_only_in_backupdir == false) {

        // 해시값 같으면 복구 안함
		char hash_recoverpath[PATH_SIZE];
		char hash_backupedpath[PATH_SIZE];
        if (strcmp(HASH, "md5") == 0) {
			strcpy(hash_recoverpath, md5(recoverpath));
			strcpy(hash_backupedpath, md5(backuppedpath));

			if (strcmp(hash_recoverpath, hash_backupedpath) == 0)
				return;
		}
		else if (strcmp(HASH, "sha1") == 0) {
			strcpy(hash_recoverpath, md5(recoverpath));
			strcpy(hash_backupedpath, md5(backuppedpath));
			if (strcmp(hash_recoverpath, hash_backupedpath) == 0) {
				return;
			}
		}

        // 기존에 존재하는 파일 삭제하고 새롭게 복구 될 파일 덮어쓰기
        if (remove(recoverpath) < 0) {
            fprintf(stderr, "remove file error for %s\n", recoverpath);
            exit(1);
        }
        if ((fpr = fopen(backuppedpath, "r")) == NULL) {
            fprintf(stderr, "fopen error for \"%s\"\n", backuppedpath);
            exit(1);
        }
        else if ((fpw = fopen(recoverpath, "w")) == NULL) {
            fprintf(stderr, "fopen error for \"%s\"\n", recoverpath);
            exit(1);
	    }
        int a;
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

        printf("\"%s\" backup recover to \"%s\"\n", backuppedpath, recoverpath);
        return;
    }
    // case 2 : 백업 복구하려는 파일이 사용자의 홈 디렉토리에 없지만 백업디렉토리에는 백업파일이 있는 경우
    else {
#ifdef DEBUG
        printf("\n== 2. 백업 복구하려는 파일이 사용자의 홈 디렉토리에 없지만 백업디렉토리에는 백업파일이 있는 경우===\n");
        printf("backuppedpath : %s\n recoverpath : %s\n", backuppedpath, recoverpath);
#endif
        DIR *dirp;
        char tmp[PATH_SIZE];
        strcpy(tmp, recoverpath);
        char *fname = strtok(tmp, "/");
        fname = strtok(NULL, "/");
        fname = strtok(NULL, "/");
        char folderpath[PATH_SIZE];
        strcpy(folderpath, HOME);
        while (1) {
            strcat(folderpath, "/");
            strcat(folderpath, fname);  
            if ((dirp = opendir(folderpath)) == NULL) {
                fname = strtok(NULL, "/");

                if (fname != NULL) {
                    mkdir(folderpath, 0777);
                }
                else {
                    break;
                }
            }
            else {
                fname = strtok(NULL, "/");
            }
        }
        if ((fpr = fopen(backuppedpath, "r")) == NULL) {
            fprintf(stderr, "fopen error for \"%s\"\n", backuppedpath);
            exit(1);
        }
        else if ((fpw = fopen(recoverpath, "w")) == NULL) {
            fprintf(stderr, "fopen error for \"%s\"\n", recoverpath);
            exit(1);
	    }
        int a;
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

        printf("\"%s\" backup recover to \"%s\"\n", backuppedpath, recoverpath);
        return;
    }
  

}

 
/*
    char *check_file_only_in_backupdir(char *subpath, char *fname);
    파일이 백업 될 파일 경로에는 존재하지 않고, 오직 백업 디렉토리에만 있는지 확인하는 함수
    오직 백업 디렉토리에만 존재할 경우, 해당 경로를 리턴하고, 아닐 경우 NULL을 반환
    첫 번째 인자 subpath : 확인할 파일의 상위 디렉토리 절대 경로 (ex. "/Users/yujimin/backup/P1")
    두 번째 인자 fname : 파일명 (ex. "a.txt")
*/
char *check_file_only_in_backupdir(char *subpath, char *fname) {
#ifdef DEBUG
    printf("### check_file_only_in_backupdir() 실행\n");
    printf("subpth : %s\npathname : %s\n", subpath, fname);
#endif
    char backuped_check[PATH_SIZE]; // 현재 존재하지 않는 recover경로가 backupped 경로에는 있는지 확인
    concat_path(backuped_check, subpath, fname);
#ifdef DEBUG
    printf("backuped_check : %s\n", backuped_check);
#endif

    // recover <FILENAME> 에서 <FILENAME>을 절대경로로 바꾼 결과. backup폴더에 존재할 경우 이 경로가 리턴됨
    char *realpath = malloc(sizeof(char) * PATH_SIZE);
    concat_path(realpath, PWD, fname);  // /Users/yujimin/P1/a.txt
#ifdef DEBUG
    printf("realpath : %s\n", realpath);
#endif

    struct dirent **namelist;
    int count;
    int index;

    //예외처리
    if((count = scandir(subpath, &namelist, NULL, alphasort)) == -1)
        return NULL;
#ifdef DEBUG
    printf("count : %d\n", count);
#endif
    for(index = count - 1; index >= 0; index--) {
        // "." 와 "..", 그리고 숨김 파일은 check 대상 아님
        if(strcmp(namelist[index]->d_name, ".") == 0 || strcmp(namelist[index]->d_name, "..") == 0 || namelist[index]->d_name[0] == '.')
            continue;

        // check 후보 파일 절대 경로를 tmp에 저장 (ex. "/Users/yujimin/backup/P1/a.txt_232323232323")
        char tmp[PATH_SIZE];
        concat_path(tmp, subpath, namelist[index]->d_name);

        // 폴더일 경우 continue
        lstat(tmp, &statbuf);
        if (S_ISDIR(statbuf.st_mode) == 1)
            continue;

        // _시간 떼고 비교해서 같으면, 백업 복구하려는 파일이 사용자의 홈 디렉토리에 없지만 백업 디렉토리에는 백업 파일이 있는 경우
        int i = strlen(tmp) - 1;
        while (i >= 0 && tmp[i] != '_') 
            i--;
        char *dateinfo = strndup(tmp, i);
#ifdef DEBUG
        printf("datainfo : %s\n", dateinfo);
#endif
        
        if (strcmp(backuped_check, dateinfo) == 0) {   
#ifdef DEBUG
            printf("백업 복구하려는 파일이 사용자의 홈 디렉토리에 없지만 백업 디렉토리에는 백업 파일이 있는 경우!!\n");
#endif
            is_file_only_in_backupdir = true;
            break;
        }
    }
    // namelist 메모리 해제
    for (index = 0; index < count; index++) {
        free(namelist[index]);
    }
    free(namelist); 

    if (is_file_only_in_backupdir == true)
         return realpath;
    else {
        free(realpath);
        return NULL;
    }
        
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