#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include "blank.h"

char datatype[DATATYPE_SIZE][MINLEN] = {"int", "char", "double", "float", "long"
			, "short", "ushort", "FILE", "DIR","pid"
			,"key_t", "ssize_t", "mode_t", "ino_t", "dev_t"
			, "nlink_t", "uid_t", "gid_t", "time_t", "blksize_t"
			, "blkcnt_t", "pid_t", "pthread_mutex_t", "pthread_cond_t", "pthread_t"
			, "void", "size_t", "unsigned", "sigset_t", "sigjmp_buf"
			, "rlim_t", "jmp_buf", "sig_atomic_t", "clock_t", "struct"};


operator_precedence operators[OPERATOR_CNT] = {
	{"(", 0}, {")", 0}
	,{"->", 1}	
	,{"*", 4}	,{"/", 3}	,{"%", 2}	
	,{"+", 6}	,{"-", 5}	
	,{"<", 7}	,{"<=", 7}	,{">", 7}	,{">=", 7}
	,{"==", 8}	,{"!=", 8}
	,{"&", 9}
	,{"^", 10}
	,{"|", 11}
	,{"&&", 12}
	,{"||", 13}
	,{"=", 14}	,{"+=", 14}	,{"-=", 14}	,{"&=", 14}	,{"|=", 14}
};

/*
	void compare_tree(node *root1,  node *root2, int *result);
	함수 설명 : 두 개의 트리(root1, root2)를 비교하여 같은지 확인. 
		두 트리가 다르면 result 변수에 false를,
		같으면 true를 저장

*/
void compare_tree(node *root1,  node *root2, int *result)
{
	node *tmp;
	int cnt1, cnt2;

	if(root1 == NULL || root2 == NULL){
		*result = false;
		return;
	}

	if(!strcmp(root1->name, "<") || !strcmp(root1->name, ">") || !strcmp(root1->name, "<=") || !strcmp(root1->name, ">=")){
		if(strcmp(root1->name, root2->name) != 0){

			if(!strncmp(root2->name, "<", 1))
				strncpy(root2->name, ">", 1);

			else if(!strncmp(root2->name, ">", 1))
				strncpy(root2->name, "<", 1);

			else if(!strncmp(root2->name, "<=", 2))
				strncpy(root2->name, ">=", 2);

			else if(!strncmp(root2->name, ">=", 2))
				strncpy(root2->name, "<=", 2);

			root2 = change_sibling(root2);
		}
	}

	if(strcmp(root1->name, root2->name) != 0){
		*result = false;
		return;
	}

	if((root1->child_head != NULL && root2->child_head == NULL)
		|| (root1->child_head == NULL && root2->child_head != NULL)){
		*result = false;
		return;
	}

	else if(root1->child_head != NULL){
		if(get_sibling_cnt(root1->child_head) != get_sibling_cnt(root2->child_head)){
			*result = false;
			return;
		}

		if(!strcmp(root1->name, "==") || !strcmp(root1->name, "!="))
		{
			compare_tree(root1->child_head, root2->child_head, result);

			if(*result == false)
			{
				*result = true;
				root2 = change_sibling(root2);
				compare_tree(root1->child_head, root2->child_head, result);
			}
		}
		else if(!strcmp(root1->name, "+") || !strcmp(root1->name, "*")
				|| !strcmp(root1->name, "|") || !strcmp(root1->name, "&")
				|| !strcmp(root1->name, "||") || !strcmp(root1->name, "&&"))
		{
			if(get_sibling_cnt(root1->child_head) != get_sibling_cnt(root2->child_head)){
				*result = false;
				return;
			}

			tmp = root2->child_head;

			while(tmp->prev != NULL)
				tmp = tmp->prev;

			while(tmp != NULL)
			{
				compare_tree(root1->child_head, tmp, result);
			
				if(*result == true)
					break;
				else{
					if(tmp->next != NULL)
						*result = true;
					tmp = tmp->next;
				}
			}
		}
		else{
			compare_tree(root1->child_head, root2->child_head, result);
		}
	}	


	if(root1->next != NULL){

		if(get_sibling_cnt(root1) != get_sibling_cnt(root2)){
			*result = false;
			return;
		}

		if(*result == true)
		{
			tmp = get_operator(root1);
	
			if(!strcmp(tmp->name, "+") || !strcmp(tmp->name, "*")
					|| !strcmp(tmp->name, "|") || !strcmp(tmp->name, "&")
					|| !strcmp(tmp->name, "||") || !strcmp(tmp->name, "&&"))
			{	
				tmp = root2;
	
				while(tmp->prev != NULL)
					tmp = tmp->prev;

				while(tmp != NULL)
				{
					compare_tree(root1->next, tmp, result);

					if(*result == true)
						break;
					else{
						if(tmp->next != NULL)
							*result = true;
						tmp = tmp->next;
					}
				}
			}

			else
				compare_tree(root1->next, root2->next, result);
		}
	}
}

