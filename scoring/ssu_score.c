#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
// #include <sys/time.h>
// #include <getopt.h>
#include <time.h>
#include <signal.h>
#include "ssu_score.h"
#include "blank.h"

// 아래에서 새로 선언되므로 extern으로 선언된 아래 두 문장은 무시됨
extern struct ssu_scoreTable score_table[QNUM];		// 학생의 성적정보(qname, score)을 저장하는 ssu_scoreTable 구조체 배열
extern char id_table[SNUM][10];						// 학생들의 id 저장하는 2차원 배열 

struct ssu_scoreTable score_table[QNUM];			// 학생의 성적정보(qname, score)을 저장하는 ssu_scoreTable 구조체 배열
char id_table[SNUM][10];							// 학생들의 id 저장하는 2차원 배열

Qnode *student[SNUM];		// 학생 개인의 문제 채점 정보(문제이름, 점수)를 링크드 리스트로 관리하기 위한 구조체 배열 변수

char stuDir[BUFLEN];		// 채점 대상이 될 학생이 제출한 답안 디렉토리 (이하 제출dir)
char ansDir[BUFLEN];		// 채점 기준이 될 정답이 있는 디렉토리 (이하 정답dir)
char errorDir[BUFLEN];		// -e 옵션 입력 시 <DIRNAME>에 저장 될 디렉토리명
char threadFiles[ARGNUM][FILELEN];	// -t 옵션 사용 시 [QNAMES...]에 해당하는 문제 번호 저장
char cpIDs[ARGNUM][10];		// -c 옵션 or -p 옵션 입력 시 가변인자 [STUDENTIDS ...]에 해당하는 학번 저장
char csv_fname[MAXLEN];	// 채점 결과 테이블 파일 생성 ("score.csv"가 기본. -n 옵션 추가 시 사용자 지정)

int nOption = false;	// -n option
int mOption = false;	// -m option
int cOption = false;	// -c option
int pOption = false;	// -p option
int tOption = false;	// -t option
int sOption = false;	// -s option
int eOption = false;	// -e option

int cnum;				// cOPtion 활성화 되어있을 때 가변인자로 입력된 학번 개수
int pnum;				// pOPtion 활성화 되어있을 때 가변인자로 입력된 학번 개수
int tnum;				// tOPtion 활성화 되어있을 때 가변인자로 입력된 문제번호 개수
int sort;				// -s <CATEGORY> <1|-1> 에서 <1|-1>에 해당하는 값
char category[FILELEN] = "";	// -s <CATEGORY> <1|-1> 에서 <CATEGORY>에 해당하는 값

Qnode *shead;	// sOption 활성화 되어있을 때 링크드 리스트 생성을 위한 새로운 노드 생성
Qnode *scur;	// 헤드 노드는 학번만 저장. "문제,점수" 정보는 head 노드의 다음 노드부터 저장하고 head는 빈 노드로 유지

/*
	void ssu_score(int argc, char *argv[]);	
	함수 설명 : main함수로부터 인자를 넘겨 받아 옵션, 정답Dir, 제출Dir 경로 확인 및 table 초기화 등 채점을 위한 기초 작업 수행
	인자 int argc : 매개변수 개수
	인자 char *argv[] : 매개변수 저장된 포인터 배열
*/ 
void ssu_score(int argc, char *argv[])
{
	// 채점 시 open error 출력하지 않도록 redirection
	// int fd_null = open("/dev/null", O_WRONLY);
	// dup2(fd_null, 2);
	// close(fd_null);

	char saved_path[BUFLEN];	// 현재 작업 디렉토리 경로 저장할 변수
	int i;

	// 인자 중에 -h 옵션이 있으면 usage 출력 후 종료
	for(i = 0; i < argc; i++){
		if(!strcmp(argv[i], "-h")){		// 인자 중에 -h가 있으면fr
			print_usage();				// usage 출력하고
			exit(0);					// 프로그램 종료
		}
	}

	memset(saved_path, 0, BUFLEN);	// saved_path 0으로 초기화
	if(argc >= 3) {		// 인자 제대로 입력한 경우 ($ ssu_score <STD_DIR> <ANS_DIR> [OPTION])
		strcpy(stuDir, argv[1]);	// <STD_DIR>인자로 들어온 문자열을 제출dir에 복사
		strcpy(ansDir, argv[2]);	// <ANS_DIR>인자로 들어온 문자열을 정답dir에 복사
	}

	if(!check_option(argc, argv)) {	// 옵션 확인. 없는 옵션 입력 시 프로그램 종료
		fprintf(stderr, "없는 옵션 입력. 프로그램 종료\n");
		exit(1);
	}
	
	if (cnum && pnum) {	// -p 옵션과 -c 옵션을 함께 사용했을 때, 가변인자 그룹이 두 번 나오는 경우
		// printf("가변인자 그룹이 두 번 나오는 경우\n");
		fprintf(stderr, "[STUDENTIDS ...] 가변인자 그룹은 -c 옵션 혹은 -p 옵션 뒤에 한 번만 나와야 함\n");
		exit(1);
	}
	else {	// -c 옵션 혹은 -p 옵션 뒤에 가변인자 그룹이 한 번만 나온 경우 -> 어느 옵션 뒤에 가변인자가 입력 되었는지 확인하여 cnum과 pnum 값 통일
		if (cnum) {
			if (cnum > 5)	// 가변인자 개수가 5개를 초과한 경우, 나머지는 무시
				cnum = 5;
			pnum = cnum;	
		}
			
		else if (pnum) {
			if (pnum > 5)	// 가변인자 개수가 5개를 초과한 경우, 나머지는 무시
				pnum = 5;
			cnum = pnum;
		}
			
	}

	getcwd(saved_path, BUFLEN);		// saved_path에 현재 작업 디렉토리 경로 저장

	// 제출dir의 존재 여부 확인
	if(chdir(stuDir) < 0){
		// 제출Dir 존재하지 않으면 에러메세지 출력 후 종료
		fprintf(stderr, "%s doesn't exist\n", stuDir);	
		return;
	}
	getcwd(stuDir, BUFLEN);		// stuDir에 제출dir 경로 저장

	chdir(saved_path);		// 다시 '현재 작업 디렉토리 경로'로 작업 디렉토리 변경

	// 정답dir의 존재 여부 확인
	if(chdir(ansDir) < 0){
		// 정답Dir 존재하지 않으면 에러메세지 출력 후 종료
		fprintf(stderr, "%s doesn't exist\n", ansDir);
		return;
	}
	getcwd(ansDir, BUFLEN);		// ansDir에 정답dir 경로 저장

	chdir(saved_path);		// 다시 '현재 작업 디렉토리 경로'로 작업 디렉토리 변경

	set_scoreTable(ansDir);	// scoreTable 초기화
	set_idTable(stuDir);	// idTable 초기화

	// nOption이 활성화 되지 않았을 때 csv_fname을 ""./ANS_DIR/score.csv" 로 지정
	if (!nOption)
		sprintf(csv_fname, "%s/score.csv", ansDir);
	// printf("csv_fname : %s\n", csv_fname);exit(0);
		
	// mOption이 활성화 되었을 때
	if(mOption)
		do_mOption();

	// sOption이 활성화 되었을 때
	if (sOption) {
		shead = (Qnode*)malloc(sizeof(Qnode));	// sOption 활성화 되어있을 때 링크드 리스트 생성을 위한 새로운 노드 생성
		scur = shead;	// 헤드 노드는 학번만 저장. "문제,점수" 정보는 head 노드의 다음 노드부터 저장하고 head는 빈 노드로 유지
		student[0] = shead;
	}

	printf("grading student's test papers..\n");
	score_students();	// 채점 함수 실행

	// if(iOption)
		// do_iOption(iIDs);

	return;
}

