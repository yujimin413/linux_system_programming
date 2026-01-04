#include "ssu_monitor.h"

int main(void)
{
    ssu_prompt();
    return 0;
}

// ssu_monitor 프로그램 실행
void ssu_monitor(int argc, char *argv[])
{
    ssu_prompt();
    return;
}

// 프롬프트 출력
void ssu_prompt(void)
{
    char command[BUFLEN];
    char **argv = NULL;
    int argc = 0;

    while (1)
    {
        printf("20211431> ");
        fgets(command, sizeof(command), stdin);
        command[strlen(command) - 1] = '\0';

        if ((argv = GetSubstring(command, &argc, " \t")) == NULL)
        { // 입력된 명령어를 공백이나 탭을 구분자로 사용하여 토큰화
            continue;
        }

        if (argc == 0)
            continue; // 엔터 입력 시

        if (execute_command(argc, argv) == 1)
        {
            // printf("exit 입력. 프로그램 종료\n");
            return;
        }
    }
}

// 명령어 실행
int execute_command(int argc, char *argv[])
{
    if (!strcmp(argv[0], "add"))
    {
        // printf("## add 실행 \n");
        execute_add(argc, argv);
    }
    else if (!strcmp(argv[0], "delete"))
    {
        // printf("## delete 실행 \n");
        execute_delete(argc, argv);
    }
    else if (!strcmp(argv[0], "tree"))
    {
        // printf("## tree 실행 \n");
        execute_tree(argc, argv);
    }
    else if (!strcmp(argv[0], "help"))
    {
        // printf("## help 실행 \n");
        execute_help(argc, argv);
    }
    else if (!strcmp(argv[0], "exit"))
    {
        // printf("## exit 실행 \n");
        // execute_exit(argc, argv);
        return 1;
    }
    else
    {
        fprintf(stderr, "wrong command in prompt\n");
        execute_help(argc, argv);
    }
    return 0;
}

// add 명령어 수행
void execute_add(int argc, char *argv[])
{
    char real_path[BUFLEN]; // 절대 경로 저장할 변수
    time_t mn_time = 1;     // 재시작 시간 1초로 설정

    if (argc < 2 || argc == 3)
    {
        printf("Usage: add <DIRPATH> [OPTION]\n");
        printf("	 -t <TIME> : 디몬 프로세스는 <TIME> 간격을 두고 모니터링 재시작\n");
        return;
    }

    if (argv[2] != NULL)
    { // [OPTION]에 옵션 입력 되었을 때
        if (strcmp(argv[2], "-t") || atoi(argv[3]) == 0)
        { // -t 옵션의 사용이 올바르지 않은 경우
            printf("Usage : add <DIRPATH> [OPTION]\n");
            printf("	 -t <TIME> : 디몬 프로세스는 <TIME> 간격을 두고 모니터링 재시작\n");
            return;
        }
        // printf("-t 옵션 입력\n");
        mn_time = atoi(argv[3]); // <TIME>에 입력된 값을 간격으로 두고 모니터링 재시작하도록 설정

        // printf("%s %d\n", argv[2], mn_time);return;
    }

    if ((realpath(argv[1], real_path)) == NULL)
    { // real_path 에 절대 경로 저장
        // fprintf(stderr, "realpath error for %s\n", argv[1]);	// 절대경로 or 상대경로가 아닐 경우 에러메세지 출력
        printf("Usage : add <DIRPATH> [OPTION]\n");
        printf("	 -t <TIME> : 디몬 프로세스는 <TIME> 간격을 두고 모니터링 재시작\n");
        return;
    }
    // printf("%s, %d\n", real_path, mn_time);exit(0);

    init_daemon(real_path, mn_time); // 디몬 프로세스 초기화 및 생성하여 <DIRPATH>의 모니터링 시작

    printf("monitoring started (%s)\n", real_path);

    // free(real_path);

    return;
}