/*
	int make_tokens(char *str, char tokens[TOKEN_CNT][MINLEN]);
	함수 설명 : str에서 op에 저장된 문자를 기준으로 분자열을 분리해서 tokens에 저장
	인자 char *str : 분리할 대상 문자열
	인자 char tokens[TOKEN_CNT][MINLEN] : 분리할 문자열을 저장할 2차원 문자 배열
*/
int make_tokens(char *str, char tokens[TOKEN_CNT][MINLEN])
{
	char *start, *end;	// 문자열 분리를 위한 포인터
	char tmp[BUFLEN];	// 임시 문자열 저장 배열
	char str2[BUFLEN];	// 문자열의 복사본 저장 배열 
	char *op = "(),;><=!|&^/+-*\""; 	// 연산자, 구분자를 저장한 문자열
	int row = 0;	// tokens 배열 행 번호
	int i;
 	int isPointer;	// 변수의 포인터 여부를 저장
	int lcount, rcount;	// 좌, 우 괄호 수 저장 
	int p_str;	
	
	clear_tokens(tokens);	// tokens 배열 초기화

	start = str;
	
	if(is_typeStatement(str) == 0) 	// 선언문 아닐 경우
		return false;		// false 리턴
	
	// 문자열 분리해서 tokens 배열에 저장
	while(1)
	{
		if((end = strpbrk(start, op)) == NULL)	// 연산자 or 구분자 없을 경우
			break;	// 반복문 탈출

		if(start == end){	// start와 end 같을 경우

			// 단항연산자일 경우
			if(!strncmp(start, "--", 2) || !strncmp(start, "++", 2)){	//++ or --가 2개 이상 연속으로 나오면
				if(!strncmp(start, "++++", 4)||!strncmp(start,"----",4))
					return false;	// flase return

				// ex) ++a
				if(is_character(*ltrim(start + 2))){
					if(row > 0 && is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1]))
						return false; //ex) ++a++

					end = strpbrk(start + 2, op);
					if(end == NULL)
						end = &str[strlen(str)];
					while(start < end) {
						if(*(start - 1) == ' ' && is_character(tokens[row][strlen(tokens[row]) - 1]))
							return false;
						else if(*start != ' ')
							strncat(tokens[row], start, 1);
						start++;	
					}
				}
				// ex) a++
				else if(row>0 && is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1])){
					if(strstr(tokens[row - 1], "++") != NULL || strstr(tokens[row - 1], "--") != NULL)	
						return false;

					memset(tmp, 0, sizeof(tmp));
					strncpy(tmp, start, 2);
					strcat(tokens[row - 1], tmp);
					start += 2;
					row--;
				}
				else{
					memset(tmp, 0, sizeof(tmp));
					strncpy(tmp, start, 2);
					strcat(tokens[row], tmp);
					start += 2;
				}
			}

			else if(!strncmp(start, "==", 2) || !strncmp(start, "!=", 2) || !strncmp(start, "<=", 2)
				|| !strncmp(start, ">=", 2) || !strncmp(start, "||", 2) || !strncmp(start, "&&", 2) 
				|| !strncmp(start, "&=", 2) || !strncmp(start, "^=", 2) || !strncmp(start, "!=", 2) 
				|| !strncmp(start, "|=", 2) || !strncmp(start, "+=", 2)	|| !strncmp(start, "-=", 2) 
				|| !strncmp(start, "*=", 2) || !strncmp(start, "/=", 2)){

				strncpy(tokens[row], start, 2);
				start += 2;
			}
			else if(!strncmp(start, "->", 2))
			{
				end = strpbrk(start + 2, op);

				if(end == NULL)
					end = &str[strlen(str)];

				while(start < end){
					if(*start != ' ')
						strncat(tokens[row - 1], start, 1);
					start++;
				}
				row--;
			}
			else if(*end == '&')
			{
				// ex) &a (address)
				if(row == 0 || (strpbrk(tokens[row - 1], op) != NULL)){
					end = strpbrk(start + 1, op);
					if(end == NULL)
						end = &str[strlen(str)];
					
					strncat(tokens[row], start, 1);
					start++;

					while(start < end){
						if(*(start - 1) == ' ' && tokens[row][strlen(tokens[row]) - 1] != '&')
							return false;
						else if(*start != ' ')
							strncat(tokens[row], start, 1);
						start++;
					}
				}
				// ex) a & b (bit)
				else{
					strncpy(tokens[row], start, 1);
					start += 1;
				}
				
			}
		  	else if(*end == '*')
			{
				isPointer=0;

				if(row > 0)
				{
					//ex) char** (pointer)
					for(i = 0; i < DATATYPE_SIZE; i++) {
						if(strstr(tokens[row - 1], datatype[i]) != NULL){
							strcat(tokens[row - 1], "*");
							start += 1;	
							isPointer = 1;
							break;
						}
					}
					if(isPointer == 1)
						continue;
					if(*(start+1) !=0)
						end = start + 1;

					// ex) a * **b (multiply then pointer)
					if(row>1 && !strcmp(tokens[row - 2], "*") && (all_star(tokens[row - 1]) == 1)){
						strncat(tokens[row - 1], start, end - start);
						row--;
					}
					
					// ex) a*b(multiply)
					else if(is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1]) == 1){ 
						strncat(tokens[row], start, end - start);   
					}

					// ex) ,*b (pointer)
					else if(strpbrk(tokens[row - 1], op) != NULL){		
						strncat(tokens[row] , start, end - start); 
							
					}
					else
						strncat(tokens[row], start, end - start);

					start += (end - start);
				}

			 	else if(row == 0)
				{
					if((end = strpbrk(start + 1, op)) == NULL){
						strncat(tokens[row], start, 1);
						start += 1;
					}
					else{
						while(start < end){
							if(*(start - 1) == ' ' && is_character(tokens[row][strlen(tokens[row]) - 1]))
								return false;
							else if(*start != ' ')
								strncat(tokens[row], start, 1);
							start++;	
						}
						if(all_star(tokens[row]))
							row--;
						
					}
				}
			}
			else if(*end == '(') 
			{
				lcount = 0;
				rcount = 0;
				if(row>0 && (strcmp(tokens[row - 1],"&") == 0 || strcmp(tokens[row - 1], "*") == 0)){
					while(*(end + lcount + 1) == '(')
						lcount++;
					start += lcount;

					end = strpbrk(start + 1, ")");

					if(end == NULL)
						return false;
					else{
						while(*(end + rcount +1) == ')')
							rcount++;
						end += rcount;

						if(lcount != rcount)
							return false;

						if( (row > 1 && !is_character(tokens[row - 2][strlen(tokens[row - 2]) - 1])) || row == 1){ 
							strncat(tokens[row - 1], start + 1, end - start - rcount - 1);
							row--;
							start = end + 1;
						}
						else{
							strncat(tokens[row], start, 1);
							start += 1;
						}
					}
						
				}
				else{
					strncat(tokens[row], start, 1);
					start += 1;
				}

			}
			else if(*end == '\"') 
			{
				end = strpbrk(start + 1, "\"");
				
				if(end == NULL)
					return false;

				else{
					strncat(tokens[row], start, end - start + 1);
					start = end + 1;
				}

			}

			else{
				// ex) a++ ++ +b
				if(row > 0 && !strcmp(tokens[row - 1], "++"))
					return false;

				// ex) a-- -- -b
				if(row > 0 && !strcmp(tokens[row - 1], "--"))
					return false;
	
				strncat(tokens[row], start, 1);
				start += 1;
				
				// ex) -a or a, -b
				if(!strcmp(tokens[row], "-") || !strcmp(tokens[row], "+") || !strcmp(tokens[row], "--") || !strcmp(tokens[row], "++")){


					// ex) -a or -a+b
					if(row == 0)
						row--;

					// ex) a+b = -c
					else if(!is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1])){
					
						if(strstr(tokens[row - 1], "++") == NULL && strstr(tokens[row - 1], "--") == NULL)
							row--;
					}
				}
			}
		}
		else{ 
			if(all_star(tokens[row - 1]) && row > 1 && !is_character(tokens[row - 2][strlen(tokens[row - 2]) - 1]))   
				row--;				

			if(all_star(tokens[row - 1]) && row == 1)   
				row--;	

			for(i = 0; i < end - start; i++){
				if(i > 0 && *(start + i) == '.'){
					strncat(tokens[row], start + i, 1);

					while( *(start + i +1) == ' ' && i< end - start )
						i++; 
				}
				else if(start[i] == ' '){
					while(start[i] == ' ')
						i++;
					break;
				}
				else
					strncat(tokens[row], start + i, 1);
			}

			if(start[0] == ' '){
				start += i;
				continue;
			}
			start += i;
		}
			
		strcpy(tokens[row], ltrim(rtrim(tokens[row])));

		 if(row > 0 && is_character(tokens[row][strlen(tokens[row]) - 1]) 
				&& (is_typeStatement(tokens[row - 1]) == 2 
					|| is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1])
					|| tokens[row - 1][strlen(tokens[row - 1]) - 1] == '.' ) ){

			if(row > 1 && strcmp(tokens[row - 2],"(") == 0)
			{
				if(strcmp(tokens[row - 1], "struct") != 0 && strcmp(tokens[row - 1],"unsigned") != 0)
					return false;
			}
			else if(row == 1 && is_character(tokens[row][strlen(tokens[row]) - 1])) {
				if(strcmp(tokens[0], "extern") != 0 && strcmp(tokens[0], "unsigned") != 0 && is_typeStatement(tokens[0]) != 2)	
					return false;
			}
			else if(row > 1 && is_typeStatement(tokens[row - 1]) == 2){
				if(strcmp(tokens[row - 2], "unsigned") != 0 && strcmp(tokens[row - 2], "extern") != 0)
					return false;
			}
			
		}

		if((row == 0 && !strcmp(tokens[row], "gcc")) ){
			clear_tokens(tokens);
			strcpy(tokens[0], str);	
			return 1;
		} 

		row++;
	}

	if(all_star(tokens[row - 1]) && row > 1 && !is_character(tokens[row - 2][strlen(tokens[row - 2]) - 1]))  
		row--;				
	if(all_star(tokens[row - 1]) && row == 1)   
		row--;	

	for(i = 0; i < strlen(start); i++)   
	{
		if(start[i] == ' ')  
		{
			while(start[i] == ' ')
				i++;
			if(start[0]==' ') {
				start += i;
				i = 0;
			}
			else
				row++;
			
			i--;
		} 
		else
		{
			strncat(tokens[row], start + i, 1);
			if( start[i] == '.' && i<strlen(start)){
				while(start[i + 1] == ' ' && i < strlen(start))
					i++;

			}
		}
		strcpy(tokens[row], ltrim(rtrim(tokens[row])));

		if(!strcmp(tokens[row], "lpthread") && row > 0 && !strcmp(tokens[row - 1], "-")){ 
			strcat(tokens[row - 1], tokens[row]);
			memset(tokens[row], 0, sizeof(tokens[row]));
			row--;
		}
	 	else if(row > 0 && is_character(tokens[row][strlen(tokens[row]) - 1]) 
				&& (is_typeStatement(tokens[row - 1]) == 2 
					|| is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1])
					|| tokens[row - 1][strlen(tokens[row - 1]) - 1] == '.') ){
			
			if(row > 1 && strcmp(tokens[row-2],"(") == 0)
			{
				if(strcmp(tokens[row-1], "struct") != 0 && strcmp(tokens[row-1], "unsigned") != 0)
					return false;
			}
			else if(row == 1 && is_character(tokens[row][strlen(tokens[row]) - 1])) {
				if(strcmp(tokens[0], "extern") != 0 && strcmp(tokens[0], "unsigned") != 0 && is_typeStatement(tokens[0]) != 2)	
					return false;
			}
			else if(row > 1 && is_typeStatement(tokens[row - 1]) == 2){
				if(strcmp(tokens[row - 2], "unsigned") != 0 && strcmp(tokens[row - 2], "extern") != 0)
					return false;
			}
		} 
	}


	if(row > 0)
	{

		// ex) #include <sys/types.h>
		if(strcmp(tokens[0], "#include") == 0 || strcmp(tokens[0], "include") == 0 || strcmp(tokens[0], "struct") == 0){ 
			clear_tokens(tokens); 
			strcpy(tokens[0], remove_extraspace(str)); 
		}
	}

	if(is_typeStatement(tokens[0]) == 2 || strstr(tokens[0], "extern") != NULL){
		for(i = 1; i < TOKEN_CNT; i++){
			if(strcmp(tokens[i],"") == 0)  
				break;		       

			if(i != TOKEN_CNT -1 )
				strcat(tokens[0], " ");
			strcat(tokens[0], tokens[i]);
			memset(tokens[i], 0, sizeof(tokens[i]));
		}
	}
	
	//change ( ' char ' )' a  ->  (char)a
	while((p_str = find_typeSpecifier(tokens)) != -1){ 
		if(!reset_tokens(p_str, tokens))
			return false;
	}

	//change sizeof ' ( ' record ' ) '-> sizeof(record)
	while((p_str = find_typeSpecifier2(tokens)) != -1){  
		if(!reset_tokens(p_str, tokens))
			return false;
	}
	
	return true;
}