// 옵션 확인 (-h 제외한 나머지 옵션 -n, -m, -c, -p, -t, -s, -e 중 하나이어야 함)
int check_option(int argc, char *argv[])
{
	int i, j, k;
	int c;
	int exist = 0;
	
	// char csvfilename[FILELEN] = "";		// -n <CSVFILENAME>
	// char category[FILELEN] = "";		// -s <CATEGORY> <1|-1> 에서 <CATEGORY>에 해당하는 값 -> 전역 변수로 뺌
	// int sort = 0;		// -s <CATEGORY> <1|-1> 에서 <1|-1>에 해당하는 값 -> 전역 변수로 뺌
	char dirname[FILELEN] = "";			// -e <DIRNAME>

	// printf("check_option() : %d\n", argc);
	// for (int i = 0; i < argc; i++) {
	// 	printf("argv[%d] : %s\n", i, argv[i]);
	// }

	optind = 3;		// argv[0] = "ssu_score", argv[1] = <STD_DIR>, argv[2] = <ANS_DIR> 이므로 옵션 처리해야하는 최초 인덱스인 3으로 optind 설정
	while ((c = getopt(argc, argv, "n:mcpts:e:h1")) != -1)
	{
		// printf("c : %c\n", c);
		// printf("## optind : %d\n", optind);
		// printf("optarg opind opterr optopt\n %c %d %d %d\n", *optarg, optind, opterr, optopt);
		switch(c) {
			case 'n':
				// printf("-n\n");
				nOption = true;
				strcpy(csv_fname, optarg);
				char *ext = strrchr(csv_fname, '.'); // 파일 확장자 위치
            	if (!ext || strcmp(ext, ".csv")) {	// 예외 처리 : 파일 확장자가 .csv 아닌 경우
					fprintf(stderr, "file extension error : \"%s\" is not \".csv\"\n", csv_fname);
					exit(1);
				}
				// printf("csvfilename : %s\n", csvfilename);
				break;

			case 'm':
				// printf("-m\n");
				mOption = true;
				break;

			case 'c':
				// printf("-c\n");
				cOption = true;
				i = optind;
				// j = 0;	
				cnum = 0;	// [STUDENTIDS]에 해당하는 가변인자 개수 저장
				// TODO : -c [STUDENTIDS...]
				while (i < argc && argv[i][0] != '-') {
					// STUDENTIDS 저장
					// if ((j + 1) > 5) {	// 예외 처리 : 가변인자 개수가 5개를 초과할 경우 아래 메세지 출력 후 수행에 반영 안 하도록 break
					// if ((cnum + 1) > 5) {	// 예외 처리 : 가변인자 개수가 5개를 초과할 경우 아래 메세지 출력 후 수행에 반영 안 하도록 break
					if (cnum >= ARGNUM)	{ // 예외 처리 : 가변인자 개수가 5개를 초과할 경우 아래 메세지 출력 후 수행에 반영 안 하도록 함
						if (cnum == ARGNUM)		// 초과된 가변인자 중, 6번째로 들어온 가변인자일 경우
							printf("Maximum Number of Argument Exceeded. :: %s", argv[i]);
						else	// 초과된 가변인자 중, 7번째 이상의 나머지 가변인자일 경우
							printf(" %s", argv[i]);
						if (i+1 == argc || argv[i+1][0] == '-') { 	// 초과된 가변인자 중, 마지막 가변인자일 경우
							printf("\n");
							// break;
						}
					}
					else {
						// strcpy(cpIDs[j], argv[i]);
						strcpy(cpIDs[cnum], argv[i]);
						// printf("cpIDs[%d] : %s\n", cnum, cpIDs[cnum]);
						
					}
					i++;
					cnum++;		// 가변 인자로 입력된 학생 수 세기
					// j++;
				}
				// for (int i = 0; i < j; i++) printf("cOption_studentIds[%d] : %s\n", i, cpIDs[i]);
				// exit(0);
				optind = i;		// 가변인자 이후 다른 옵션 입력 받기 위해 다음 번에 처리될 옵션의 인덱스 갱신
				break;

			case 'p':
				// printf("-p\n");
				pOption = true;
				i = optind;
				pnum = 0;

				// TODO : -p [STUDENTIDS...]
				while (i < argc && argv[i][0] != '-') {
					// STUDENTIDS 저장
					if (pnum >= ARGNUM)	{ // 예외 처리 : 가변인자 개수가 5개를 초과할 경우 아래 메세지 출력 후 수행에 반영 안 하도록 함
						if (pnum == ARGNUM)		// 초과된 가변인자 중, 6번째로 들어온 가변인자일 경우
							printf("Maximum Number of Argument Exceeded. :: %s", argv[i]);
						else	// 초과된 가변인자 중, 7번째 이상의 나머지 가변인자일 경우
							printf(" %s", argv[i]);
						if (i+1 == argc || argv[i+1][0] == '-') { 	// 초과된 가변인자 중, 마지막 가변인자일 경우
							printf("\n");
							// break;
						}
					}
					else {
						// strcpy(cpIDs[j], argv[i]);
						strcpy(cpIDs[pnum], argv[i]);
						// printf("cpIDs[%d] : %s\n", pnum, cpIDs[pnum]);
					}
					i++;
					pnum++;		// 가변 인자로 입력된 학생 수 세기
					// j++;
				}
				optind = i;
				break;

			case 't':
				// printf("-t\n");
				tOption = true;
				i = optind;
				// printf("optind : %d\n", i);
				tnum = 0;

				// -t [QNAMES...] 형태로 입력 받은 문제 번호(QNAMES)들을 threadFiles 배열에 저장 (컴파일 시 -lpthread 옵션 추가하기 위함)
				while (i < argc && argv[i][0] != '-') {
					// if (j >= ARGNUM)	// 예외 처리 : 가변인자 개수가 5개를 초과할 경우 아래 메세지 출력하여 수행에 반영 안 하도록 함
					// 	printf("Maximum Number of Argument Exceeded.  :: %s\n", argv[i]);
					if (tnum >= ARGNUM)	{ // 예외 처리 : 가변인자 개수가 5개를 초과할 경우 아래 메세지 출력 후 수행에 반영 안 하도록 함
						if (tnum == ARGNUM)		// 초과된 가변인자 중, 6번째로 들어온 가변인자일 경우
							printf("Maximum Number of Argument Exceeded. :: %s", argv[i]);
						else	// 초과된 가변인자 중, 7번째 이상의 나머지 가변인자일 경우
							printf(" %s", argv[i]);
						if (i+1 == argc || argv[i+1][0] == '-') { 	// 초과된 가변인자 중, 마지막 가변인자일 경우
							printf("\n");
							// break;
						}
					}					
					else {
						strcpy(threadFiles[tnum], argv[i]);
						// printf("threadFiles[%d] : %s\n", j, threadFiles[j]);
					}
					i++; 
					tnum++;
				}
				optind = i;
				break;

			case 's':
				// printf("-s\n");
				sOption = true;
				strcpy(category, optarg);
				// printf("category : %s\n", category);
				// 예외 처리 : <CATEGORY> 자리에 stdid나 score 외에 다른 입력이 들어온 경우
				if (strcmp(category, "stdid") && strcmp(category, "score")) {
					fprintf(stderr, "첫 번째 인자로 stdid나 score 외에 다른 입력이 들어옴 :: %s\n", category);
					exit(1);
				}
				// printf("argv[optind] : %s\n", argv[optind]);
				// 예외 처리 : <1|-1> 자리에 아무 인자도 들어오지 않거나, <1|-1>이 아닌 경우
				if (argv[optind] == NULL || (strcmp(argv[optind], "1") && strcmp(argv[optind], "-1"))) {
					fprintf(stderr, "두 번째 인자로 1 혹은 -1 외에 다른 입력이 들어옴 :: %c\n", optopt);
					exit(1);
				}
				else {
					sort = atoi(argv[optind]);
					// printf("## sort : %d\n", sort);
					// printf("## <CATEGORY> : %s\n", category);
				}
				break;
			
			case 'e':
				// printf("-e\n");
				eOption = true;
				strcpy(errorDir, optarg);	// 첫 번째 인자로 입력 받은 <DIRNAME>을 errorDir 변수에 복사
				char tmp[BUFLEN];	// errorDir의 절대 경로 저장하기 위한 임시 변수
				
				strcpy(tmp, errorDir);	// 임시 변수에 realpath()하기 전의 errorDir 문자열 저장
				// -e <DIRNAME> 옵션 사용했을 경우 상대경로 or 절대경로 모두 처리해 주기 위해 realpath() 함수 사용
				if ((realpath(tmp, errorDir)) < 0) {
					// 절대경로 or 상대경로가 아닐 경우 에러메세지 출력 후 프로그램 종료
					fprintf(stderr, "realpath error for %s\n", tmp);
					exit(1);
				}

				if (access(errorDir, F_OK) < 0)	{	// <DIRNAME> 폴더가 없을 경우
					// printf("폴더 없음\n");
					mkdir(errorDir, 0755);	// 폴더 생성
				}
				else {		// 폴더가 있을 경우
					// printf("폴더 있음\n");
					rmdirs(errorDir);	// 같은 에러 디렉토리에 대해서 덮어씌울 경우에는 안에 내용 삭제해야 함
					// printf("rmdir 성공\n");
					mkdir(errorDir, 0755);	// 다시 에러 파일 만듦
					// printf("mkdir 성공\n");
				}
				break;

			case '1' :
				// -s 옵션에서 입력된 숫자 -1을 옵션으로 처리하지 않도록 함
				break;

			case '?':	// 올바르지 않음 옵션 입력된 경우
				printf("Unknown option %c\n", optopt);
				return false;
		}
	}

	return true;
}

// void do_iOption(char (*ids)[FILELEN])
// {
// 	FILE *fp;
// 	char tmp[BUFLEN];
// 	char qname[QNUM][FILELEN];
// 	char *p, *id;
// 	int i, j;
// 	char first, exist;

// 	if((fp = fopen("./score.csv", "r")) == NULL){
// 		fprintf(stderr, "score.csv file doesn't exist\n");
// 		return;
// 	}

// 	// get qnames
// 	i = 0;
// 	fscanf(fp, "%s\n", tmp);
// 	strcpy(qname[i++], strtok(tmp, ","));
	
// 	while((p = strtok(NULL, ",")) != NULL)
// 		strcpy(qname[i++], p);

// 	// print result
// 	i = 0;
// 	while(i++ <= ARGNUM - 1)
// 	{
// 		exist = 0;
// 		fseek(fp, 0, SEEK_SET);
// 		fscanf(fp, "%s\n", tmp);

// 		while(fscanf(fp, "%s\n", tmp) != EOF){
// 			id = strtok(tmp, ",");

// 			if(!strcmp(ids[i - 1], id)){
// 				exist = 1;
// 				j = 0;
// 				first = 0;
// 				while((p = strtok(NULL, ",")) != NULL){
// 					if(atof(p) == 0){
// 						if(!first){
// 							printf("%s's wrong answer :\n", id);
// 							first = 1;
// 						}
// 						if(strcmp(qname[j], "sum"))
// 							printf("%s    ", qname[j]);
// 					}
// 					j++;
// 				}
// 				printf("\n");
// 			}
// 		}

// 		if(!exist)
// 			printf("%s doesn't exist!\n", ids[i - 1]);
// 	}

// 	fclose(fp);
// }

// M - Option을 처리해주는 함수
// 점수 테이블 파일 내에 특정 번호의 배점을 수정해준다.
// no를 입력할때까지 수정 가능.
// 예외처리 : 구현되어 있지 않음.
// Input 없음
// 리턴 없음
/*
	void do_mOption();
	함수 설명 : -m 옵션 활성화 되어있을 때 실행되는 함수로, 점수 테이블 파일 내의 특정 문제 번호의 배점 수정. "no"를 입력해 함수 종료할 수 있음
*/
void do_mOption()
{
	double newScore;
	char modiName[FILELEN];
	char filename[MAXLEN];
	char *ptr;
	int i;

	ptr = malloc(sizeof(char) * FILELEN);

	// 예외 처리 - 점수 테이블 파일이 존재하지 않을 경우 에러 처리 후 프로그램 종료
	
	char tmp[MAXLEN];	// score_table.csv 경로 저장할 변수
	// 점수 테이블 파일은 "./ANS/score_table.csv" 이름으로 생성해야 함. ANS는 ANS_DIR 디렉토리라고 해석함.
	sprintf(tmp, "%s/score_table.csv", ansDir);
	if(access(tmp, F_OK) != 0) {
		fprintf(stderr, "score table file(ex. \"%s\") does not exist!\n", tmp);
		exit(1);
	}

	while(1){
		
		printf("Input question's number to modify >> ");
		scanf("%s", modiName);

		if(strcmp(modiName, "no") == 0)
			break;

		for(i=0; i < sizeof(score_table) / sizeof(score_table[0]); i++){
			strcpy(ptr, score_table[i].qname);
			ptr = strtok(ptr, ".");
			if(!strcmp(ptr, modiName)){
				printf("Current score : %.2f\n", score_table[i].score);
				printf("New score : ");
				scanf("%lf", &newScore);
				getchar();
				score_table[i].score = newScore;
				break;
			}
		}
	}

	sprintf(filename, "%s", csv_fname);
	write_scoreTable(filename);
	free(ptr);

}