// delete 명령어 수행
void execute_delete(int argc, char *argv[])
{
    char line[BUFLEN];
    char dirpath[BUFLEN];
    FILE *fp;
    FILE *fp_tmp;

    if (argc < 2)
    { // 인자 제대로 입력 안 한 경우
        fprintf(stderr, "Usage: delete <DAEMON_PID>\n");
        return;
    }

    if ((fp = fopen(monitor_list, "r")) == NULL)
    { // monitor_list.txt 읽기 모드로 열기
        fprintf(stderr, "fopen error for \"%s\"\n", monitor_list);
        return;
    }

    if ((fp_tmp = fopen("tmp.txt", "w")) == NULL)
    { // tmp.txt 쓰기 모드로 열기
        fprintf(stderr, "fopen error for \"%s\"\n", "tmp.txt");
        fclose(fp);
        return;
    }

    bool exist = false; // pid일치여부 저장

    while (fgets(line, sizeof(line), fp) != NULL)
    {

        char *saved_path = strtok(line, " \n");
        char *saved_pid = strtok(NULL, " \n");
        // printf("## saved_path : %s, saved_pid : %s, argv[1] : %s\n", saved_path, saved_pid, argv[1]);
        if (saved_pid && saved_path)
        {
            // printf("saved_pid : %s, saved_pid : %s\n", saved_pid, argv[1]);
            if (strcmp(saved_pid, argv[1]))
            { // 첫 번째 인자 <DAEMON_PID>가 "monitor_list.txt"에 존재하지 않음
                // printf("%s != %s\n", saved_pid, argv[1]);
                fprintf(fp_tmp, "%s %s\n", saved_path, saved_pid); // 삭제 대상이 아니므로 "tmp.txt"에 복사
            }
            else
            {                 // 첫 번째 인자 <DAEMON_PID>가 "monitor_list.txt"에 존재할 경우
                exist = true; // 삭제 대상이 존재함을 표시
                strcpy(dirpath, saved_path);
            }
        }
    }

    fclose(fp);
    fclose(fp_tmp);

    if (!exist)
    { // 첫 번째 인자 <DAEMON_PID>가 "monitor_list.txt"에 존재하지 않는 경우
        fprintf(stderr, "<DAEMON_PID> \"%s\"가 \"%s\"에 존재하지 않음\n", argv[1], monitor_list);
        remove("tmp.txt");
        return;
    }

    // 일치하는 pid가 존재하는 경우
    if (remove(monitor_list) != 0)
    { // "monitor_list.txt"삭제
        fprintf(stderr, "remove error for \"%s\"\n", monitor_list);
        remove("temp.txt");
        return;
    }
    if (rename("tmp.txt", monitor_list) != 0)
    { // "tmp.txt를 "monitor_list.txt"로 이름 변경
        fprintf(stderr, "rename error\n");
        remove("tmp.txt");
        return;
    }

    // <DAEMON_PID>에 SIGUSR1을 보내 디몬 프로세스 종료함
    if (kill(atoi(argv[1]), SIGUSR1) != 0)
    {
        fprintf(stderr, "<DAEMON_PID> \"%s\"에 SIGUSR 보내기 실패\n", argv[1]);
        return;
    }

    printf("monitoring ended (%s)\n", dirpath);
    exit(0);
}

// tree 명령어 수행
void execute_tree(int argc, char *argv[])
{
    char *dirpath; // 절대 경로를 dirpath에 저장
    FILE *fp;
    tree *root;

    if (argc != 2)
    {                                      // 인자 개수 2보다 작을 경우
        printf("Usage: tree <DIRPATH>\n"); // usage 출력
        return;
    }

    if ((dirpath = realpath(argv[1], NULL)) == NULL)
    { // 절대경로/상대경로 모두 입력 가능하도록 절대 경로를 dirpath에 저장
        fprintf(stderr, "\"%s\" 경로 잘못 입력\n", argv[1]);
        return;
    }

    if ((fp = fopen(monitor_list, "r")) == NULL)
    { // "monitor_list.txt" 열기
        fprintf(stderr, "fopen error for %s\n", monitor_list);
        free(dirpath);
        return;
    }

    int exist = 0;
    char line[BUFLEN];
    while (fgets(line, sizeof(line), fp))
    {
        char *path = strtok(line, " "); // 공백을 기준으로 경로 추출
        if (strcmp(path, dirpath) == 0)
        { // 디렉토리 경로가 존재하는 경우
            exist = 1;
            break;
        }
    }

    fclose(fp);

    if (!exist)
    {
        fprintf(stderr, "<DIRPATH> \"%s\"가 \"%s\"에 존재하지 않음\n", argv[1], monitor_list);
        free(dirpath); // 디렉토리 경로 메모리 해제
        return;
    }

    root = (tree *)malloc(sizeof(tree));  // 루트 노드 생성
    strncpy(root->path, dirpath, BUFLEN); // 루트 노드 경로 설정
    root->next = NULL;
    root->child = NULL;

    execute_tree_re(root, argv[1]); // 디렉토리 트리 생성 함수 호출

    printf("%s\n", argv[1]);          // 입력된 디렉토리 경로 출력
    print_tree_re(root->child, 1, 0); // 트리 출력 함수 호출

    free_tree(root); // 트리 메모리 해제
}