/*
	node *make_tree(node *root, char (*tokens)[MINLEN], int *idx, int parentheses);
	함수 설명 : 인자로 입력 된 문자열을 토큰 배열로 토큰화하는 함수
	인자 char *str: 분석하려는 C 코드 문자열
	인자 token **tokens: 토큰 배열의 주소. 함수 안에서 이 배열이 할당됨
	인자 int *num_tokens: 생성된 토큰 수를 저장하는 변수의 주소로, 함수 내부에서 이 변수가 초기화됨
*/
node *make_tree(node *root, char (*tokens)[MINLEN], int *idx, int parentheses)
{
	node *cur = root;
	node *new;
	node *saved_operator;
	node *operator;
	int fstart;
	int i;

	while(1)	
	{
		if(strcmp(tokens[*idx], "") == 0)	
			break;
	
		if(!strcmp(tokens[*idx], ")"))
			return get_root(cur);

		else if(!strcmp(tokens[*idx], ","))
			return get_root(cur);

		else if(!strcmp(tokens[*idx], "("))
		{
			// function()
			if(*idx > 0 && !is_operator(tokens[*idx - 1]) && strcmp(tokens[*idx - 1], ",") != 0){
				fstart = true;

				while(1)
				{
					*idx += 1;

					if(!strcmp(tokens[*idx], ")"))
						break;
					
					new = make_tree(NULL, tokens, idx, parentheses + 1);
					
					if(new != NULL){
						if(fstart == true){
							cur->child_head = new;
							new->parent = cur;
	
							fstart = false;
						}
						else{
							cur->next = new;
							new->prev = cur;
						}

						cur = new;
					}

					if(!strcmp(tokens[*idx], ")"))
						break;
				}
			}
			else{
				*idx += 1;
	
				new = make_tree(NULL, tokens, idx, parentheses + 1);

				if(cur == NULL)
					cur = new;

				else if(!strcmp(new->name, cur->name)){
					if(!strcmp(new->name, "|") || !strcmp(new->name, "||") 
						|| !strcmp(new->name, "&") || !strcmp(new->name, "&&"))
					{
						cur = get_last_child(cur);

						if(new->child_head != NULL){
							new = new->child_head;

							new->parent->child_head = NULL;
							new->parent = NULL;
							new->prev = cur;
							cur->next = new;
						}
					}
					else if(!strcmp(new->name, "+") || !strcmp(new->name, "*"))
					{
						i = 0;

						while(1)
						{
							if(!strcmp(tokens[*idx + i], ""))
								break;

							if(is_operator(tokens[*idx + i]) && strcmp(tokens[*idx + i], ")") != 0)
								break;

							i++;
						}
						
						if(get_precedence(tokens[*idx + i]) < get_precedence(new->name))
						{
							cur = get_last_child(cur);
							cur->next = new;
							new->prev = cur;
							cur = new;
						}
						else
						{
							cur = get_last_child(cur);

							if(new->child_head != NULL){
								new = new->child_head;

								new->parent->child_head = NULL;
								new->parent = NULL;
								new->prev = cur;
								cur->next = new;
							}
						}
					}
					else{
						cur = get_last_child(cur);
						cur->next = new;
						new->prev = cur;
						cur = new;
					}
				}
	
				else
				{
					cur = get_last_child(cur);

					cur->next = new;
					new->prev = cur;
	
					cur = new;
				}
			}
		}
		else if(is_operator(tokens[*idx]))
		{
			if(!strcmp(tokens[*idx], "||") || !strcmp(tokens[*idx], "&&")
					|| !strcmp(tokens[*idx], "|") || !strcmp(tokens[*idx], "&") 
					|| !strcmp(tokens[*idx], "+") || !strcmp(tokens[*idx], "*"))
			{
				if(is_operator(cur->name) == true && !strcmp(cur->name, tokens[*idx]))
					operator = cur;
		
				else
				{
					new = create_node(tokens[*idx], parentheses);
					operator = get_most_high_precedence_node(cur, new);	// 이 함수에서 segmentationfault

					if(operator->parent == NULL && operator->prev == NULL){

						if(get_precedence(operator->name) < get_precedence(new->name)){
							cur = insert_node(operator, new);
						}

						else if(get_precedence(operator->name) > get_precedence(new->name))
						{
							if(operator->child_head != NULL){
								operator = get_last_child(operator);
								cur = insert_node(operator, new);
							}
						}
						else
						{
							operator = cur;
	
							while(1)
							{
								if(is_operator(operator->name) == true && !strcmp(operator->name, tokens[*idx]))
									break;
						
								if(operator->prev != NULL)
									operator = operator->prev;
								else
									break;
							}

							if(strcmp(operator->name, tokens[*idx]) != 0)
								operator = operator->parent;

							if(operator != NULL){
								if(!strcmp(operator->name, tokens[*idx]))
									cur = operator;
							}
						}
					}

					else
						cur = insert_node(operator, new);
				}

			}
			else
			{
				new = create_node(tokens[*idx], parentheses);

				if(cur == NULL)
					cur = new;

				else
				{
					operator = get_most_high_precedence_node(cur, new);

					if(operator->parentheses > new->parentheses)
						cur = insert_node(operator, new);

					else if(operator->parent == NULL && operator->prev ==  NULL){
					
						if(get_precedence(operator->name) > get_precedence(new->name))
						{
							if(operator->child_head != NULL){
	
								operator = get_last_child(operator);
								cur = insert_node(operator, new);
							}
						}
					
						else	
							cur = insert_node(operator, new);
					}
	
					else
						cur = insert_node(operator, new);
				}
			}
		}
		else 
		{
			new = create_node(tokens[*idx], parentheses);

			if(cur == NULL)
				cur = new;

			else if(cur->child_head == NULL){
				cur->child_head = new;
				new->parent = cur;

				cur = new;
			}
			else{

				cur = get_last_child(cur);

				cur->next = new;
				new->prev = cur;

				cur = new;
			}
		}

		*idx += 1;
	}

	return get_root(cur);
}