/*
	int is_exist(char (*src)[FILELEN], char *target);
	함수 설명 : -c 혹은 -p 옵션이 활성화 되어있을 때, 가변인자로 들어온 학번과 실제 STD_DIR에 해당 학번이 존재하는지 확인
			STD_DIR 디렉토리 내에 존재하는 학번은 id_table에 저장될 것이므로 id_table과 target을 비교
	인자 cahr(*src)[10] : 가변인자가 저장 된 cpIDs배열
	인자 char *target : id_table의 학번들과 비교해서 존재여부를 확인할, 가변인자로 입력 받은 학번 
*/
int is_exist(char(*src)[10], char *target)
{
	int i = 0;
	while(1)
	{
		// // 예외처리 : 가변인자 개수가 5개가 넘어갈 경우 false 리턴
		// if(i > ARGNUM)	
		// 	return false;
		// printf("id_table[%d] : %s\t\t", i, cpIDs[i]);
		

		if(!strcmp(src[i], ""))	// id_table의 마지막까지 검색했는지 확인
			return false;	// 존재하지 않는 경우이므로 false 리턴
		else if(!strcmp(src[i++], target))	// 존재하는지 확인
			return true;	// 존재하므로 true return
	}	
	return false;	// 존재하지 않으므로 false 리턴
}

/*
	int qname_exist(struct ssu_scoreTable *src, char *target);
	함수 설명 : -t 옵션이 활성화 되어있을 때, 가변인자로 들어온 문제 번호가 실제 score_table에 존재하는지 확인
	인자 struct ssu_scoreTable *src : score_table 배열
	인자 char *target : 가변인자로 들어온 문제 번호 (threadFiles 배열에 저장되어 있다)
*/
int qname_exist(struct ssu_scoreTable *src, char *target)
{
    int i = 0;
    while (1)
    {
        if (!strcmp(src[i].qname, ""))	// score_table의 마지막까지 검색했는지 확인
            return false;	// 존재하지 않을 경우 false 리턴
        else {
            char *ext = strrchr(src[i].qname, '.'); // 파일 확장자 위치
            if (ext && !strcmp(ext, ".c")) { // 파일 확장자가 .c인 경우
                char tmp[FILELEN];
                strncpy(tmp, src[i].qname, ext - src[i].qname); // 파일 이름을 복사
				// printf("tmp : %s\n", tmp);
				// printf("qname : %s\n\n", src[i].qname);
                tmp[ext - src[i].qname] = '\0'; // 복사한 이름 끝에 NULL 삽입
                if (!strcmp(tmp, target))	// 가변인자로 들어온 문제 번호와 일치하는 문제 존재 여부 검색
                    return true;	// 일치할 경우(존재할 경우) true 리턴
            }
        }
        i++;	// 인덱스 증가
    }
    return false;	// 존재하지 않을 경우 false 리턴
}


/*
	void set_scoreTable(char *ansDir);
	함수 설명 : score_table.csv 파일의 존재 여부 확인 후,
		 존재할 경우 -> 파일 내용을 읽어 score_table 구조체 배열에 저장
		 존재하지 않을 경우 -> score_table 구조체 배열과 score_table.csv 파일을 생성 및 완성
	인자 char *ansDir : 정답dir의 절대 경로 (ex. "/Users/yujimin/scoreprogram/ANS_DIR")
*/
void set_scoreTable(char *ansDir)
{
	char filename[FILELEN];	// score_table.csv 경로 저장할 변수

	// 점수 테이블 파일은 "./ANS/score_table.csv" 이름으로 생성해야 함. ANS는 ANS_DIR 디렉토리라고 해석함.
	sprintf(filename, "%s/score_table.csv", ansDir);

	// 점수 테이블 파일이 "현재 작업 디렉토리 경로"에 존재하는지 확인
	if(access(filename, F_OK) == 0)	// /ANS_DIR/score_table.csv 파일이 존재할 경우
		read_scoreTable(filename);	// score_table.csv 파일을 읽어와 내용(문제번호, 점수)를 score_table 구조체 배열에 저장 후 배열 정렬
	else {	// 존재하지 않는 경우 "score_table.csv" 이름으로 점수 테이블 파일 새로 생성
		make_scoreTable(ansDir);	// score_table 구조체 배열 생성. 각 문제 번호에 점수 설정한 뒤 정렬
		write_scoreTable(filename);	// score_table.csv 생성. score_table 구조체 배열의 정보(문제 번호, 점수)를 파일에 write
	}

	// -t 옵션 예외 처리 : 첫 번째 인자로 입력 받은 문제 번호가 없는 문제 번호인 경우 에러 처리 후 프로그램 종료
	if (((tOption && tnum)) && !qname_exist(score_table, threadFiles[0])) {
		fprintf(stderr, "첫 번째 인자로 입력 받은 문제 번호가 없는 문제 번호임\n");
		exit(1);
	}
}

/*
	void read_scoreTable(char *path);
	함수 설명 : score_table.csv가 있는 경우 실행 되며, score_table.csv 파일을 읽어와 score_table 구조체 배열에 문제이름(qname)과 점수(score)을 저장
	인자 char *path : score_table.csv 파일의 절대경로 (ex. "/Users/yujimin/scoreprogram/ANS_DIR/score_table.csv")
*/
void read_scoreTable(char *path)
{
	FILE *fp;	// 파일 스트림 포인터
	char qname[FILELEN];	// 문제 이름 저장 배열
	char score[BUFLEN];		// 점수 저장 배열
	int idx = 0;

	// 점수 테이블 파일을 읽기 모드로 열기
	if((fp = fopen(path, "r")) == NULL){
		// 열기 실패 시 에러메세지 출력 후 함수 종료
		fprintf(stderr, "file open error for %s\n", path);	
		return ;
	}

	// 점수 테이블 파일에서 문제이름(qname)과 점수(score)을 읽어와 score_table에 저장
	while(fscanf(fp, "%[^,],%s\n", qname, score) != EOF){
		strcpy(score_table[idx].qname, qname);
		score_table[idx++].score = atof(score);
	}

	fclose(fp);
}

