# monitoring (ssu_monitor)

지정한 디렉토리를 **디몬(daemon) 프로세스**로 모니터링하며,
파일의 **생성(create) / 수정(modify) / 삭제(remove)** 이벤트를 감지하여
변경 이력을 **로그 파일로 기록·관리**하는 디렉토리 모니터링 프로그램입니다.

---

## 1. 폴더 구조

```text
monitoring/
├── ssu_monitor.c        # 메인 로직
├── ssu_monitor.h        # 구조체 및 함수 선언
├── ssu_monitor          # 실행 파일
├── monitor_list.txt     # 모니터링 중인 디렉토리 목록 (경로, PID)
└── jimindir/            # 모니터링 대상 예시 디렉토리
    ├── a/
    │   ├── a.txt
    │   └── b/
    │       └── a.txt
    ├── a.txt
    ├── b.txt
    └── c.txt
```

* **monitor_list.txt**

  * 현재 모니터링 중인 디렉토리의 **절대 경로 + 디몬 PID** 저장
* 각 디렉토리는 **독립적인 디몬 프로세스**로 모니터링됨
* 파일 변경 이벤트는 **log.txt**에 기록됨

---

## 2. 빌드 & 실행

### 빌드

```bash
gcc -o ssu_monitor ssu_monitor.c
```

또는 제공된 실행 파일 사용:

```bash
./ssu_monitor
```

### 기본 실행

```bash
./ssu_monitor
```

프로그램 실행 시 프롬프트가 출력되며,
내장 명령어(`add`, `delete`, `tree`, `help`, `exit`) 입력을 대기합니다.

```text
학번>
```

---

## 3. 내부 명령어 (Built-in Commands)

### `add <DIRPATH> [-t <TIME>]` : 디렉토리 모니터링 시작

```bash
add testdir
```

* `<DIRPATH>`에 대해 **디몬 프로세스 생성**
* 해당 디렉토리의 **절대 경로 + PID**를 `monitor_list.txt`에 저장
* 이후 파일 변경 사항을 주기적으로 감시

<img width="483" height="160" alt="image" src="https://github.com/user-attachments/assets/b66e1369-683b-4d9d-9902-0c51dd8f91f1" />

#### `-t <TIME>` 옵션

```bash
add testdir -t 2
```

* `<TIME>` 초 간격으로 디렉토리 상태를 재탐색하며 모니터링 수행

<img width="485" height="105" alt="image" src="https://github.com/user-attachments/assets/ac4c8870-5efa-4272-aa57-2c71e69871c1" />
<br>
<br>

**예외 처리**

* 인자 누락 또는 잘못된 옵션 → Usage 출력
* 경로가 존재하지 않거나 디렉토리가 아닌 경우 → 에러
* 이미 모니터링 중인 경로와 **같거나 / 포함하거나 / 포함되는 경우** → 에러

<img width="481" height="298" alt="image" src="https://github.com/user-attachments/assets/a07a120b-ef08-4116-9db2-79bd43eda4be" />  

> 첫 번째 인자 입력이 없거나 잘못 되었을 때 Usage 출력 후 프롬프트 재출력  
  
<img width="478" height="186" alt="image" src="https://github.com/user-attachments/assets/68d21294-e3e6-4e65-b425-c4d5daf2a61c" />  

> 첫 번째 인자로 입력 받은 경로가 이미 모니터링 중인 디렉토리의 경로와 같거나/포함하거나/포함될 때 에러 처리  
  
<img width="481" height="144" alt="image" src="https://github.com/user-attachments/assets/aa6d5918-afe6-40e8-bebc-a3e009bdbd13" />  

> -t 옵션의 사용이 올바르지 않은 경우 Usage 출력 후 프롬프트 재출력  
> - [OPTION]을 입력하지 않은 경우와 [OPTION] 자리에 숫자가 아닌 값, 혹은 0을 입력했을 때 예외 처리  



---

### `delete <DAEMON_PID>` : 모니터링 종료

```bash
delete 31888
```

* `<DAEMON_PID>`에 **SIGUSR1 시그널 전송**
* 해당 디몬 프로세스 종료
* `monitor_list.txt`에서 해당 항목 제거

<img width="480" height="122" alt="image" src="https://github.com/user-attachments/assets/4f2d0cb9-a095-47f8-b470-881a5c54c146" />

**예외 처리**

* PID가 `monitor_list.txt`에 존재하지 않는 경우 → 에러 출력 후 프롬프트 복귀

<img width="480" height="106" alt="image" src="https://github.com/user-attachments/assets/018e7efa-14ee-4349-b848-8146a4ecc13b" />


---

### `tree <DIRPATH>` : 디렉토리 구조 출력

```bash
tree testdir
```

* `monitor_list.txt`에 등록된 디렉토리만 출력 가능
* 디렉토리 구조를 **tree 형태**로 재귀 출력

**예외 처리**

* `<DIRPATH>`가 모니터링 목록에 없는 경우 → 에러

<img width="481" height="247" alt="image" src="https://github.com/user-attachments/assets/6c1431c7-e24f-4961-81e7-2090eec6aaa7" />


---

### `help` : 사용법 출력

```bash
help
```

* 내장 명령어 및 옵션 설명 출력

<img width="481" height="155" alt="image" src="https://github.com/user-attachments/assets/59b11b6f-11d0-4ec2-b4d6-0ae514c35a79" />


---

### `exit` : 프로그램 종료

```bash
exit
```

* 프롬프트 종료 및 프로그램 종료

<img width="477" height="38" alt="image" src="https://github.com/user-attachments/assets/5717bb90-f245-4288-95f5-e6daa09ab226" />

---

## 4. 로그 파일 (log.txt)

* 모니터링 중인 디렉토리에서 이벤트 발생 시 기록
* 기록 대상 이벤트:

  * **create** : 파일 생성
  * **modify** : 파일 수정
  * **remove** : 파일 삭제

**로그 기록 형식**

```text
[YYYY-MM-DD HH:MM:SS] [EVENT] /absolute/path/to/file
```

<img width="481" height="223" alt="image" src="https://github.com/user-attachments/assets/4d4e7c7f-7173-4d2a-833f-45e3e4c002fe" />


---

## 5. 동작 방식 요약

* `add` 명령어 입력 시 디렉토리마다 **독립 디몬 프로세스 생성**
* 디몬 프로세스는 주기적으로 디렉토리를 순회하며

  * 이전 상태 트리(old)
  * 현재 상태 트리(new)
    를 비교
* 트리 비교 결과를 통해 **생성/수정/삭제 이벤트 감지**
* 이벤트 발생 시 `log.txt`에 기록
* `delete` 명령어로 시그널 기반 안전 종료

### Call Graph (주요 흐름)

* `main → ssu_prompt → execute_command`
* `add` → `init_daemon` → `make_daemon` → 디몬 생성
* 디몬 내부에서 트리 생성(`create_node`) → 트리 비교(`compare_tree`)
* 시그널(`SIGUSR1`) 수신 시 디몬 정상 종료

<img width="496" height="141" alt="image" src="https://github.com/user-attachments/assets/391363c2-2a57-4ae4-9bdf-c827fe77df31" />


---

## 6. 참고 (개발 / 디버깅)

* 모니터링 대상은 **디렉토리만 가능**
* 동일하거나 포함 관계에 있는 디렉토리는 중복 등록 불가
* 비정상 종료 시 `monitor_list.txt`에 PID가 남아 있을 수 있으므로
  테스트 전 정리 권장
* 모든 파일 경로는 **절대 경로 기준**으로 관리됨