/*
	node *change_sibling(node *parent);
	함수 설명 : parent 노드의 첫번째 자식 노드와 두번째 자식 노드의 위치를 변경하는 함수
	인자 node *parent : 자식 노드의 위치를 변경할 부모 노드를 가리키는 포인터 변수
*/
node *change_sibling(node *parent)
{
	node *tmp;	// 포인터 변수 선언
	
	tmp = parent->child_head;	// parent 노드의 첫번째 자식 노드를 tmp 변수에 저장

	parent->child_head = parent->child_head->next;	// parent 노드의 첫번째 자식 노드를 두번째 자식 노드로 변경
	parent->child_head->parent = parent;	// 변경된 두번째 자식 노드의 부모 노드를 parent 노드로 변경
	parent->child_head->prev = NULL;	// 변경된 두번째 자식 노드의 이전 노드를 NULL로 설정

	parent->child_head->next = tmp;	// 변경된 두번째 자식 노드의 다음 노드를 tmp 노드로 설정
	parent->child_head->next->prev = parent->child_head;	// tmp 노드의 이전 노드를 변경된 두번째 자식 노드로 설정
	parent->child_head->next->next = NULL;	// tmp 노드의 다음 노드를 NULL로 설정
	parent->child_head->next->parent = NULL;	// tmp 노드의 부모 노드를 NULL로 설정	

	return parent;	// 변경된 parent 노드를 반환
}