// help 명령어 수행
void execute_help(int argc, char *argv[])
{
    print_usage();
}

// daemon process 생성 위한 초기화 함수
void init_daemon(char *dirpath, time_t mn_time)
{
    pid_t pid, dpid;
    tree *head, *new_tree;
    // char path[BUFLEN];
    FILE *fp; // monitor_list.txt를 가리킬 파일 디스크립터

    sprintf(log_path, "%s/%s", dirpath, "log.txt"); // log.txt 파일을 dirpath 경로에 해당하는 디렉토리 내에 생성

    head = create_node(dirpath, 0, 0);
    make_tree(head, dirpath);

    // 첫 번째 인자로 입력받은 경로(dirpath)가 이미 모니터링 중인 디렉토리의 경로(saved_path)와 같거나/포함하거나/포함되는 지 확인
    if (is_already_monitored(dirpath))
    { // 포함될 경우
        // fprintf(stderr, "\"%s\"는 이미 모니터링 중인 디렉토리의 경로와 같거나/포함하거나/포함되는 경우임\n", dirpath);	// 에러 메세지 출력
        exit(1); // 프로그램 비정상 종료
    }
    log_fp = fopen(log_path, "a"); // dirpath 디렉토리 내에 log.txt 생성
    if (log_fp == NULL)
    {
        fprintf(stderr, "fopen error for \"%s\"\n", "log.txt");
        exit(1);
    }
    fclose(log_fp);

    if ((pid = fork()) < 0)
    { // 자식 프로세스 생성. error 발생 시g
        fprintf(stderr, "fork error\n");
        exit(1);
    }
    else if (pid == 0)
    { // child process
        if ((dpid = (make_daemon(fp, dirpath))) < 0)
        { // 디몬 프로세스 생성
            fprintf(stderr, "make_daemon error\n");
            exit(1);
        }

        while (!signal_received) // 시그널 발생할 때까지 모니터링 작업
        {
            new_tree = create_node(dirpath, 0, 0);
            make_tree(new_tree, dirpath); // 비교를 위해 새로운 트리 만들기
            compare_tree(head, new_tree); // 이전 트리와 비교해서 모니터링
            head = new_tree;

            sleep(mn_time);
        }
        free(new_tree);
    }
    else
    { // parent process
        // printf("parent process\n");
        return;
    }
}

// daemon process 생성하는 함수
pid_t make_daemon(FILE *fp, char *dirpath)
{
    pid_t pid;
    int fd, maxfd;

    if ((pid = fork()) < 0)
    { // 자식 프로세스 생성. 에러 발생 시
        fprintf(stderr, "fork error\n");
        exit(1);
    }
    else if (pid != 0) // child process 아닐 경우
        exit(0);       // 프로그램 종료

    // dirpath가 saved_path와 같거나/포함하거나/포함되지 않는 경우
    // pwd의 "monitor_list.txt"에 <DIRPATH>의 절대경로와 생성된 디몬 프로세스의 pid 정보 저장
    if ((fp = fopen(monitor_list, "a")) == NULL)
    { // pwd의 "monitor_list.txt"를 읽기쓰기 모드로 오픈
        fprintf(stderr, "fopen error for \"%s\"", monitor_list);
        exit(1);
    }
    fprintf(fp, "%s %d\n", dirpath, getpid()); // "monitor_list.txt"에 <DIRPATH>의 절대경로와 생성된 디몬 프로세스의 pid 정보 저장
    fclose(fp);                                // "monitor_list.txt" 닫기

    setsid();                 // 새로운 세션 및 프로세스 그룹 생성
    signal(SIGTTIN, SIG_IGN); // 터미널로부터의 입력 시그널 무시
    signal(SIGTTOU, SIG_IGN); // 터미널로부터의 출력 시그널 무시
    signal(SIGTSTP, SIG_IGN); // 터미널에서의 중지 시그널 무시

    maxfd = getdtablesize();       // 최대 파일 디스크립터 개수 반환
    for (fd = 0; fd < maxfd; fd++) // for debug , fd=3
        close(fd);                 // fd 닫기

    umask(0); // umask 0으로 초기화
    //	chdir("/");
    fd = open("/dev/null", O_RDWR); // 입출력을 모두 /dev/null로 리디렉션
    dup(0);                         // 표준 입력 복제
    dup(0);                         // 표준 출력 복제

    return getpid(); // child process 아이디 리턴
}

