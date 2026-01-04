#ifndef BLANK_H_
#define BLANK_H_

#ifndef true
	#define true 1
#endif
#ifndef false
	#define false 0
#endif
#ifndef BUFLEN
	#define BUFLEN 1024
#endif

#define OPERATOR_CNT 24
#define DATATYPE_SIZE 35
#define MINLEN 64
#define TOKEN_CNT 50

typedef struct node{
	int parentheses;
	char *name;
	struct node *parent;
	struct node *child_head;
	struct node *prev;
	struct node *next;
}node;

typedef struct operator_precedence{
	char *operator;
	int precedence;
}operator_precedence;

void compare_tree(node *root1,  node *root2, int *result);	// 두 개의 트리(root1, root2)를 비교하여 같은지 확인
node *make_tree(node *root, char (*tokens)[MINLEN], int *idx, int parentheses);		// 인자로 입력 된 문자열을 토큰 배열로 토큰화하는 함수
node *change_sibling(node *parent);	// parent 노드의 첫번째 자식 노드와 두번째 자식 노드의 위치를 변경하는 함수
node *create_node(char *name, int parentheses);	// 이름과 괄호 개수를 입력으로 받아 새로운 노드를 생성
int get_precedence(char *op);	// 주어진 연산자 문자열에 해당하는 연산자의 우선수위 리턴
int is_operator(char *op);	// 입력받은 연산자 문자열이 operators 배열에 있는 연산자들 중 하나인지 확인
void print(node *cur);		// 입력으로 받은 노드를 중위표기법으로 출력
node *get_operator(node *cur);	// 입력받은 노드(cur)의 가장 왼쪽 자식 노드부터 시작하여 이전 노드가 없을 때까지 이동 후 마지막으로 만난 연산자 노드의 부모 노드 리턴
node *get_root(node *cur);	// 입력받은 노드의 가장 왼쪽 자식 노드부터 시작하여 최상위 루트 노드 리턴
node *get_high_precedence_node(node *cur, node *new);		// 현재 노드의 이전 노드부터 시작해서 연산자 노드 중 new의 우선순위보다 높은 노드 리턴
node *get_most_high_precedence_node(node *cur, node *new);	// 현재 노드부터 루트 노드까지 거슬러 올라가면서 new의 우선순위보다 높은 연산자 노드 리턴
node *insert_node(node *old, node *new);	// 연결 리스트에서 old 노드 앞에 new 노드 삽입. 
node *get_last_child(node *cur);	// 현재 노드의 마지막 자식 노드를 찾음
void free_node(node *cur);		// 전달된 노드의 자식 노드부터 순차적으로 메모리 해제. 마지막으로 cur 노드를 메모리 해제
int get_sibling_cnt(node *cur);		// 현재 노드의 형제 노드 개수 리턴

int make_tokens(char *str, char tokens[TOKEN_CNT][MINLEN]);		// str에서 op에 저장된 문자를 기준으로 분자열을 분리해서 tokens에 저장
int is_typeStatement(char *str);		// 주어진 문자열이 유효한 유형 선언문인지 확인
int find_typeSpecifier(char tokens[TOKEN_CNT][MINLEN]);		// 주어진 토큰 배열에서 데이터 타입 지정자를 찾아 해당 인덱스 리턴
int find_typeSpecifier2(char tokens[TOKEN_CNT][MINLEN]);	// tokens 배열에서 "struct" 다음에 구조체 이름이 오는 타입 지정자를 찾아 해당 인덱스 리턴
int is_character(char c);		// 인자로 받은 문자 c가 알파벳 대문자, 소문자, 숫자인지 판단 
int all_star(char *str);		// 문자열이 모두 '' 인지 확인 -> 모두 '*' 이면 1 리턴, 아니면 0 리턴
int all_character(char *str);	// 문자열 str이 모두 알파벳 문자인지 검사
int reset_tokens(int start, char tokens[TOKEN_CNT][MINLEN]);	// tokens 배열에서 start 인덱스부터 괄호를 제거. struct, unsigned 키워드를 하나로 합침 or sizeof 연산자 있는지 확인
void clear_tokens(char tokens[TOKEN_CNT][MINLEN]);		// 문자열 배열로 이루어진 토큰 배열을 인자로 받아 모든 토큰을 빈 문자열("")로 초기화
int get_token_cnt(char tokens[TOKEN_CNT][MINLEN]);		//  tokens 배열에서 비어있지 않은 토큰의 개수 리턴
char *rtrim(char *_str);		// 문자열 _str의 오른쪽 공백 제거 후 리턴
char *ltrim(char *_str);		// 문자열 _str의 왼쪽 공백 제거 후 리턴
void remove_space(char *str);	// 문자열 공백 제거
int check_brackets(char *str);	// 괄호의 짝이 맞는지 확인. 맞으면 1, 안 맞으면 0 리턴
char* remove_extraspace(char *str);	// 문자열 내에 불필요한 공백을 제거 or #include < > 문장에서 '<' 앞뒤로 공백을 추가

#endif