/*
	node *create_node(char *name, int parentheses);
	함수 설명 : 이름과 괄호 개수를 입력으로 받아 새로운 노드를 생성.
	해당 노드의 멤버 변수들 초기화 후 생성된 노드 리턴
	인자 name: 생성할 노드의 이름 문자열 포인터
	인자 parentheses: 생성할 노드의 괄호 개수 
*/
node *create_node(char *name, int parentheses)
{
	node *new;

	new = (node *)malloc(sizeof(node));	// 새로운 노드 동적 할당
	new->name = (char *)malloc(sizeof(char) * (strlen(name) + 1));	// 새로운 노드의 이름을 저장할 char 배열을 동적 할당. 이름은 입력으로 받은 문자열의 길이보다 1만큼 더 크게 할당
	strcpy(new->name, name);	// 입력으로 받은 문자열 -> 새로운 노드의 이름으로 복사

	new->parentheses = parentheses;	// 새로운 노드의 parentheses 멤버 변수에 입력받은 parentheses 값을 저장.

	new->parent = NULL;	// 초기화
	new->child_head = NULL;	// 초기화
	new->prev = NULL;	// 초기화
	new->next = NULL;	// 초기화

	return new;	// 새로운 노드 new 리턴
}

/*
	int get_precedence(char *op);
	함수 설명 : 주어진 연산자 문자열에 해당하는 연산자의 우선수위 리턴
	인자 char *op : 우선순위 가져올 연산자 문자열
*/
int get_precedence(char *op)
{
	int i;	

	//  operators 배열에서 주어진 연산자 문자열과 일치하는 연산자 찾기
	for(i = 2; i < OPERATOR_CNT; i++){
		if(!strcmp(operators[i].operator, op))	// 찾을 경우
			return operators[i].precedence;	// 그 우선 순위를 리턴
	}
	return false;	// 없을 경우 false
}

/*
	int is_operator(char *op);
	함수 설명 :	입력받은 연산자 문자열이 operators 배열에 있는 연산자들 중 하나인지 확인
	인자 char *op : 연산자 문자열
*/
int is_operator(char *op)
{
	int i;

	// 연산자 배열을 반복하면서 입력된 op와 일치하는 연산자가 있는지 확인
	for(i = 0; i < OPERATOR_CNT; i++)
	{
		if(operators[i].operator == NULL)	// operatere 배열의 마지막 원소가 NULL이면
			break;	// 반복문 탈출
		if(!strcmp(operators[i].operator, op)){		// op와 일치하는 연산자 찾으면
			return true;	// true 리턴
		}
	}

	return false;	// 못착으면 false 리턴
}

/*
	void print(node *cur);
	함수 설명 : 입력으로 받은 노드를 중위표기법으로 출력
	인자 node *cur : 출력할 노드 포인터
*/
void print(node *cur)
{
	if(cur->child_head != NULL){	// 자식 노드 있을 경우
		print(cur->child_head);	// 자식 노드 출력
		printf("\n");
	}

	if(cur->next != NULL){	// 자식 노드 없을 경우
		print(cur->next);	// 자식 노드 출력
		printf("\t");	
	}
	printf("%s", cur->name);	// 현재 노드 이름 출력
}

/*
	node *get_operator(node *cur);
	함수 설명 : 입력받은 노드(cur)의 가장 왼쪽 자식 노드부터 시작하여 이전 노드가 없을 때까지 이동
				마지막으로 만난 연산자 노드의 부모 노드 리턴
	인자 node *cur : 현재 노드
*/
node *get_operator(node *cur)
{
	if(cur == NULL)	// 현재 노드가 NULL이면
		return cur;	// 현재 노드 반환

	if(cur->prev != NULL)	// 이전 노드 존재 하면
		while(cur->prev != NULL)	// 존재 안할 때까지 
			cur = cur->prev;	// 이전 노드로 이동 (마지막으로 만난 연산자 노드의 부모 노드 리턴하기 위함)

	return cur->parent;	// 마지막으로 만난 연산자 노드의  부모 노드 리턴
}

/*
	node *get_root(node *cur);
	함수 설명 : 입력받은 노드의 가장 왼쪽 자식 노드부터 시작하여 최상위 루트 노드 리턴
	인자 node *cur : 현재 노드
*/
node *get_root(node *cur)
{
	if(cur == NULL)	// 현재 노드 NULL이면
		return cur;	// 현재 노드 리턴

	while(cur->prev != NULL)	// 이전 노드 NULL 아니면
		cur = cur->prev;	// 이전 노드로 이동

	if(cur->parent != NULL)	// 부모 노드 NULL 아니면
		cur = get_root(cur->parent);	// 부모 노드로 이동 (root node 찾기 위함)

	return cur;	// root node 반환
}