// dirpath에 저장 된 경로가 이미 모니터링 중인 디렉토리의 경로(saved_path)와 같거나/포함하거나/포함되는 지 확인하는 함수
bool is_already_monitored(char *dirpath)
{
    FILE *fp;          // monitor_list.txt 가리킬 파일 디스크립터
    char line[BUFLEN]; // monitor_list.txt 파일을 한 줄 씩 가져올 변수

    if ((fp = fopen(monitor_list, "r")) == NULL)
    {                                                            // 파일을 읽기 모드로 오픈
        fprintf(stderr, "fopen error for \"%s\"", monitor_list); // 오픈 실패 시 에러 메세지 출력
        exit(1);                                                 // 프로그램 비정상 종료
    }

    while (fgets(line, sizeof(line), fp) != NULL)
    {                                           // monitor_list.txt 파일에 저장된 문자열 한 줄씩 line 변수에 저장
        char *saved_path = strtok(line, " \n"); // monitor_list.txt 파일에 저장된 dirpath만 추출해 saved_path 변수에 저장
        if (saved_path)
        { // monitor_list.txt 파일에서 가져온 dirpath가 NULL이 아닌 경우
            if (!strcmp(saved_path, dirpath))
            {                                                                                     // 첫 번째 인자로 입력 받은 경로(dirpath)가 이미 모니터링 중인 디렉토리의 경로(saved_path)와 같은 경우
                fprintf(stderr, "\"%s\"는 이미 모니터링 중인 디렉토리의 경로와 같음\n", dirpath); // 에러 메세지 출력
                fclose(fp);                                                                       // monitor_list.txt 파일 닫기
                return true;                                                                      // true 리턴
            }
            else if (strstr(dirpath, saved_path) != NULL)
            { // 첫 번째 인자로 입력 받은 경로(dirpath) 문자열에서 이미 모니터링 중인 디렉토리의 경로(saved_path)와 일치하는 문자열이 존재
                // ex) dirpath = "/Users/yujimin/monitor/testdir/a", saved_path = "/Users/yujimin/monitor/testdir"
                // saved_path가 dirpath 포함하는지 확인해야 함
                size_t saved_path_len = strlen(saved_path); // saved_path 경로 길이 저장
                size_t dirpath_len = strlen(dirpath);       // dirpath 경로 길이 저장
                if (dirpath_len > saved_path_len)
                {
                    if (dirpath[saved_path_len] == '/' && !strncmp(dirpath, saved_path, saved_path_len))
                    {
                        fprintf(stderr, "\"%s\"는 이미 모니터링 중인 디렉토리의 경로에 포함됨\n", dirpath);
                        fclose(fp);  // monitor_list.txt 파일 닫기
                        return true; // true 리턴
                    }
                }
            }
            else if (strstr(saved_path, dirpath) != NULL)
            { // 이미 모니터링 중인 디렉토리의 경로(saved_path)에서 첫 번째 인자로 입력 받은 경로(dirpath)와 일치하는 문자열이 존재
                // ex) dirpath = "/Users/yujimin", saved_path = "/Users/yujimin/monitor/testdir"
                // dirpath가 saved_path를 포함하는지 확인해야 함
                size_t saved_path_len = strlen(saved_path); // saved_path 경로 길이 저장
                size_t dirpath_len = strlen(dirpath);       // dirpath 경로 길이 저장
                if (saved_path_len > dirpath_len)
                {
                    if (saved_path[dirpath_len] == '/' && !strncmp(saved_path, dirpath, dirpath_len))
                    {
                        fprintf(stderr, "\"%s\"는 이미 모니터링 중인 디렉토리의 경로를 포함\n", dirpath);
                        fclose(fp);  // monitor_list.txt 파일 닫기
                        return true; // true 리턴
                    }
                }
            }
        }
    }
    // 위의 세 가지 케이스에 포함되지 않을 경우
    fclose(fp);   // monitor_list.txt 파일 닫기
    return false; // false 리턴
}