/*
	void make_scoreTable(char *ansDir);
	함수 설명 : score_table.csv가 없는 경우 실행되며, score_table 구조체 배열 생성 후 각 문제 번호에 점수 설정
	인자 char *ansDir : 정답dir의 절대 경로 (ex. "/Users/yujimin/scoreprogram/ANS_DIR")
*/ 
void make_scoreTable(char *ansDir)
{
	int type, num;	// type : .txt or .c (파일 타입), num : 1 or 2 (점수 설정 방식)
	double score, bscore, pscore;	// score : 테이블에 저장할 점수, bscore : blank score(빈칸 채우기 문제 점수), pscore : program score(프로그래밍 문제 점수)
	struct dirent *dirp;	// dp가 가리키는 디렉토리 탐색하면서 내부 파일 혹은 디렉토리를 가리키는 스트림 포인터
	DIR *dp;	// 디렉토리 스트림 포인터
	int idx = 0;	// 문제수
	int i;

	num = get_create_type(ansDir);		// 표준 입력으로 점수 설정 방식(1 or 2) 입력 받음

	// "1. input blank question and program question's score. ex) 0.5 1" 선택
	if(num == 1)
	{
		// 빈칸 채우기 문제 점수를 bscore에 입력 받음
		printf("Input value of blank question : ");
		scanf("%lf", &bscore);		
		
		// 프로그래밍 문제 점수를 pscore에 입력 받음
		printf("Input value of program question : ");
		scanf("%lf", &pscore);
	}

	// 정답dir 열기
	if((dp = opendir(ansDir)) == NULL){
		// 정답dir 열기 실패 시 에러문 호출 후 종료
		fprintf(stderr, "open dir error for %s\n", ansDir);	
		return;	
	}

	// 정답dir 안에 있는 모든 파일과 디렉토리 탐색하면서 ".txt"파일과 ".c"파일만 점수 테이블에 추가
	while((dirp = readdir(dp)) != NULL){

		// "."과 ".."는 pass
		if(!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."))
			continue;

		// 파일 타입 검사. ".txt"와 ".c"가 아닐 경우 pass
		if((type = get_file_type(dirp->d_name)) < 0)
			continue;

		// ".txt" or ".c" 파일을 점수 테이블에 추가
		strcpy(score_table[idx].qname, dirp->d_name);

		// 인덱스++;
		idx++;
	}

	closedir(dp);	// 정답dir 닫기
	sort_scoreTable(idx);	// score_table 구조체 배열 정렬

	// 정렬된 점수 테이블의 문제번호(qname)에 사용자로부터 입력 받은 점수(score) 설정
	for(i = 0; i < idx; i++)
	{
		type = get_file_type(score_table[i].qname);	// type에 qname(문제 이름)의 타입 (.txt or .c) 저장

		// 1번 선택 시 "빈칸 채우기 문제.txt"에는 bscore를, "프로그램 작성 문제.c"에는 pscore를 
		// 각 문제 번호(score_tabel[i])에 해당하는 점수(score)에 일괄적으로 설정
		if(num == 1)
		{
			// .txt 파일일 경우 빈칸 채우기 문제이므로 score에 bscore 저장
			if(type == TEXTFILE)
				score = bscore;
			// .c 파일일 경우 프로그래밍 문제이므로 score에 pscore 저장
			else if(type == CFILE)
				score = pscore;
		}
		// 2번 선택 시 각 문제마다 점수를 입력 받아 설정
		else if(num == 2)
		{
			printf("Input of %s: ", score_table[i].qname);
			scanf("%lf", &score);
		}
		// 최종적으로 table에 점수 저장
		score_table[i].score = score;
	}
}

/*
	void write_scoreTable(char *filename);
	함수 설명 : score_table.csv가 없는 경우 실행되며, 점수 테이블 파일을 생성하고 score_table 구조체 배열의 문제 이름(qname)과 점수(score)을 파일에 write함
	인자 char *filename : 생성할 score_table.csv 파일의 절대 경로 (ex. "/Users/yujimin/scoreprogram/ANS_DIR/score_table.csv")
*/
void write_scoreTable(char *filename)
{
	// printf("%s\n", filename);exit(0);
	int fd;	// score_table.csv 파일 가리킬 파일 디스크립터
	char tmp[BUFLEN];	// "문제이름,점수" 형태의 문자열을 저장할 임시 변수
	int i;
	int num = sizeof(score_table) / sizeof(score_table[0]);	// 테이블 행 수
	// printf("ceat -> %s\n", filename);exit(0);

	// 인자로 전달 받은 경로에 score_table.csv 파일 생성
	if((fd = creat(filename, 0666)) < 0){
		// 파일 생성 실패 시 에러 메세지 출력 후 종료
		fprintf(stderr, "creat error for %s\n", filename);	
		return;
	}

	// score_table.csv 파일에 "문제이름,점수" 형태의 문제 정보 저장. 문제수(num) 만큼 반복
	for(i = 0; i < num; i++)
	{
		// 문제의 점수가 0점이면 반복문 탈출
		if(score_table[i].score == 0)
			break;

		sprintf(tmp, "%s,%.2f\n", score_table[i].qname, score_table[i].score);	// "문제이름,점수" 형태로 tmp에 저장
		write(fd, tmp, strlen(tmp));	// "문제이름,점수" 형태의 문자열을 fd가 가리키는 score_table.csv 파일에 write
	}

	close(fd);	// 파일 닫기
}

/*
	void set_idTable(char *stuDir);
	함수 설명 : STD_DIR 디렉토리를 탐색하며 학생들이 제출한 답안 폴더(ex. "20200001")들을 id_table 구조체에 추가
	인자 cahr *stuDir : 제출dir의 절대 경로 (ex. "/Users/yujimin/scoreprogram/STD_DIR")
*/
void set_idTable(char *stuDir)
{
	struct stat statbuf;	// 파일 상태 저장하기 위한 stat 구조체 변수
	struct dirent *dirp;	// dp가 가리키는 디렉토리 탐색하면서 내부 파일 혹은 디렉토리를 가리키는 스트림 포인터
	DIR *dp;	// 디렉토리 스트림 포인터
	char tmp[BUFLEN];	// dp가 가리키는 디렉토리 내 파일의 절대 경로 저장하기 위한 임시 변수
	int num = 0;	// id_table 행 수

	// STD_DIR 디렉토리 열어서 dp가 가리키도록 함
	if((dp = opendir(stuDir)) == NULL){
		// 실패 시 에러 메세지 출력 수 프로그램 종료
		fprintf(stderr, "opendir error for %s\n", stuDir);	
		exit(1);
	}

	// dp가 가리키는 디렉토리(STD_DIR) 내부의 파일 or 디렉토리 탐색
	while((dirp = readdir(dp)) != NULL){
		// "." or ".."일 경우 pass
		if(!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."))
			continue;

		sprintf(tmp, "%s/%s", stuDir, dirp->d_name);	// tmp에 STD_DIR 폴더 내 파일의 절대 경로 저장
		stat(tmp, &statbuf);	// tmp의 stat를 statbuf에 저장

		// tmp가 디렉토리인지 확인
		if(S_ISDIR(statbuf.st_mode))
			strcpy(id_table[num++], dirp->d_name);	// 디렉토리일 경우 id_table에 해당 디렉토리명(ex. "20200001") 저장
		else
			continue;	// 디렉토리 아닐 경우 continue
	}
	closedir(dp);	// 디렉토리 닫기

	// -c 옵션 or -p 옵션 예외 처리 : 첫 번째 인자로 입력 받은 학생이 <STD_DIR>에 없는 경우 에러 처리 후 프로그램 종료
	if (((cOption && cnum) || (pOption && pnum)) && !is_exist(id_table, cpIDs[0])) {
		fprintf(stderr, "첫 번째 인자로 입력 받은 학생이 <STD_DIR>에 존재하지 않음\n");
		exit(1);
	}

	sort_idTable(num);	// id_table 구조체 배열 정렬
}

/*
	void sort_idTable(int size);
	함수 설명 : id_table 구조체 배열 정렬
	인자 int size : id_table 행 수
*/
void sort_idTable(int size)
{
	int i, j;
	char tmp[10];

	// 오름차순 정렬
	for(i = 0; i < size - 1; i++){
		for(j = 0; j < size - 1 -i; j++){
			if(strcmp(id_table[j], id_table[j+1]) > 0){
				strcpy(tmp, id_table[j]);
				strcpy(id_table[j], id_table[j+1]);
				strcpy(id_table[j+1], tmp);
			}
		}
	}
}

/*
	void sort_scoreTable(int size);
	함수 설명 : score_table(점수 테이블) 구조체 배열 완성 후에 실행되며, 점수 테이블을 정렬함
	인자 int size : 문제 수
*/
void sort_scoreTable(int size)
{
	int i, j;
	struct ssu_scoreTable tmp;	// 점수 테이블 구조체 임시 변수
	int num1_1, num1_2;		// 문제 번호1, 서브문제 번호1
	int num2_1, num2_2;		// 문제 번호2, 서브문제 번호2

	for(i = 0; i < size - 1; i++){
		for(j = 0; j < size - 1 - i; j++){

			// 연속한 두 문제 각각의 문제 번호와 서브문제 번호를 분리 (ex. qname이 "16-3.txt"라면 최종적으로 num1 = 16, num2 = 3이 됨)
			get_qname_number(score_table[j].qname, &num1_1, &num1_2);
			get_qname_number(score_table[j+1].qname, &num2_1, &num2_2);

			// 문제 번호와 서브 문제를 비교해 swap함으로써 최종적으로 score_table이 문제 번호를 기준으로 오름차순으로 정렬 되도록 함
			if((num1_1 > num2_1) || ((num1_1 == num2_1) && (num1_2 > num2_2))){

				memcpy(&tmp, &score_table[j], sizeof(score_table[0]));
				memcpy(&score_table[j], &score_table[j+1], sizeof(score_table[0]));
				memcpy(&score_table[j+1], &tmp, sizeof(score_table[0]));
			}
		}
	}


}

/*
	void get_qname_number(char *qname, int *num1, int *num2);
	함수 설명 : 문제 이름(qname)의 문제 번호와 서브문제 번호를 분리 해주는 함수. (ex. qname이 "16-3.txt"라면 최종적으로 num1 = 16, num2 = 3이 됨)
	인자 char *gname : 문제 이름
	인자 int *num1 : 함수가 끝나면 문제 번호('-'를 기준으로 앞의 숫자)가 저장 됨
	인자 int *num2 : 함수가 끝나면 서브문제 번호('-'를 기준으로 뒤의 숫자)가 저장 됨
*/
void get_qname_number(char *qname, int *num1, int *num2)
{
	char *p;	// strtok함수에서 문자를 가리킬 포인터 변수
	char dup[FILELEN];	// strtok 함수에 의해 qname이 손상 되지 않도록 임시 변수 사용

	strncpy(dup, qname, strlen(qname));		// qname이 손상 되지 않도록 qname을 dup에 복사
	*num1 = atoi(strtok(dup, "-."));	// '-' or '.' 전의 문자열(문제 번호)을 정수 타입으로 바꿔 num1에 저장
	
	p = strtok(NULL, "-.");		// 다음으로 등장하는 '-' or '.'을 NULL로 바꿈
	
	if(p == NULL)
		*num2 = 0;	// 서브문제가 없으면 0번 째 서브문제로 간주해 0을 num2에 저장
	else
		*num2 = atoi(p);	// 서브문제가 있으면 해당 번호를 num2에 저장
}


/*
	int get_create_type(char *ansDir);
	함수 설명 : 점수테이블파일(score_table.csv) 존재하지 않는 경우에 실행되며, 사용자로부터 파일 생성 형태를 입력 받음
	인자 char *ansDir : 정답dir의 절대 경로 (ex. "/Users/yujimin/scoreprogram/ANS_DIR")
*/
int get_create_type(char *ansDir)
{
	int num;	// 입력 받을 번호 (1 or 2)

	while(1)
	{
		printf("score_table.csv file doesn't exist in \"%s\"!\n", ansDir);
		if (mOption)
			exit(1);
		// 1. "빈칸 채우기 문제"와 "프로그램 작성 문제"에 각각 점수 일괄 설정
		printf("1. input blank question and program question's score. ex) 0.5 1\n");
		// 1. 모든 문제에 따로 점수 설정
		printf("2. input all question's score. ex) Input value of 1-1: 0.1\n");
		// 사용자로부터 번호 입력 받아 num에 저장
		printf("select type >> ");
		scanf("%d", &num);

		// 1번과 2번 중에 선택하지 않으면 메세지 출력 후 다시 입력 받도록 함
		if(num != 1 && num != 2) {
			printf("not correct number!\n");
			while(getchar() != '\n'); // 버퍼에 남아있는 값 제거
		}
		else	// 1번과 2번 중에 선택했을 때, while문 탈출
			break;
	}

	return num;	// 입력 받은 번호 리턴
}

/*
	void score_students();
	함수 설명 : 채정 결과 테이블(score.csv)를 생성하여 학생들에게 성적 매기고 전체 학생의 점수 평균을 출력함
*/
void score_students()
{
	double score = 0;	// 모든 학생들의 점수 합을 저장
	int num;	// 학생 인덱스 (몇 번째 학생인지 저장)
	int i, j;
	// int cnum = 0; // -c 옵션과 함께 가변인자로 입력된 학생 수
	// int tmpnum = 0;	// 학생의 점수 합산하기 전에 저장할 임시 변수
	int fd;		// score.csv 파일을 가리키는 파일 디스크립터
	char tmp[BUFLEN];	// "학번," 형식으로 score.csv 파일에 추가하기 위한 임시 변수
	int size = sizeof(id_table) / sizeof(id_table[0]);		// 학생 수

	// 학생들의 성적 저장할 score.csv 생성(파일 유무와 관계 없이 새로 생성)
	// -n <CSVFILENAME> 옵션 사용했을 경우 상대경로 or 절대경로 모두 처리해 주기 위해 realpath() 함수 사용
	strcpy(tmp, csv_fname);
	if ((realpath(tmp, csv_fname)) < 0) {
		// 절대경로 or 상대경로가 아닐 경우 에러메세지 출력 후 프로그램 종료
		fprintf(stderr, "realpath error for %s\n", tmp);
		exit(1);
	}
	// printf("realpth : %s \t\t\t %s\n", csv_fname, tmp);exit(0);
	// printf("%s\n",tmp);exit(0);
	// if((fd = creat("./score.csv", 0666)) < 0){
	if((fd = creat(csv_fname, 0666)) < 0){
		// creat 실패 시 에러 메세지 출력 후 함수 종료
		fprintf(stderr, "creat error for %s\n", csv_fname);	
		return;
	}

	write_first_row(fd);	// header row 생성 (1.1.txt ... sum)

	// 학생들의 점수 데이터를 score.csv에 추가
	for(num = 0; num < size; num++)
	{
		// 학번 저장된 id_table의 num번째 행이 빈칸일 경우, 더이상 채점할 학생이 존재하지 않는 것으로 판단하여 for문 탈출
		if(!strcmp(id_table[num], ""))
			break;

		sprintf(tmp, "%s,", id_table[num]);		// "학번," 형식으로 임시 변수에 저장	

		if (!sOption) 	// sOption이 활성화 되지 않았을 경우
			write(fd, tmp, strlen(tmp)); 	// "학번," 형식의 문자열을 score.csv 파일에 씀
		else {	// sOption이 활성화 되었을 경우	
			// Qnode *shead = (Qnode*)malloc(sizeof(Qnode));	// sOption 활성화 되어있을 때 링크드 리스트 생성을 위한 새로운 노드 생성
			// Qnode *scur = shead;	// 헤드 노드는 학번만 저장. "문제,점수" 정보는 head 노드의 다음 노드부터 저장하고 head는 빈 노드로 유지
			student[num] = (Qnode*)malloc(sizeof(Qnode));
			shead = student[num];
			scur = student[num];
			strcpy(scur->id, tmp);	// student 구조체 배열의 학번(id) 멤버 변수에 학번 저장 (ex. "20200001,")
			// printf("scur->id : %s\n", scur->id);exit(0);
		}

		// sturent[num] = id;

		score += score_student(fd, id_table[num]);	// 학생 개인의 문제 채점 결과를 score.csv 파일에 추가하며 파일을 완성하고 총점 합산
	}

	
	if (cOption && cnum)	// cOption이 활성화 되고, 첫 번째 인자가 생략 되지 않은 경우
		printf("Total average : %.2f\n", score / cnum);	// 가변인자로 입력된 학생 점수의 평균 출력
	else if (cOption && !cnum)		// cOption이 활성화 되고, 첫 번째 인자가 생량된 경우 
		printf("Total average : %.2f\n", score / num);	// 모든 학생들의 점수 평균 출력

	printf("result saved.. (%s)\n", realpath(csv_fname, tmp));
	
	if (eOption)
		printf("error saved.. (%s)\n", realpath(errorDir, tmp));


	if (sOption) {	// sOption 활성화 된 경우
		// printf("정렬 함수 호출\n");
		// printf("학생 수 %d\n", num);
		sort_student_by(num, category, sort);
		// printf("정렬 함수  ㄲ ㅡ ㅌ\n");
		for (int i = 0; i < num; i++) {
			// printf("학번 : %s, 총점 : %.2f\n", student[i]->id, student[i]->score);
		}

		for(i = 0; i < num; i++) {	// 학생 수만큼 반복
			// scur = student[i];	// scur 초기화
			scur = student[i]; // 학생의 링크드 리스트의 시작 노드를 scur로 지정
			// printf("%s 학번 write 시작\n", student[i]->id);
			
		// while (scur != NULL) {
			write(fd, student[i]->id, strlen(student[i]->id));	// "학번,"을 score.csv 파일에 write
			// scur->next = student[i]->next;
			scur = student[i]->next;
			int qsize = sizeof(score_table) / sizeof(score_table[0]);	// 문제 수
			for (j = 0; j < qsize; j++) {	// 학생 개인의 링크드 리스트를 돌며 점수를 score.csv 파일에 저장
			// while (scur != NULL) {
				// printf("%s 문제 %.2f 점\n", scur->qname, scur->score);
				if (scur->score == 0) { // 0점일 경우
					write(fd, "0,", 2);		// "0," 형태로 score.csv 파일에 write
				}
				else {		// 0점이 아닐 경우
					sprintf(tmp, "%.2f,", scur->score);		// "점수," 형태로 tmp에 저장
					write(fd, tmp, strlen(tmp));	// "점수," 형태로 score.csv 파일에 write
				}
				if (scur->next == NULL) {	// 다음 노드가 빈 노드일 때
				// if (j + 1 == qsize) {	// 마지막 문제 끝
					// printf("마지막 노드 작성 끝. 총점은 %.2f\n", student[i]->score);
					sprintf(tmp, "%.2f\n", student[i]->score);	// "총점," 형태로 tmp에 저장
					write(fd, tmp, strlen(tmp));	// "총점"을 score.csv 파일에 write
					break;
				}
				scur = scur->next;	// 다음 노드
				// scur->next = NULL;
			}
			// exit(0);

			// }

		}

	}
	

	close(fd);		// score.csv 파일 닫기
}

/*
	double score_student(int fd, char *id);
	함수 설명 : 학생 개인에게 각 문제를 채점한 점수(빈칸채우기문제, 프로그래밍문제)를 부여해 score.csv에 채졈 결과를 추가하며 파일을 완성하고 총점을 리턴
*/
double score_student(int fd, char *id)
{
	int type;	// 문제 타입(.txt or .c)
	double result;		// 각 문제의 채점 결과(true(맞음) or false(틀림)) 저장
	double score = 0;	// 문제의 총점 저장
	int i;
	char tmp[MAXLEN];	// "파일의 절대경로" or "점수," 형식의 문자열을 임시 저장할 변수
	int size = sizeof(score_table) / sizeof(score_table[0]);	// 문제 수

	
	Qnode *phead = (Qnode*)malloc(sizeof(Qnode));	// pOption 활성화 되어있을 때 wrong problem 출력 위한 새로운 노드 생성
	Qnode *pcur = phead;	// phead(헤드 노드)는 빈 노드로 유지

	// 문제 수만큼 반복하며 score.csv 파일에 점수 추가
	for(i = 0; i < size ; i++)
	{
		// 점수 테이블의 i번째 행에 점수가 0일 경우, 더이상 데이터가 없는 것이므로 반복문 탈출 
		if(score_table[i].score == 0) {
			// printf("(i:%d) for문 탈출 1\n", i);
			pcur = NULL;
			break;
		}
			// break;

		// 학생이 제출한 답안 파일의 절대 경로를 tmp에 저장 (ex. "/Users/yujimin/scoreprogram/STD_DIR/20200001/1-1.txt")
		sprintf(tmp, "%s/%s/%s", stuDir, id, score_table[i].qname);

		// tmp 파일의 존재 여부 확인
		if(access(tmp, F_OK) < 0)
			result = false;		// 파일이 존재하지 않을 경우 result에 false(틀림) 저장
		else
		{
			// 문제 타입 확인
			if((type = get_file_type(score_table[i].qname)) < 0) {
				// printf("for문 탈출 2\n");
				continue;	// .txt or .c가 아닐 경우 pass
			}
			// 문제 타입이 .txt일 경우와 .c일 경우로 나누어 채점
			if(type == TEXTFILE)	// .txt일 경우
				result = score_blank(id, score_table[i].qname);		// 빈칸채우기문제 채점. 맞으면 true, 틀리면 false을 result에 저장
			else if(type == CFILE)	// .c일 경우
				result = score_program(id, score_table[i].qname);	// 프로그래밍문제 채점. 맞으면 true, 틀리면 false을 result에 저장
		
			if (sOption) {		// sOption 활성화 되어있을 경우
				scur->next = (Qnode*)malloc(sizeof(Qnode));		// 문제이름, 점수를 저장할 노드 생성
				scur = scur->next;
				scur->next = NULL;

				strcpy(scur->id, id);	// 현재 노드에 학번 저장
				strcpy(scur->qname, score_table[i].qname);		// 현재 노드에 문제이름 저장
			}

		}



		// result값 을 확인해 해당 문제가 맞았는지 틀렸는지에 따라 분기
		if(result == false) {		// 틀렸을 경우
			pcur->next = (Qnode*)malloc(sizeof(Qnode));	// 링크드 리스트에 틀린 문제 추가하기 위한 노드 생성
			pcur = pcur->next;	

			if (!sOption) 	// sOption이 활성화 되지 않았을 경우
				write(fd, "0,", 2);		// score.csv 파일에 점수 입력
			else {		// sOption이 활성화 되어있을 경우	
				scur->score = 0;	// 현재 노드에 점수 저장
				// printf("%s 문제 틀림. %f 점 \n", scur->qname, scur->score);
			}

			// 링크드 리스트에 틀린 문제 추가
			// strcpy(cur->qname, memcscore_table[i].qname);
			memcpy(pcur->qname, score_table[i].qname, strlen(score_table[i].qname) - strlen(strrchr(score_table[i].qname, '.')));	// 확장자 제외한 문제이름 링크드 리스트에 추가
			pcur->score = score_table[i].score;	// 배점 링크드 리스트에 추가
			// printf("size(%d)/i(%d) 틀린문제 : %s \n", size, i, cur->qname);
			// printf("$$ %s(%.2f)\n", cur->qname, cur->score);

			// cur->next = (Qnode*)malloc(sizeof(Qnode));
			// cur = cur->next;
			pcur->next= NULL;
			
		}
		else{	// 맞거나 감점 되었을 경우
			if(result == true){		// 맞았을 경우
				score += score_table[i].score;	// 총점에 해당 문제 점수 합산
				sprintf(tmp, "%.2f,", score_table[i].score);	// tmp에 "점수," 형식으로 받은 점수를 저장
			}
			else if(result < 0){	// 감점 되었을 경우
				score = score + score_table[i].score + result;	// 총점에서 감점된 만큼 점수를 뺀 후 합산
				sprintf(tmp, "%.2f,", score_table[i].score + result);	// tmp에 "점수," 형식으로 받은 점수를 저장
			}

			if (!sOption) 	// sOption이 활성화 되지 않았을 경우
				write(fd, tmp, strlen(tmp));	// "점수," 형식의 문자열을 score.csv 파일에 추가
			else {		// sOption이 활성화 되어있을 경우
				if (result == true) {	// 맞았을 경우
					scur->score = score_table[i].score;		// 현재 노드에 점수 저장
					// printf("%s 문제 맞음. %f 점 \n", scur->qname, scur->score);
				}
				else if (result < 0) {	// 감점 되었을 경우
					scur->score = score_table[i].score + result;	// 현재 노드에 점수 저장
					// printf("%s 문제 맞음. %f 점 \n", scur->qname, scur->score);
				}
			}
		}

		
	}

	// printf("%s is finished. score : %.2f\n", id, score); 	// 학번과 해당 학생이 받은 총점 출력
	printf("%s is finished.. ", id); 	// 학생 개인의 답안이 채점 완료 되었음을 학번과 함께 출력
	
	sprintf(tmp, "%.2f\n", score);		// 총점을 "총점\n" 형식으로 tmp에 저장
	if (!sOption)	// sOption이 활성화 되지 않은 경우
		write(fd, tmp, strlen(tmp));		// "총점\n" 형식의 문자열을 score.csv 파일에 추가
	else {		// sOption이 활성화 되어있을 경우
		// printf("총점 : %.2f\n", score);
		shead->score = score;	// 총점을 shead 노드에 저장
	}

	// soption 제대로 들어갔나 확인..
	// if (sOption) {
	// 	// scur = shead;
	// 	scur = student[0];
	// 	printf("학번 : %s, 총점 : %f\n", scur->id, scur->score);
	// 	scur = scur->next;
	// 	while (scur != NULL) {
	// 		if (scur->next == NULL) {
	// 			printf("%s(%.2f)", scur->qname, scur->score);
	// 			break;
	// 		}
	// 		printf("%s(%.2f), ", scur->qname, scur->score);
	// 		scur = scur->next;
	// 	}
	// }

	// cOption 활성화 된 경우 
	if (cOption) {
		// return do_cOption(score, id);	// 학생 개인의 score 출력 여부를 결정하는 함수 호출
		double sc = do_cOption(score, id);	// 학생 개인의 score 출력 여부를 결정하는 함수 호출

		// pOption 활성화 된 경우
		if (pOption) {
			do_pOption(phead->next, id);		// 링크드 리스트 출력
		}

		printf("\n");
		return sc;
	}
	else if (pOption)	// pOption "만" 활성화 된 경우
		do_pOption(phead->next, id);
	printf("\n");
	return 0;	// 0 리턴
}

/*
	double do_cOption(double score, char* id);
	함수 설명 : cOption이 활성화 되어있을 때, score_student()함수의 끝에서 학생 개인 채점이 완료된 뒤에 실행 되는 함수. 가변 인자의 입력 여부에 따라 학생 개인의 점수를 출력하고 리턴함
	인자 double score : 학생 개인이 얻은 점수
	인자 char *id : 학생 개인의 학번
*/
double do_cOption(double score, char* id) {
	if (!cnum) {	// 첫 번째 입력 생략 시
		printf("score : %.2f", score);	// 모든 학생에 대해 결과 출력
		return score;	// 모든 학생이 받은 점수 리턴
	}
	else if (cnum && is_exist(cpIDs, id)) {	// 가변인자 입력 시 
		printf("score : %.2f", score);	// 가변인자로 입력 된 학생들의 점수 출력
		return score;	// 해당 학생이 받은 점수 리턴
	}
	// else
	// 	printf("\n");
	return 0;
}

/*
	void do_pOption(Qnode *cur);
	함수 설명 : pOption이 활성화 되었을 때, 가변인자 수에 따라 올바른 출력 함수(print_wrong_problem)로 분기
	인자 Qnode *cur : 첫 번째 틀린 문제 정보가 저장된 노드
	인자 char *id : 학생 개인의 학번
*/
void do_pOption(Qnode *cur, char* id) {
	if (!pnum) {	// 첫 번째 입력 생략 시
		print_wrong_problem(cur, id);
	}
	else if (pnum && is_exist(cpIDs, id)) {
		print_wrong_problem(cur, id);
	}
}

/*
	void print_wrong_problem(Qnode *cur, char* id);
	함수 설명 : pOption이 활성화 되었을 때, 학생의 "틀린문제번호(배점)" 형태의 링크드 리스트를 출력
	인자 Qnode *cur : 첫 번째 틀린 문제 정보가 저장된 노드
	인자 char *id : 학생 개인의 학번
*/
void print_wrong_problem(Qnode *cur, char* id) {
	if (cOption) {
		printf(", ");
	}
	printf("wrong problem : ");
	while (cur != NULL) {
		if (cur->next == NULL) {
			printf("%s(%.2f)", cur->qname, cur->score);
			break;
		}
		printf("%s(%.2f), ", cur->qname, cur->score);
		cur = cur->next;
	}
}

/*
	void sort_student_by(int size, char *sort_key, int sort_order);
	함수 설명 : sOption이 활성화 되어있을 때, <CATEGORY>에 입력 된 "id" 혹은 "score"을 기준으로 student 구조체 배열 sorting
	인자 int size : 학생 수
	인자 char *sort_key : 정렬의 기준이 되는 "id" or "score"
	인자 int sort_order : 1일 경우 오름차순 정렬, -1일 경우 내림차순 정렬
*/
void sort_student_by(int size, char *sort_key, int sort_order) {
	// printf("size : %d, sort_key : %s, sort_order : %d\n", size, sort_key, sort_order);
    for (int i = 0; i < size - 1; i++) {
		// printf("i : %d\n", i);
        int min_index = i;
        for (int j = i + 1; j < size; j++) {
			// printf("j : %d\n", j);
			// printf("%s%.2f vs %s:%.2f\n", student[j]->id, student[j]->score, student[min_index]->id, student[min_index]->score);
			// 정렬 기준과 오름/내림차순을 판단하는 조건문
            if ((strcmp(sort_key, "id") == 0 && sort_order == 1 && strcmp(student[j]->id, student[min_index]->id) < 0) ||
                (strcmp(sort_key, "id") == 0 && sort_order == -1 && strcmp(student[j]->id, student[min_index]->id) > 0) ||
                (strcmp(sort_key, "score") == 0 && sort_order == 1 && student[j]->score < student[min_index]->score) ||
                (strcmp(sort_key, "score") == 0 && sort_order == -1 && student[j]->score > student[min_index]->score)) {
                min_index = j;
            }
        }
        if (min_index != i) { // swap
			Qnode *temp = (Qnode*)malloc(sizeof(Qnode));
            temp = student[i];
            student[i] = student[min_index];
            student[min_index] = temp;
        }
    }
}

/*
	void write_first_row(int fd);
	함수 설명 : 학생들의 성적을 저장할 score_table.csv 파일에 header row 생성 (1.1.txt ... sum)
	인자 int fd : 파일 디스트립터
*/
void write_first_row(int fd)
{
	int i;
	char tmp[BUFLEN];	// "문제이름," 형식으로 저장하기 위한 임시 변수
	int size = sizeof(score_table) / sizeof(score_table[0]);	// 문제 수

	write(fd, ",", 1);	// score_table.csv 파일의 첫 번째 열은 학번 쓸 공간이므로 pass하기 위해 ","을 써줌

	// 문제 수만큼 반복하며 header row 생성
	for(i = 0; i < size; i++){
		// 해당 문제의 배점이 0인 경우, 더이상 문제가 없는 것으로 판단
		if(score_table[i].score == 0)
			break;
		
		sprintf(tmp, "%s,", score_table[i].qname);	// "문제이름," 형식으로 임시 변수에 저장
		write(fd, tmp, strlen(tmp));	// "문제이름," 형식의 문자열을 score_table.csv 파일에 추가
	}
	write(fd, "sum\n", 4);	// 총점을 저장할 마지막 열의 header에 "sum" 추가
}

/*
	char *get_answer(int fd, char *result)
	함수 설명 : ':'를 기준으로 정답 문자열은 구분해 result에 저장 후 리턴하는 함수
*/
char *get_answer(int fd, char *result)
{
	char c;
	int idx = 0;

	memset(result, 0, BUFLEN);
	while(read(fd, &c, 1) > 0)
	{
		if(c == ':')	// ':'은 답과 답을 구분하는 기준
			break;
		
		result[idx++] = c;	// result에 답을 한 문자 씩 저장
	}
	if(result[strlen(result) - 1] == '\n')	// 마지막에 입력 된 개행 문자를 NULL로 대체
		result[strlen(result) - 1] = '\0';

	// printf("result 문자열 길이 : %ld\n", strlen(result));

	return result;
}

/*
	int score_blank(char *id, char *filename);
	함수 설명 : "빈칸 채우기 문제" 채점 함수 (맞으면 true, 틀리면 false)
	인자 char *id : 학생 개인의 학번
	인자 char *filename : 학생이 제출한 빈칸 채우기 문제의 답안 파일명(.txt)
*/
int score_blank(char *id, char *filename)
{
	char tokens[TOKEN_CNT][MINLEN];
	node *std_root = NULL, *ans_root = NULL;
	int idx, start;		// 
	char tmp[MAXLEN];
	char s_answer[BUFLEN], a_answer[BUFLEN];	// 학생이 제출한 답 문자열, 정답 문자열
	char qname[FILELEN];	// 문제이름
	int fd_std, fd_ans;		// 학생이 제출한 파일 디스크립터, 정답 파일의 파일 디스크립터
	int result = true;	// 맞고 틀림 여부를 저장하는 변수
	int has_semicolon = false;		// 학생이 제출한 답의 맨 끝에 세미콜론이 있는지 여부를 저장

	memset(qname, 0, sizeof(qname));
	memcpy(qname, filename, strlen(filename) - strlen(strrchr(filename, '.')));

	sprintf(tmp, "%s/%s/%s", stuDir, id, filename);		// tmp에 채점할 .txt파일의 절대 경로 저장
	fd_std = open(tmp, O_RDONLY);	// 채점할 파일을 읽기 권한으로 열어 fd_std가 가리키도록 함

	// strcpy(s_answer, get_answer(fd_std, s_answer));	// 여기서 터지네
	get_answer(fd_std, s_answer);	// 해결

	// 답 없으면 fd_std 파일 닫고 false(0)점 리턴
	if(!strcmp(s_answer, "")){
		close(fd_std);
		return false;
	}

	// s_answer의 괄호 짝 맞는지 확인
	// 괄호 짝 안 맞으면 fd_std 파일 닫고 false(0)점 리턴
	if(!check_brackets(s_answer)){
		close(fd_std);
		return false;
	}

	// s_answer의 좌우 공백 제거
	strcpy(s_answer, ltrim(rtrim(s_answer)));
	
	// s_answer 문자열 맨 끝에 ';' 있으면 제거
	if(s_answer[strlen(s_answer) - 1] == ';'){
		has_semicolon = true;
		s_answer[strlen(s_answer) - 1] = '\0';
	}

	// s_answer를 토큰화해서 tokens에 저장
	if(!make_tokens(s_answer, tokens)){
		// 실패 시 fd_std 파일 닫고 false(0)점 리턴
		close(fd_std);
		return false;
	}

	idx = 0;
	std_root = make_tree(std_root, tokens, &idx, 0);

	sprintf(tmp, "%s/%s", ansDir, filename);
	fd_ans = open(tmp, O_RDONLY);

	while(1)
	{
		ans_root = NULL;
		result = true;

		for(idx = 0; idx < TOKEN_CNT; idx++)
			memset(tokens[idx], 0, sizeof(tokens[idx]));

		// strcpy(a_answer, get_answer(fd_ans, a_answer));	여기도 터짐
		get_answer(fd_ans, a_answer);	// 해결


		if(!strcmp(a_answer, ""))
			break;

		strcpy(a_answer, ltrim(rtrim(a_answer)));

		if(has_semicolon == false){
			if(a_answer[strlen(a_answer) -1] == ';')
				continue;
		}

		else if(has_semicolon == true)
		{
			if(a_answer[strlen(a_answer) - 1] != ';')
				continue;
			else
				a_answer[strlen(a_answer) - 1] = '\0';
		}

		if(!make_tokens(a_answer, tokens))
			continue;

		idx = 0;
		ans_root = make_tree(ans_root, tokens, &idx, 0);

		compare_tree(std_root, ans_root, &result);

		if(result == true){
			close(fd_std);
			close(fd_ans);

			if(std_root != NULL)
				free_node(std_root);
			if(ans_root != NULL)
				free_node(ans_root);
			return true;

		}
	}
	
	close(fd_std);
	close(fd_ans);

	if(std_root != NULL)
		free_node(std_root);
	if(ans_root != NULL)
		free_node(ans_root);

	return false;
}

/*
	double score_program(char *id, char *filename);
	함수 설명 : 프로그래밍문제 채점. 맞으면 true, 틀리면 false 리턴
	인자 char *id : 학번
	인자 char *filename : 파일명
*/
double score_program(char *id, char *filename)
{
	double compile;
	int result;

	compile = compile_program(id, filename);	// filename에 해당하는 문제(ex. 29.c) 컴파일

	if(compile == ERROR || compile == false)	
		return false;
	
	result = execute_program(id, filename);		// 프로그램 실행

	if(!result)
		return false;

	if(compile < 0)
		return compile;

	return true;
}


/*
	int is_thread(char *qname);
	함수 설명 : -t 옵션의 가변인자[QNAMES...]로 입력된 문제 번호인지 확인 (맞을 경우 컴파일 시 -lpthread 옵션 추가하기 위함)
	인자 char *gname : 채점할 파일명에서 확장자를 뗀 문제이름 (ex. 29)
*/
int is_thread(char *qname)
{
	// printf("is_thread의 인자 : %s\n", qname);
	int i;
	int size = sizeof(threadFiles) / sizeof(threadFiles[0]);

	for(i = 0; i < size; i++){
		if(!strcmp(threadFiles[i], qname))
			return true;
	}
	return false;
}

/*
	double compile_program(char *id, char *filename);
	함수 설명 : 학생이 제출한 프로그래밍 문제의 답안 파일(.c)을 컴파일하는 함수
	인자 char *id : 학생 개인의 학번
	인자 char *filename : 컴파일할 파일명 (ex. "29.c")
*/
double compile_program(char *id, char *filename)
{
	int fd;
	char tmp_f[MAXLEN], tmp_e[MAXLEN];
	char command[2*MAXLEN+20];
	char qname[FILELEN];
	int isthread;
	off_t size;
	double result;

	memset(qname, 0, sizeof(qname));
	memcpy(qname, filename, strlen(filename) - strlen(strrchr(filename, '.')));
	
	isthread = is_thread(qname);		// -t 옵션의 가변인자로 입력된 문제 번호인지 확인

	sprintf(tmp_f, "%s/%s", ansDir, filename);
// printf("## tmp_f : %s\n", tmp_f);
	sprintf(tmp_e, "%s/%s.exe", ansDir, qname);
// printf("## tmp_e : %s\n", tmp_e);

	// -t [QNAMES...] 형태로 입력된 문제 번호일 경우 컴파일 시 -lpthread 옵션 추가
	if((tOption && !tnum) ||(tOption && isthread)) {	// 첫 번째 인자 생략하거나(모든 문제 번호에 대하여 컴파일 실행) || 입력된 인자인 경우(입력된 인자에 대해서만 컴파일 실행)
		sprintf(command, "gcc -o %s %s -lpthread", tmp_e, tmp_f);

		// if (tOption && !tnum) printf("모든 인자에 대해 컴파일 \n");
		// else if (tOption && isthread) printf("선택된 인자에 대해 컴파일\n");
	}
	else
		sprintf(command, "gcc -o %s %s", tmp_e, tmp_f);

	sprintf(tmp_e, "%s/%s_error.txt", ansDir, qname);
// printf("## tmp_e : %s\n", tmp_e);
	fd = creat(tmp_e, 0666);

	redirection(command, fd, STDERR);
	size = lseek(fd, 0, SEEK_END);
	close(fd);
	unlink(tmp_e);

	if(size > 0)
		return false;

	sprintf(tmp_f, "%s/%s/%s", stuDir, id, filename);
	sprintf(tmp_e, "%s/%s/%s.stdexe", stuDir, id, qname);

	// if(tOption && isthread)
	if((tOption && !tnum) ||(tOption && isthread)) {	// 첫 번째 인자 생략하거나(모든 문제 번호에 대하여 컴파일 실행) || 입력된 인자인 경우(입력된 인자에 대해서만 컴파일 실행)
		sprintf(command, "gcc -o %s %s -lpthread", tmp_e, tmp_f);
		// if (tOption && !tnum) printf("모든 인자에 대해 컴파일 \n");
		// else if (tOption && isthread) printf("선택된 인자에 대해 컴파일\n");
	}
	else
		sprintf(command, "gcc -o %s %s", tmp_e, tmp_f);

	sprintf(tmp_f, "%s/%s/%s_error.txt", stuDir, id, qname);
	if ((fd = creat(tmp_f, 0666)) < 0) {
		fprintf(stderr, "creat error for %s\n", tmp_f);
		exit(1);
	}


	redirection(command, fd, STDERR);
	size = lseek(fd, 0, SEEK_END);
	close(fd);

	if(size > 0){
		if(eOption)	// eOption 실행. <DIRNAME>/학번/문제번호_error.txt에 에러 메세지가 출력
		{
			sprintf(tmp_e, "%s/%s", errorDir, id);
			if(access(tmp_e, F_OK) < 0)
				mkdir(tmp_e, 0755);

			sprintf(tmp_e, "%s/%s/%s_error.txt", errorDir, id, qname);
			rename(tmp_f, tmp_e);

			result = check_error_warning(tmp_e);
		}
		else{ 
			result = check_error_warning(tmp_f);
			unlink(tmp_f);
		}

		return result;
	}

	unlink(tmp_f);
	return true;
}

/*
	double check_error_warning(char *filename);
	함수 설명 : -e옵션의 예외 처리 중 에러 파일 생성 시 오류 발생하면 에러 처리를 하도록 하는 함수
	인자 char *filename : "<DIRNAME>/학번/문제번호_error.text" 형태의 에러 메세지를 출력할 파일명
*/
double check_error_warning(char *filename)
{
	FILE *fp;
	char tmp[BUFLEN];
	double warning = 0;

	if((fp = fopen(filename, "r")) == NULL){
		fprintf(stderr, "fopen error for %s\n", filename);
		return false;
	}

	while(fscanf(fp, "%s", tmp) > 0){
		if(!strcmp(tmp, "error:"))
			return ERROR;
		else if(!strcmp(tmp, "warning:"))
			warning += WARNING;
	}

	return warning;
}

/*
	int execute_program(char *id, char *filename);
	함수 설명 : 밑의 compare_tree 리턴 값 또는 false 값을 int형으로 리턴
	인자 char *id : 학번
	인자 char *filename : .c 문제 파일명 (ex. "29.c")
*/
int execute_program(char *id, char *filename)
{
	char std_fname[MAXLEN], ans_fname[MAXLEN];
	char tmp[MAXLEN];
	char qname[FILELEN];
	time_t start, end;	// 시작, 종료 시간 저장하기 위한 변수
	pid_t pid;
	int fd;

	memset(qname, 0, sizeof(qname));	// qname 배열 초기화
	memcpy(qname, filename, strlen(filename) - strlen(strrchr(filename, '.')));

	sprintf(ans_fname, "%s/%s.stdout", ansDir, qname);
// printf("$$ ans_fname : %s\n", ans_fname);
	fd = creat(ans_fname, 0666);

	sprintf(tmp, "%s/%s.exe", ansDir, qname);
// printf("$$ tmp : %s\n", tmp);
	redirection(tmp, fd, STDOUT);
	close(fd);

	sprintf(std_fname, "%s/%s/%s.stdout", stuDir, id, qname);
// printf("$$ std_fname : %s\n", std_fname);
	fd = creat(std_fname, 0666);

	sprintf(tmp, "%s/%s/%s.stdexe &", stuDir, id, qname);

	start = time(NULL);
	redirection(tmp, fd, STDOUT);
	
	sprintf(tmp, "%s.stdexe", qname);
	//  tmp 프로세스가 백그라운드에서 실행 중인지 확인
	while((pid = inBackground(tmp)) > 0){	// 백그라운드에서 실행 중인 프로세스가 있을 경우 해당 프로세스의 pid 값을 pid 변수에 저장
		end = time(NULL);	// 현재 시각을 end에 저장

		// 프로세스 실행 시간이 OVER(5초) 보다 큰 경우 강제 종료
		if(difftime(end, start) > OVER){	// 시각차가 OVER(5초)를 초과할 경우
			kill(pid, SIGKILL);		// pid 프로세스 강제 종료
			close(fd);		// fd 파일 닫기
			return false;	// 
		}
	}

	close(fd);

	return compare_resultfile(std_fname, ans_fname);
}

/*
	pid_t inBackground(char *name);
	함수 설명 : name 프로세스가 백그라운드에서 실행 중인지 확인
		실행 중일 경우 해당 프로세스의 pid 리턴,
		실행 중인 프로세스가 없을 경우 0 리턴
*/
pid_t inBackground(char *name)
{
	// printf("inBackground 함수 실행\n");
	pid_t pid;	// procee id 저장할 변수
	char command[64];	// 명령어 저장할 변수
	char tmp[64];
	int fd;		// background.txt 파일 가리킬 파일 디스크립터
	off_t size;
	
	memset(tmp, 0, sizeof(tmp));	// tmp 배열 초기화
	fd = open("background.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);	// "background.txt" 를 생성해 fd가 가리키도록 함. 읽기쓰기, 파일 없으면 생성, 기존 파일 초기화 권한 부여. 0666모드

	// sprintf(command, "ps | grep %s", name);		// command 배열에 "ps | grep <name>" 형식이 문자열 저장
	sprintf(command, "ps | grep %s | grep -v grep", name);		
	// sprintf(command, "ps -ef | grep -v grep | grep %s", name);
	// sprintf(command, "ps -eo pid,comm | grep %s | grep -v grep", name); 	// BSD에서
	// sprintf(command, "ps -ax | grep %s | grep -v grep", name);



	redirection(command, fd, STDOUT);		// command 문자열에 저장된 명령어 실행한 결과를 포준출력(stdout)이 아닌 fd가 가리키는 파일로 redirection

	lseek(fd, 0, SEEK_SET);		// fd 파일의 offset을 맨 처음으로 설정
	read(fd, tmp, sizeof(tmp));	// fd 파일 읽어서 tmp에 저장

	// tmp 문자열에 저장된 내용 확인
	if(!strcmp(tmp, "")){	// tmp가 비었을 경우, 즉 background에서 실행 중인 프로세스가 없을 경우
		unlink("background.txt");	// background.txt 파일 삭제
		close(fd);	// fd 파일 닫기
		return 0;	// 0 리턴
	}

	pid = atoi(strtok(tmp, " "));	// tmp 문자열을 공백 전까지(pid 값) 읽어와 정수형으로 바꿔 pid에 저장
	close(fd);	// fd 파일 닫기

	unlink("background.txt");	// background.txt 파일 삭제
	return pid;		// pid 리턴
}

/*
	int compare_resultfile(char *file1, char *file2);
	함수 설명 : 두개의 파일을 비교해 파일 내용이 일치하면 true(0이 아닌 값) 불일치하면 false(0)을 리턴함.
			이때 대소문자 구분을 하지 않아 따로 만들어둔 to_lower_case()함수 사용
	인자 char *file1 : 비교할 결과 파일1
	인자 char *file2 : 비교할 결과 파일2
*/
int compare_resultfile(char *file1, char *file2)
{
	int fd1, fd2;	// 각 파일을 가리킬 파일 디스크립터
	char c1, c2;	// 각 파일에서 문자 하나 씩 읽어들임
	int len1, len2;	// 각 파일 길이 저장

	fd1 = open(file1, O_RDONLY);	// file1을 읽기 모드로 open
	fd2 = open(file2, O_RDONLY);	// file2을 읽기 모드로 open

	while(1)
	{
		// 첫 번째 파일을 1byte 씩 읽음
		while((len1 = read(fd1, &c1, 1)) > 0){
			// 공백 문자는 pass
			if(c1 == ' ') 
				continue;
			else 
				break;
		}
		while((len2 = read(fd2, &c2, 1)) > 0){
			if(c2 == ' ') 
				continue;
			else 
				break;
		}
		
		if(len1 == 0 && len2 == 0)
			break;

		to_lower_case(&c1);
		to_lower_case(&c2);

		if(c1 != c2){
			close(fd1);
			close(fd2);
			return false;
		}
	}
	close(fd1);
	close(fd2);
	return true;
}

/*
	void redirection(char *command, int new, int old);
	함수 설명 : 기존 파일 디스크립터(old)를 새로운 파일 디스크립터(new)로 redirection하여, old를 통해 입출력되던 데이터를 new를 통해 입출력되도록 변경함
	(ex. old가 stdout이었다면, 기존에는 프로그램이 출력하던 데이터가 화면에 출력되었겠지만, new로 redirection하면 프로그램이 출력하는 데이터를 화면이 아닌 new가 가리키는 파일에 저장됨)
	인자 char *command : redirection의 대상이 되는 명령어
	인자 int new : 새로운 출력 파일 디스크립터
	인자 int old : 기존 출력 파일 디스크립터
*/
void redirection(char *command, int new, int old)
{
	int saved;	// 기존 출력파일 디스크립터를 백업해 둘 변수

	saved = dup(old);	// 기존 출력 파일 디스크립터(old)를 saved에 임시 저장
	dup2(new, old);		// 새로운 출력 파일 디스크립터(new)를 기존 출력파일 디스크립터(old)로 복제

	system(command); // command 명령어 실행

	dup2(saved, old);	// 기존 출력 파일 디스크립터(old)로 복원
	close(saved);	// saved로 복제되었던 old 닫기
}


/*
	int get_file_type(char *filename);
	함수 역할 : 파일 타입 검사 (".txt" or ".c" 파일 찾기 위함)
	인자 char *filename : 타입을 알아내고자 하는 파일명
*/
int get_file_type(char *filename)
{
	char *extension = strrchr(filename, '.');

	if(!strcmp(extension, ".txt"))
		return TEXTFILE;
	else if (!strcmp(extension, ".c"))
		return CFILE;
	else
		return -1;
}

// TODO : path 절대 경로인 지 확인
/*
	void rmdirs(const char *path);
	함수 설명 : -e 옵션 입력 시 실행 되는 폴더 삭제 함수. 폴더 내에 파일이 있을 경우 파일을 모두 지워 폴더를 비운 후 폴더를 삭제. 폴더 내부에 폴더가 있을 경우 재귀적으로 호출
	인자 const char *path : 삭제할 디렉토리의 경로
*/
void rmdirs(const char *path)
{
	struct dirent *dirp;	// dp가 가리키는 디렉토리 탐색하면서 내부 파일 혹은 디렉토리를 가리키는 스트림 포인터
	struct stat statbuf;	// 파일 상태 저장하기 위한 stat 구조체 변수
	DIR *dp;	// 디렉토리 스트림 포인터
	char tmp[MAXLEN];	
	// printf("dirname : %s\n", path);
	
	// path에 해당하는 디렉토리 열어서 dp가 가리키도록 함
	if((dp = opendir(path)) == NULL)
		return;	// 디렉토리 열기 실패 시 함수 종료

	//	dp가 가리키는 디렉토리를 탐색하면서 dirp가 내부 파일 혹은 디렉토리를 가리키도록 함
	while((dirp = readdir(dp)) != NULL)
	{
		
		// "." or ".."일 경우 pass
		if(!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."))
			continue;

		// printf("readdir 성공\n");
		// printf("%s\n", path);
		// printf("%s\n", dirp->d_name);
		sprintf(tmp, "%s/%s", path, dirp->d_name);	// tmp에 path 폴더 내 파일의 절대 경로 저장
		// printf("%s\n", tmp);

		// tmp의 stat를 statbuf에 저장
		if(lstat(tmp, &statbuf) == -1)
			continue;

		// tmp가 폴더일 경우 현재 함수 재귀적으로 호출
		if(S_ISDIR(statbuf.st_mode))	
			rmdirs(tmp);
		else
			unlink(tmp);	// tmp가 파일인 경우 삭제
	}

	closedir(dp);	// dp가 가리키는 디렉토리 닫기
	rmdir(path);	// 지우려는 폴더 내부의 파일 모두 지웠으므로 최종적으로 폴더 삭제
}

/*
	void to_lower_case(char *c);
	함수 설명 : 입력 문자를 소문자로 변환해줌
	인자 char *c : 문자
*/
void to_lower_case(char *c)
{
	if(*c >= 'A' && *c <= 'Z')
		*c = *c + 32;
}

/*
	void print_usage();
	함수 설명 : usage를 출력
*/
void print_usage()
{
	printf("Usage : ssu_score <STD_DIR> <ANS_DIR> [OPTION]\n");
	printf("Option : \n");
	printf(" -n <CSVFILENAME>\n");
	printf(" -m\n");
	printf(" -c [STUDENTIDS ...]\n");
	printf(" -p [STUDENTIDS ...]\n");
	printf(" -t [QNAMES ...]\n");
	printf(" -s <CATEGORY> <1|-1>\n");
	printf(" -e <DIRNAME>\n");
	printf(" -h\n");
}