/*
	node *get_high_precedence_node(node *cur, node *new);
	함수 설명 : 현재 노드의 이전 노드부터 시작해서 연산자 노드 중 new의 우선순위보다 높은 노드 리턴
	인자 node *cur : 현재 노드
	인자 node *new : 새로운 노드
*/
node *get_high_precedence_node(node *cur, node *new)
{
	if(is_operator(cur->name))	// 현재 노드가 연산자 노드일 경우
		if(get_precedence(cur->name) < get_precedence(new->name))	// 현재 노드의 연산자 우선순위가 새로운 노드의 연산자 우선순위보다 낮은 경우
			return cur;	// 현재 노드 리턴

	if(cur->prev != NULL){	// 현재 노드 이전 노드가 존재하는 경우
		while(cur->prev != NULL){	// 이전 노드가 존재할 때까지 반복
			cur = cur->prev;	// 이전 노드로 이동
			
			return get_high_precedence_node(cur, new);	// 이전 노드에서 재귀적으로 호출
		}


		if(cur->parent != NULL) 	// 부모 노드가 존재하는 경우
			return get_high_precedence_node(cur->parent, new);	// 부모 노드에서 재귀적으로 호출
	}

	// segmentation fault 해결 위해 아래 문장 주석 처리
	// if(cur->parent == NULL) 

	return cur; // 현재 노드가 루트 노드인 경우 현재 노드 리턴
}

/*
	node *get_most_high_precedence_node(node *cur, node *new);
	함수 설명 : 현재 노드부터 루트 노드까지 거슬러 올라가면서 new의 우선순위보다 높은 연산자 노드 리턴
	인자 node *cur : 현재 노드
	인자 node *new : 새로운 노드
*/
node *get_most_high_precedence_node(node *cur, node *new)
{
    node *operator = get_high_precedence_node(cur, new);
    node *saved_operator = operator;

	while(1)
	{
		if(saved_operator->parent == NULL)	// 루트 노드에 도달한 경우
			break;

		if(saved_operator->prev != NULL)	// 이전 노드 있는 경우
			operator = get_high_precedence_node(saved_operator->prev, new);

		else if(saved_operator->parent != NULL)	// 부모 노드 있는 경우
			operator = get_high_precedence_node(saved_operator->parent, new);

		saved_operator = operator;
	}
	
	return saved_operator;
}

/*
	node *insert_node(node *old, node *new);
	함수 설명 : 연결 리스트에서 old 노드 앞에 new 노드 삽입
	인자 node *old : 이전 노드의 포인터
	인자 node *new : 새로운 노드 포인터 
*/
node *insert_node(node *old, node *new)
{
	if(old->prev != NULL){
		new->prev = old->prev;
		old->prev->next = new;
		old->prev = NULL;
	}

	new->child_head = old;
	old->parent = new;

	return new;
}

/*
	node *get_last_child(node *cur);
	함수 설명 : 현재 노드의 마지막 자식 노드를 찾음
	인자 node *cur : 현재 노드 포인터
*/
node *get_last_child(node *cur)
{
	if(cur->child_head != NULL)
		cur = cur->child_head;

	while(cur->next != NULL)
		cur = cur->next;

	return cur;
}

/*
	int get_sibling_cnt(node *cur);
	함수 설명 : 현재 노드의 형제 노드 개수 리턴
	인자 node *cur : 현재 노드 포인터
*/
int get_sibling_cnt(node *cur)
{
	int i = 0;

	while(cur->prev != NULL)
		cur = cur->prev;

	while(cur->next != NULL){
		cur = cur->next;
		i++;
	}

	return i;
}

/*
	void free_node(node *cur);
	함수 설명 : 전달된 노드의 자식 노드부터 순차적으로 메모리 해제. 마지막으로 cur 노드를 메모리 해제
	인자 node *cur : 메모리를 해제할 노드 포인터
*/
void free_node(node *cur)
{
	if(cur->child_head != NULL)	 // cur의 자식 노드들을 순차적으로 메모리 해제
		free_node(cur->child_head);

	if(cur->next != NULL)	// cur의 다음 노드를 순차적으로 메모리 해제
		free_node(cur->next);

	if(cur != NULL){	// cur 노드의 이전 노드, 다음 노드, 부모 노드, 자식 노드 포인터 초기화 후 메모리 해제
		cur->prev = NULL;
		cur->next = NULL;
		cur->parent = NULL;
		cur->child_head = NULL;
		free(cur);
	}
}


/*
	int is_character(char c);
	함수 설명 : 인자로 받은 문자 c가 알파벳 대문자, 소문자, 숫자인지 판단 
	인자 char c : 판단할 문자
*/
int is_character(char c)
{
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}


/*
	int is_typeStatement(char *str);
	함수 설명 : 주어진 문자열이 유효한 유형 선언문인지 확인
	인자 char *str : 문자열
*/
int is_typeStatement(char *str)
{ 
	char *start;
	char str2[BUFLEN] = {0}; 
	char tmp[BUFLEN] = {0}; 
	char tmp2[BUFLEN] = {0}; 
	int i;	 
	
	start = str;
	strncpy(str2,str,strlen(str));
	remove_space(str2);

	while(start[0] == ' ')
		start += 1;

	if(strstr(str2, "gcc") != NULL)
	{
		strncpy(tmp2, start, strlen("gcc"));
		if(strcmp(tmp2,"gcc") != 0)
			return 0;
		else
			return 2;
	}
	
	for(i = 0; i < DATATYPE_SIZE; i++)
	{
		if(strstr(str2,datatype[i]) != NULL)
		{	
			strncpy(tmp, str2, strlen(datatype[i]));
			strncpy(tmp2, start, strlen(datatype[i]));
			
			if(strcmp(tmp, datatype[i]) == 0) {
				if(strcmp(tmp, tmp2) != 0) {
					return 0;  
				}
				else {
					return 2;
				}
			}
		}

	}
	return 1;

}