// 파일 이름을 필터링하여 특정 파일들을 무시하는 함수
int scandir_filter(const struct dirent *file)
{
    // 파일 이름이 ".", "..", ".DS_Store", "log.txt", monitor_list, 또는 ".swp"와 일치하는 경우 무시
    if (!strcmp(file->d_name, ".") || !strcmp(file->d_name, "..") || !strcmp(file->d_name, ".DS_Store") || !strcmp(file->d_name, "log.txt") || !strcmp(file->d_name, monitor_list) || strstr(file->d_name, ".swp") != NULL)
    {

        return 0;
    }
    else
        return 1;
}

// 노드 생성
tree *create_node(char *path, mode_t mode, time_t mtime)
{
    tree *new; // 새로운 트리 노드를 가리키는 포인터

    new = (tree *)malloc(sizeof(tree)); // 동적 메모리 할당을 통해 새로운 노드 생성

    strcpy(new->path, path); // 주어진 경로를 새로운 노드의 경로로 복사
    new->mode = mode;        // 주어진 모드 값을 새로운 노드의 모드로 설정
    new->mtime = mtime;      // 주어진 수정 시간 값을 새로운 노드의 수정 시간으로 설정

    // 초기화
    new->isEnd = false; // isEnd 초기값을 false로 설정
    new->next = NULL;   // 다음 노드에 대한 포인터를 NULL로 초기화
    new->prev = NULL;   // 이전 노드에 대한 포인터를 NULL로 초기화
    new->child = NULL;  // 자식 노드에 대한 포인터를 NULL로 초기화
    new->parent = NULL; // 부모 노드에 대한 포인터를 NULL로 초기화

    return new; // 생성된 새로운 노드 반환
}

// tree 노드 메모리 해제
void free_tree(tree *node)
{
    if (node == NULL)
        return;

    // 재귀적으로 자식 노드와 형제 노드를 해제
    free_tree(node->child);
    free_tree(node->next);
    free(node);
}

// 모니터링 위한 트리 생성 함수
void make_tree(tree *dir, char *path)
{
    struct dirent **dirp;  // dp가 가리키는 디렉토리 탐색하면서 내부 파일 혹은 디렉토리를 가리키는 스트림 포인터
    struct stat statbuf;   // 파일 또는 디렉토리의 정보를 저장하는 구조체
    char new_path[BUFLEN]; // 새로운 경로를 저장할 문자열 배열
    int count;             // 디렉토리 내 파일/디렉토리 개수
    int i;                 // 반복문을 위한 변수

    // 주어진 경로에서 디렉토리 내의 파일 및 디렉토리를 스캔하여 dirp에 저장
    if ((count = scandir(path, &dirp, scandir_filter, alphasort)) < 0)
    {
        fprintf(stderr, "scandir error for %s\n", path);
        return;
    }

    // 디렉토리 내의 파일/디렉토리에 대해 반복 수행
    for (i = 0; i < count; i++)
    {
        tree *new_node; // 새로운 트리 노드를 가리키는 포인터
        tree *temp;     // 임시 트리 노드를 가리키는 포인터

        bzero(new_path, BUFLEN);                           // new_path 배열을 0으로 초기화
        sprintf(new_path, "%s/%s", path, dirp[i]->d_name); // 새로운 경로 생성

        // 새로운 경로에 대한 정보를 얻어옴
        if (stat(new_path, &statbuf) < 0)
        {
            fprintf(stderr, "stat error for %s\n", new_path);
            continue;
        }

        // 새로운 노드 생성
        new_node = create_node(new_path, statbuf.st_mode, statbuf.st_mtime);
        new_node->size = statbuf.st_size; // 파일 또는 디렉토리의 크기 저장

        // 첫 번째 노드인 경우, dir의 child로 설정
        if (i == 0)
        {
            dir->child = new_node;
        }
        else
        {
            temp->next = new_node; // 이전 노드의 next로 새로운 노드 연결
            new_node->prev = temp; // 새로운 노드의 prev로 이전 노드 연결
        }

        temp = new_node; // temp를 새로운 노드로 업데이트

        // 마지막 노드인 경우, isEnd를 true로 설정
        if (i == count - 1)
            new_node->isEnd = true;

        // 새로운 경로가 디렉토리인 경우, 재귀적으로 make_tree 호출하여 하위 트리 생성
        if (S_ISDIR(statbuf.st_mode))
            make_tree(new_node, new_path);
    }
}

