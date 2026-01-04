#ifndef MAIN_H_
#define MAIN_H_

#ifndef true
	#define true 1
#endif
#ifndef false
	#define false 0
#endif
#ifndef STDOUT
	#define STDOUT 1
#endif
#ifndef STDERR
	#define STDERR 2
#endif
#ifndef TEXTFILE
	#define TEXTFILE 3		
#endif
#ifndef CFILE
	#define CFILE 4			
#endif
#ifndef OVER
	#define OVER 5
#endif
#ifndef WARNING
	#define WARNING -0.1
#endif
#ifndef ERROR
	#define ERROR 0
#endif

#define MAXLEN 4096	
#define FILELEN 128		// FILE LENgth : 파일 길이
#define BUFLEN 1024		// BUFfer LENgth : 버퍼 길이
#define SNUM 100		// Student NUMber : 학셍 수
#define QNUM 100		// Question NUMber : 문제 수
#define ARGNUM 5		// ARG NUMber : 프로그램 실행 시 최대 인자 개수

struct ssu_scoreTable {
	char qname[FILELEN];	// 문제명
	double score;			// 학생이 습득한 점수
};

// 학생 개인의 틀린 문제,점수 저장할 링크드 리스트 구조체
typedef struct Qnode {
	char id[10];			// 학번
	char qname[FILELEN];	// 문제 이름
	double score;			// 배점
struct Qnode *next;			// 다음 노드 가리키는 포인터
} Qnode;


void ssu_score(int argc, char *argv[]);			// 채점 함수
int check_option(int argc, char *argv[]);		// 프로그램 실행 시 입력한 옵션 확인
void print_usage();								// usage 출력

void score_students();						// 채정 결과 테이블(score.csv)를 생성하여 학생들에게 성적 매기고 전체 학생의 점수 평균을 출력함
double score_student(int fd, char *id);		// 학생 개인에게 각 문제를 채점한 점수(빈칸채우기문제, 프로그래밍문제)를 부여해 score.csv에 채졈 결과를 추가하며 파일을 완성하고 총점을 리턴
void write_first_row(int fd);				// 학생들의 성적을 저장할 score_table.csv 파일에 header row 생성 (1.1.txt ... sum)

char *get_answer(int fd, char *result);				// ':'를 기준으로 정답 문자열은 구분해 result에 저장 후 리턴하는 함수
int score_blank(char *id, char *filename);			// "빈칸 채우기 문제" 채점 함수 (맞으면 true, 틀리면 false)
double score_program(char *id, char *filename);		// 프로그래밍문제 채점. 맞으면 true, 틀리면 false 리턴
double compile_program(char *id, char *filename);	// 학생이 제출한 프로그래밍 문제의 답안 파일(.c)을 컴파일하는 함수
int execute_program(char *id, char *filname);		// 밑의 compare_tree 리턴 값 또는 false 값을 int형으로 리턴
pid_t inBackground(char *name);						// name 프로세스가 백그라운드에서 실행 중인지 확인
double check_error_warning(char *filename);			// -e옵션의 예외 처리 중 에러 파일 생성 시 오류 발생하면 에러 처리를 하도록 하는 함수
int compare_resultfile(char *file1, char *file2);	// 두개의 파일을 비교해 파일 내용이 일치하면 true(0이 아닌 값) 불일치하면 false(0)을 리턴

void do_mOption();							// -m 입력 시 실행
double do_cOption(double score, char* id);	// -c 입력 시 실행
void do_pOption(Qnode *cur, char* id);		// -p 입력 시 실행

int is_exist(char(*src)[10], char *target);					// -c 혹은 -p 옵션이 활성화 되어있을 때, 가변인자로 들어온 학번과 실제 STD_DIR에 해당 학번이 존재하는지 확인
int qname_exist(struct ssu_scoreTable *src, char *target);	// -t 옵션이 활성화 되어있을 때, 가변인자로 들어온 문제 번호가 실제 score_table에 존재하는지 확인

void print_wrong_problem(Qnode *cur, char* id);						// -p 입력 시, 링크드 리스트에 저장된 학생의 틀린 문제 정보 출력
void sort_student_by(int size, char *sort_key, int sort_order);		// -s 옵션 입력 시, 기준에 따라 student 구조체 배열 정렬

int is_thread(char *qname);		// -t 옵션의 가변인자[QNAMES...]로 입력된 문제 번호인지 확인 (맞을 경우 컴파일 시 -lpthread 옵션 추가하기 위함)
void redirection(char *command, int newfd, int oldfd);	// 기존 파일 디스크립터(old)를 새로운 파일 디스크립터(new)로 redirection하여, old를 통해 입출력되던 데이터를 new를 통해 입출력되도록 변경함
int get_file_type(char *filename);	// 파일 타입 검사 (".txt" or ".c" 파일 찾기 위함)
void rmdirs(const char *path);	// 폴더 삭제 함수. 폴더 내에 파일이 있을 경우 파일을 모두 지워 폴더를 비운 후 폴더를 삭제. 폴더 내부에 폴더가 있을 경우 재귀적으로 호출
void to_lower_case(char *c);	// 입력 문자를 소문자로 변환해줌

void set_scoreTable(char *ansDir);		// score_table.csv 파일의 존재 여부에 따라 파일 생성하거나 읽어오도록 분기
void read_scoreTable(char *path);		// score_table.csv가 있는 경우 실행 되며, score_table.csv 파일을 읽어와 score_table 구조체 배열에 문제이름(qname)과 점수(score)을 저장
void make_scoreTable(char *ansDir);		// score_table.csv가 없는 경우 실행되며, score_table 구조체 배열 생성 후 각 문제 번호에 점수 설정
void write_scoreTable(char *filename);	// score_table.csv가 없는 경우 실행되며, 점수 테이블 파일을 생성하고 score_table 구조체 배열의 문제 이름(qname)과 점수(score)을 파일에 write함
void set_idTable(char *stuDir);			// STD_DIR 디렉토리를 탐색하며 학생들이 제출한 답안 폴더(ex. "20200001")들을 id_table 구조체에 추가
int get_create_type(char *ansDir);		// 점수테이블파일(score_table.csv) 존재하지 않는 경우에 실행되며, 사용자로부터 파일 생성 형태를 입력 받음

void sort_idTable(int size);		// id_table 구조체 배열 정렬
void sort_scoreTable(int size);		// score_table(점수 테이블) 구조체 배열 완성 후에 실행되며, 점수 테이블을 정렬함
void get_qname_number(char *qname, int *num1, int *num2);	// 문제 이름(qname)의 문제 번호와 서브문제 번호를 분리 해주는 함수

#endif