/*
	int find_typeSpecifier(char tokens[TOKEN_CNT][MINLEN]);
	함수 설명 : 주어진 토큰 배열에서 데이터 타입 지정자를 찾아 해당 인덱스 리턴
	인자 char tokens[TOKEN_CNT][MINLEN] : 토큰 배열
*/
int find_typeSpecifier(char tokens[TOKEN_CNT][MINLEN]) 
{
	int i, j;

	for(i = 0; i < TOKEN_CNT; i++)
	{
		for(j = 0; j < DATATYPE_SIZE; j++)
		{
			if(strstr(tokens[i], datatype[j]) != NULL && i > 0)
			{
				if(!strcmp(tokens[i - 1], "(") && !strcmp(tokens[i + 1], ")") 
						&& (tokens[i + 2][0] == '&' || tokens[i + 2][0] == '*' 
							|| tokens[i + 2][0] == ')' || tokens[i + 2][0] == '(' 
							|| tokens[i + 2][0] == '-' || tokens[i + 2][0] == '+' 
							|| is_character(tokens[i + 2][0])))  
					return i;	// 데이터 타딥 지정자 있는 인덱스 리턴
			}
		}
	}
	return -1;	// 인덱스 없을 경우 -1 리턴
}

/*
	int find_typeSpecifier2(char tokens[TOKEN_CNT][MINLEN]) 
	함수 설명 : tokens 배열에서 "struct" 다음에 구조체 이름이 오는 타입 지정자를 찾아 해당 인덱스 리턴
	인자 char tokens[TOKEN_CNT][MINLEN] : 
*/
int find_typeSpecifier2(char tokens[TOKEN_CNT][MINLEN]) 
{
    int i, j;

   
    for(i = 0; i < TOKEN_CNT; i++)
    {
        for(j = 0; j < DATATYPE_SIZE; j++)
        {
            if(!strcmp(tokens[i], "struct") && (i+1) <= TOKEN_CNT && is_character(tokens[i + 1][strlen(tokens[i + 1]) - 1]))  
                    return i;
        }
    }
    return -1;
}

/*
	int all_star(char *str);
	함수 설명 : 문자열이 모두 '' 인지 확인 -> 모두 '*' 이면 1 리턴, 아니면 0 리턴
	인자 char *str : 문자열
*/
int all_star(char *str)
{
	int i;
	int length= strlen(str);
	
 	if(length == 0)	
		return 0;
	
	for(i = 0; i < length; i++)
		if(str[i] != '*')
			return 0;
	return 1;

}

/*
	int all_character(char *str);
	함수 설명 : 문자열 str이 모두 알파벳 문자인지 검사
	인자 char *str : 검사할 문자열
*/
int all_character(char *str)
{
	int i;

	for(i = 0; i < strlen(str); i++)
		if(is_character(str[i]))
			return 1;
	return 0;
	
}

/*
	int reset_tokens(int start, char tokens[TOKEN_CNT][MINLEN]) 
	함수 설명 : tokens 배열에서 start 인덱스부터 괄호를 제거.
			struct, unsigned 키워드를 하나로 합침 or sizeof 연산자 있는지 확인
*/
int reset_tokens(int start, char tokens[TOKEN_CNT][MINLEN]) 
{
	int i;
	int j = start - 1;
	int lcount = 0, rcount = 0;		// 좌우 괄호 개수 저장
	int sub_lcount = 0, sub_rcount = 0;	// 서브 괄호 개수 저장

	if(start > -1){
		// "struct" 키워드가 포함된 토큰의 경우
		if(!strcmp(tokens[start], "struct")) {		
			strcat(tokens[start], " ");
			strcat(tokens[start], tokens[start+1]);	     

			for(i = start + 1; i < TOKEN_CNT - 1; i++){
				strcpy(tokens[i], tokens[i + 1]);
				memset(tokens[i + 1], 0, sizeof(tokens[0]));
			}
		}
		// "unsigned" 키워드가 포함된 토큰의 경우
		else if(!strcmp(tokens[start], "unsigned") && strcmp(tokens[start+1], ")") != 0) {		
			strcat(tokens[start], " ");
			strcat(tokens[start], tokens[start + 1]);	     
			strcat(tokens[start], tokens[start + 2]);

			for(i = start + 1; i < TOKEN_CNT - 1; i++){
				strcpy(tokens[i], tokens[i + 1]);
				memset(tokens[i + 1], 0, sizeof(tokens[0]));
			}
		}

     		j = start + 1;           
        	while(!strcmp(tokens[j], ")")){
                	rcount ++;
                	if(j==TOKEN_CNT)
                        	break;
                	j++;
        	}
	
		j = start - 1;
		while(!strcmp(tokens[j], "(")){
        	        lcount ++;
                	if(j == 0)
                        	break;
               		j--;
		}
		// 괄호 앞에 문자가 있는 경우, 왼쪽 괄호 개수를 오른쪽 괄호 개수와 동일하게 조정
		if( (j!=0 && is_character(tokens[j][strlen(tokens[j])-1]) ) || j==0)
			lcount = rcount;

		// 괄호 개수가 일치하지 않는 경우, 실패 리턴
		if(lcount != rcount )
			return false;

		// "sizeof" 키워드가 포함된 토큰의 경우, true 리턴
		if( (start - lcount) >0 && !strcmp(tokens[start - lcount - 1], "sizeof")){
			return true; 
		}
		
		// "unsigned" 또는 "struct" 키워드가 포함된 토큰의 경우
		else if((!strcmp(tokens[start], "unsigned") || !strcmp(tokens[start], "struct")) && strcmp(tokens[start+1], ")")) {		
			strcat(tokens[start - lcount], tokens[start]);
			strcat(tokens[start - lcount], tokens[start + 1]);
			strcpy(tokens[start - lcount + 1], tokens[start + rcount]);
		 
			for(int i = start - lcount + 1; i < TOKEN_CNT - lcount -rcount; i++) {
				strcpy(tokens[i], tokens[i + lcount + rcount]);
				memset(tokens[i + lcount + rcount], 0, sizeof(tokens[0]));
			}


		}
 		else{
			if(tokens[start + 2][0] == '('){
				j = start + 2;
				while(!strcmp(tokens[j], "(")){
					sub_lcount++;
					j++;
				} 	
				if(!strcmp(tokens[j + 1],")")){
					j = j + 1;
					while(!strcmp(tokens[j], ")")){
						sub_rcount++;
						j++;
					}
				}
				else 
					return false;

				if(sub_lcount != sub_rcount)
					return false;
				
				strcpy(tokens[start + 2], tokens[start + 2 + sub_lcount]);	
				for(int i = start + 3; i<TOKEN_CNT; i++)
					memset(tokens[i], 0, sizeof(tokens[0]));

			}
			strcat(tokens[start - lcount], tokens[start]);
			strcat(tokens[start - lcount], tokens[start + 1]);
			strcat(tokens[start - lcount], tokens[start + rcount + 1]);
		 
			for(int i = start - lcount + 1; i < TOKEN_CNT - lcount -rcount -1; i++) {
				strcpy(tokens[i], tokens[i + lcount + rcount +1]);
				memset(tokens[i + lcount + rcount + 1], 0, sizeof(tokens[0]));

			}
		}
	}
	return true;
}