// 재귀적으로 디렉토리 트리를 생성
void execute_tree_re(tree *parent, const char *parentpath)
{
    DIR *dir;
    struct dirent *dirp;

    if ((dir = opendir(parentpath)) == NULL) // 디렉토리 열기 실패한 경우(디렉토리 아닌 경우임)
        return;

    while ((dirp = readdir(dir)) != NULL)
    { // 디렉토리 순회
        if (!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, "..") || !strcmp(dirp->d_name, ".DS_Store") || !strcmp(dirp->d_name, "log.txt") || !strcmp(dirp->d_name, monitor_list))
            continue;

        tree *node = (tree *)malloc(sizeof(tree));
        snprintf(node->path, BUFLEN, "%s/%s", parentpath, dirp->d_name);
        node->next = NULL;
        node->child = NULL;

        if (parent->child == NULL)
            parent->child = node;
        else
        {
            tree *temp = parent->child;
            while (temp->next != NULL)
                temp = temp->next;
            temp->next = node;
        }

        char subdirpath[PATH_MAX];
        snprintf(subdirpath, PATH_MAX, "%s/%s", parentpath, dirp->d_name);
        execute_tree_re(node, subdirpath);
    }

    closedir(dir);
}

// tree 구조를 재귀적으로 호출하며 출력
void print_tree_re(tree *node, int depth, int is_last)
{

    if (node == NULL)
        return;

    // printf("%s %s %d %d\n", node->path, (node->child)? "child" : "no", depth, is_last);

    for (int i = 0; i < depth; i++)
    {
        if (i == depth - 1) // 해당 디렉토리에 더이상 파일/디렉토리(형제노드)가 없을 경우
            printf("└── ");
        else if (node->child || node->next != NULL) // 자식 노드가 있거나, 마지막 노드가 아니면
            printf("│   ");
        else // 나머지 경우 공백 출력해야함
            printf("    ");
    }

    char *filename = strrchr(node->path, '/');
    // printf("filename : %s \t\t depth : %d\n", filename, depth);
    if (filename != NULL)
        printf("%s\n", filename + 1);
    else
        printf("%s\n", node->path);

    print_tree_re(node->child, depth + 1, 0); // 자식 노드 재귀 호출
    print_tree_re(node->next, depth, 1);      // 형제 노드 재귀 호출
}

// log.txt에 로그 기록
void append_log(char *log)
{
    log_fp = fopen(log_path, "a"); // dirpath 디렉토리 내에 log.txt 생성
    if (log_fp == NULL)
    {
        fprintf(stderr, "fopen error for \"%s\"\n", "log.txt");
        exit(1);
    }
    fprintf(log_fp, "%s\n", log);
    fflush(log_fp); // 버퍼 비우기
    fclose(log_fp);
}

// 재귀적으로 tree 비교하며 변경사항 확인
void compare_tree(tree *old, tree *new)
{
    time_t curtime;       // 현재 시간을 저장하는 변수
    struct tm *t;         // 시간 정보를 저장하는 구조체 포인터
    char daystr[BUFLEN];  // 날짜 및 시간을 문자열로 저장하는 배열
    char message[BUFLEN]; // 메시지를 저장하는 배열

    curtime = time(NULL);    // 현재 시간을 가져옴
    t = localtime(&curtime); // 현재 시간을 지역 시간으로 변환하여 t에 저장

    memset(daystr, 0, BUFLEN); // daystr 배열을 0으로 초기화
    sprintf(daystr, "%d-%02d-%02d %02d:%02d:%02d", t->tm_year + 1900, t->tm_mon + 1,
            t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec); // daystr에 현재 시간을 형식화하여 저장

    if (old != NULL && new != NULL)
    {
        // 수정된 파일/디렉토리 비교
        if (strcmp(old->path, new->path) != 0)
        {
            // 삭제 또는 생성에 대한 작업 수행
            if (old->next && strcmp(old->next->path, new->path) == 0)
            {
                // 파일/디렉토리 삭제됨
                sprintf(message, "[%s][remove][%s]", daystr, old->path);
                append_log(message);
                old = old->next;
            }
            else if (new->next && strcmp(new->next->path, old->path) == 0)
            {
                // 파일/디렉토리 생성됨
                sprintf(message, "[%s][create][%s]", daystr, new->path);
                append_log(message);
                new = new->next;
            }
            else if (new->next && !old)
            {
                // 파일/디렉토리 생성됨 (old에는 없는 파일/디렉토리)
                sprintf(message, "[%s][create][%s]", daystr, new->path);
                append_log(message);
                new = new->next;
            }
        }

        if (S_ISREG(old->mode) && S_ISREG(new->mode))
        {
            // 정규 파일 비교
            if (old->mtime != new->mtime || (old->size == 0 && new->size > 0) || (old->size > 0 && new->size == 0))
            {
                // 수정 시간이 변경되었거나 크기가 0인 파일이 생성 또는 삭제된 경우에도 로그 출력
                sprintf(message, "[%s][modify][%s]", daystr, new->path);
                append_log(message);
            }
        }

        // 자식 노드들을 재귀적으로 비교
        compare_tree(old->child, new->child);
        compare_tree(old->next, new->next); // 형제 노드들을 비교
    }
    else if (!old && new != NULL)
    {
        // 파일/디렉토리 생성됨 (old에는 없는 파일/디렉토리)
        sprintf(message, "[%s][create][%s]", daystr, new->path);
        append_log(message);
        compare_tree(NULL, new->next); // 다음 노드와 비교
    }
    else if (!new && old != NULL)
    {
        // 파일/디렉토리 삭제제됨 
        sprintf(message, "[%s][remove][%s]", daystr, old->path);
        append_log(message);
        compare_tree(NULL, old->next); // 다음 노드와 비교
    }
}






  

// usage 출력 함수
void print_usage(void)
{
    printf("Usage : ssu_moniter\n");
    printf("	> add <DIRPATH> [OPTION]\n");
    printf("	 -t <TIME> : 디몬 프로세스는 <TIME> 간격을 두고 모니터링 재시작\n");
    printf("	> delete <DEMON_PID>\n");
    printf("	> tree <DIRPATH>\n");
    printf("	> help\n");
    printf("	> exit\n");

    return;
}

// 입력 문자열인 str을 구분자인 del로 나누어 각 부분 문자열을 추출하고, 추출된 부분 문자열을 리턴
char **GetSubstring(char *str, int *cnt, char *del)
{
    *cnt = 0;
    int i = 0;
    char *token = NULL;
    char *templist[100] = {
        NULL,
    };
    token = Tokenize(str, del);
    if (token == NULL)
    {
        return NULL;
    }

    while (token != NULL)
    {
        templist[*cnt] = token;
        *cnt += 1;
        token = Tokenize(NULL, del);
    }

    char **temp = (char **)malloc(sizeof(char *) * (*cnt + 1));
    for (i = 0; i < *cnt; i++)
    {
        temp[i] = templist[i];
    }
    return temp;
}

// 입력된 문자열(str)을 구분자(del)를 기준으로 토큰으로 분리하여 반환
char *Tokenize(char *str, char *del)
{
    int i = 0;
    int del_len = strlen(del);
    static char *tmp = NULL;
    char *tmp2 = NULL;

    if (str != NULL && tmp == NULL)
    {
        tmp = str;
    }

    if (str == NULL && tmp == NULL)
    {
        return NULL;
    }

    char *idx = tmp;

    while (i < del_len)
    {
        if (*idx == del[i])
        {
            idx++;
            i = 0;
        }
        else
        {
            i++;
        }
    }
    if (*idx == '\0')
    {
        tmp = NULL;
        return tmp;
    }
    tmp = idx;

    while (*tmp != '\0')
    {
        if (*tmp == '\'' || *tmp == '\"')
        {
            QuoteCheck(&tmp, *tmp);
            continue;
        }
        for (i = 0; i < del_len; i++)
        {
            if (*tmp == del[i])
            {
                *tmp = '\0';
                break;
            }
        }
        tmp++;
        if (i < del_len)
        {
            break;
        }
    }

    return idx;
}

// 입력된 문자열(str) 내의 따옴표를 확인하고 처리
char *QuoteCheck(char **str, char del)
{
    char *tmp = *str + 1;
    int i = 0;

    while (*tmp != '\0' && *tmp != del)
    {
        tmp++;
        i++;
    }
    if (*tmp == '\0')
    {
        *str = tmp;
        return NULL;
    }
    if (*tmp == del)
    {
        for (char *c = *str; *c != '\0'; c++)
        {
            *c = *(c + 1);
        }
        *str += i;
        for (char *c = *str; *c != '\0'; c++)
        {
            *c = *(c + 1);
        }
    }
    return *str;
}