/*
	void clear_tokens(char tokens[TOKEN_CNT][MINLEN]);
	함수 설명 : 문자열 배열로 이루어진 토큰 배열을 인자로 받아 모든 토큰을 빈 문자열("")로 초기화
	인자 char tokens[TOKEN_CNT][MINLEN] : 초기화할 토큰 배열
*/
void clear_tokens(char tokens[TOKEN_CNT][MINLEN])
{
	int i;

	for(i = 0; i < TOKEN_CNT; i++)
		memset(tokens[i], 0, sizeof(tokens[i]));
}

/*
	char *rtrim(char *_str)
	함수 설명 : 문자열 _str의 오른쪽 공백 제거 후 반환
	인자 char *_str : 공백 제거할 문자열
*/
char *rtrim(char *_str)
{
	char tmp[BUFLEN];
	char *end;

	strcpy(tmp, _str);
	end = tmp + strlen(tmp) - 1;
	while(end != _str && isspace(*end))
		--end;

	*(end + 1) = '\0';
	_str = tmp;
	return _str;
}

/*
	char *ltrim(char *_str)
	함수 설명 : 문자열 _str의 왼쪽 공백 제거 후 리턴
	인자 char *_str : 공백 제거할 문자열
*/
char *ltrim(char *_str)
{
	char *start = _str;

	while(*start != '\0' && isspace(*start))
		++start;
	_str = start;
	return _str;
}

/*
	char* remove_extraspace(char *str);
	함수 설명 : 문자열 내에 불필요한 공백을 제거 or #include < > 문장에서 '<' 앞뒤로 공백을 추가
	인자 char *str : 처리할 문자열
*/
char* remove_extraspace(char *str)
{
	int i;
	char *str2 = (char*)malloc(sizeof(char) * BUFLEN);
	char *start, *end;
	char temp[BUFLEN] = "";
	int position;

	// #include < > 문장일 경우 
	if(strstr(str,"include<")!=NULL){
		start = str;
		end = strpbrk(str, "<");
		position = end - start;
	
		strncat(temp, str, position);
		strncat(temp, " ", 2);
		strncat(temp, str + position, strlen(str) - position + 1);

		str = temp;		
	}
	
	for(i = 0; i < strlen(str); i++)
	{
		// 공백이 있을 경우 처리
		if(str[i] ==' ')
		{
			if(i == 0 && str[0] ==' ')	// 문자열 맨 앞 공백 제거
				while(str[i + 1] == ' ')
					i++;	
			else{	
				if(i > 0 && str[i - 1] != ' ')	// 연속된 공백 중복 제거
					str2[strlen(str2)] = str[i];
				while(str[i + 1] == ' ')
					i++;
			} 
		}
		else	// 공백이 아닐 경우 그대로 저장
			str2[strlen(str2)] = str[i];
	}

	return str2;
}



/*
	void remove_space(char *str);
	함수 설명 : 문자열 공백 제거
	인자 char *str : 공백 제거할 문자열
*/
void remove_space(char *str)
{
	char* i = str;
	char* j = str;
	
	while(*j != 0)
	{
		*i = *j++;
		if(*i != ' ')
			i++;
	}
	*i = 0;
}


/*
	int check_brackets(char *str);
	함수 설명 : 괄호의 짝이 맞는지 확인. 맞으면 1, 안 맞으면 0 리턴
	인자 char *str : 검사할 문자열 
*/
int check_brackets(char *str)
{
	char *start = str;	// 괄호의 짝을 검사할 문자열
	int lcount = 0, rcount = 0;	 	// 좌, 우 괄호 개수
	
	while(1){
		if((start = strpbrk(start, "()")) != NULL){	
			if(*(start) == '(')
				lcount++;
			else
				rcount++;

			start += 1; 		
		}
		else
			break;
	}

	if(lcount != rcount)
		return 0;
	else 
		return 1;
}

/*
	int get_token_cnt(char tokens[TOKEN_CNT][MINLEN]);
	함수 설명 : tokens 배열에서 비어있지 않은 토큰의 개수 리턴
	인자 char tokens[TOKEN_CNT][MINLEN] : 토큰 담긴 배열
*/
int get_token_cnt(char tokens[TOKEN_CNT][MINLEN])
{
	int i;
	
	//	배열을 처음부터 검사
	for(i = 0; i < TOKEN_CNT; i++)
		if(!strcmp(tokens[i], ""))	// 빈 문자열 나오면
			break;	// 	반복문 탈출

	return i;	// 해당 인덱스 리턴
}
