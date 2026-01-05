# scoring (ssu_score)

시험 답안(빈칸 채우기 `.txt` + 프로그램 작성 `.c`)을 **정답 디렉토리(ANS_DIR)** 기준으로 채점하고, 학생별/문항별 점수를 **CSV로 저장**하는 채점 프로그램입니다. 

---

## 1. 폴더 구조

```text
scoring/
├── main.c
├── ssu_score.c
├── ssu_score.h
├── blank.c
├── blank.h
├── Makefile
├── ANS_DIR/                 # 정답 파일 디렉토리 (.txt, .c)
└── STD_DIR/                 # 학생 답안 디렉토리 (학번/문제파일)
    ├── 20200001/
    ├── 20200002/
    └── ...
```

* **빈칸 채우기 문제**: `*.txt`
* **프로그램 문제**: `*.c`
* 채점 결과는 실행 시 **`score.csv`** 로 생성/저장됩니다(옵션으로 파일명 변경 가능). 
* 문항별 배점 테이블 **`score_table.csv`** 는 최초 실행 시 생성되며 이후 채점에 사용됩니다. 

---

## 2. 빌드 & 실행

### 빌드

```bash
make
```

### 기본 실행

```bash
./ssu_score <STD_DIR> <ANS_DIR>
```

예)

```bash
./ssu_score STD_DIR ANS_DIR
```

* `<STD_DIR>`: 학생 제출 폴더들이 있는 디렉토리 (ex. `STD_DIR/20200001/...`)
* `<ANS_DIR>`: 정답 파일들이 있는 디렉토리

채점 프로그램을 처음 실행하면, 배점 테이블이 존재하지 않을 경우
사용자로부터 문제 유형별 배점을 입력받은 뒤 채점을 수행합니다.
채점 완료 후 결과 CSV가 저장됩니다. 

<img width="484" height="329" alt="image" src="https://github.com/user-attachments/assets/9559fa85-df93-4094-8572-20cc8d975a9d" />

---

## 3. 출력 파일

### score.csv (채점 결과)

* 학생별 문항 점수 + 총점(sum) 저장
* 점수는 **소수점 둘째 자리까지 출력** 

<img width="483" height="339" alt="image" src="https://github.com/user-attachments/assets/a5023e22-80b7-4b96-b800-dc38b1fcd144" />



### score_table.csv (배점 테이블)

* 문항명(qname)과 배점(score)을 관리하는 기준 테이블
* 최초 실행 시 자동 생성되며, 이후 모든 채점은 이 배점을 기준으로 수행

> 예시 형식: `1-1.txt,0.5`

<img width="458" height="153" alt="image" src="https://github.com/user-attachments/assets/9a5b1ede-380b-406a-b55d-22b6930b1b72" />

---

## 4. 옵션(Options)

### `-n <CSVFILENAME>` : 결과 파일명 지정

```bash
./ssu_score STD_DIR ANS_DIR -n myscore.csv
```

* 결과 파일 `score.csv` 대신 `<CSVFILENAME>`로 저장
* **상대/절대경로 가능**
* 확장자가 `.csv`가 아니면 에러 처리

<img width="482" height="234" alt="image" src="https://github.com/user-attachments/assets/4c41e09e-a996-4a76-bd0c-909415c96ea6" />

---

### `-m` : 배점 수정

```bash
./ssu_score STD_DIR ANS_DIR -m
```

* `score_table.csv`의 특정 문항 배점을 사용자 입력으로 수정
* 종료하려면 `no` 입력
* `score_table.csv`가 없으면 에러 

<img width="480" height="301" alt="image" src="https://github.com/user-attachments/assets/412b37eb-8c96-40f0-9acc-5391013ca359" />

---

### `-c [STUDENTIDS...]` : 총점 출력

```bash
./ssu_score STD_DIR ANS_DIR -c 20200001 20200002
```

* 입력된 학번들의 총점 출력
* 학번 인자를 생략하면 **전체 학생 출력**
* 가변 인자 최대 5개(초과 입력은 무시 + 메시지 출력) 
* 마지막에 평균 출력 

<img width="486" height="256" alt="image" src="https://github.com/user-attachments/assets/b6033336-bb29-4846-acbe-19a5d3759b60" />

---

### `-p [STUDENTIDS...]` : 오답 문항(문항번호+배점) 출력

```bash
./ssu_score STD_DIR ANS_DIR -p 20200001
```

* 학생별로 틀린 문항을 `문제번호(배점)` 형태로 출력 
* 학번 인자를 생략하면 **전체 학생 출력**
* 가변 인자 최대 5개 

> ⚠️ **주의**
> `-c` 옵션과 `-p` 옵션은 동일한 가변 인자(STUDENTIDS)를 공유합니다.  
> 따라서 학번 인자 그룹은 한 번만 입력해야 하며,
> 두 번 입력될 경우 에러 처리 후 프로그램이 종료됩니다.


<img width="482" height="346" alt="image" src="https://github.com/user-attachments/assets/8cfd1542-bf00-4027-b61e-361cdfd9c914" />

---

### `-t [QNAMES...]` : 특정 문제 컴파일 시 `-lpthread` 추가

```bash
./ssu_score STD_DIR ANS_DIR -t 28 29
```

* 지정한 문제 번호를 컴파일할 때 `-lpthread`를 추가 
* 최신 컴파일러 환경에서는 결과 차이가 없을 수 있음 
* 가변 인자 최대 5개(초과 입력은 무시 + 메시지 출력) 

<img width="483" height="250" alt="image" src="https://github.com/user-attachments/assets/d7e76e57-bb81-4bb5-af2d-ddde529e2cdf" />

---

### `-s <CATEGORY> <1|-1>` : 결과 정렬 후 저장

```bash
./ssu_score STD_DIR ANS_DIR -s score -1
```

* `<CATEGORY>`: `score` 또는 `stdid`
* `<1|-1>`: `1` 오름차순, `-1` 내림차순
* 정렬된 결과를 CSV로 저장

<img width="482" height="237" alt="image" src="https://github.com/user-attachments/assets/4522290d-cf9e-426f-ac37-d7fdbba8be79" />
<img width="416" height="140" alt="image" src="https://github.com/user-attachments/assets/2ee76af0-52c8-4ed0-8d1a-0fb559543f22" />

---

### `-e <DIRNAME>` : 에러 로그 파일 저장

```bash
./ssu_score STD_DIR ANS_DIR -e error
```

* `<DIRNAME>/<학번>/<문제번호>_error.txt` 형태로 에러 메시지 저장 
* `<DIRNAME>`이 없으면 생성
* 학생별 서브 디렉토리도 자동 생성 

<img width="482" height="333" alt="image" src="https://github.com/user-attachments/assets/ffa8b5f9-82df-4a45-ad68-31b34168702f" />
<img width="482" height="232" alt="image" src="https://github.com/user-attachments/assets/9a782a64-18c8-482b-a304-f46d920e9fa1" />


---

### `-h` : 도움말 출력

```bash
./ssu_score -h
```

* 사용법 및 옵션 설명 출력
* 다른 옵션과 같이 쓰면 도움말 출력 후 종료

<img width="485" height="182" alt="image" src="https://github.com/user-attachments/assets/83ce2e28-a321-4003-a66b-9abb872faca0" />


---

## 5. 채점 방식 요약

* `.txt` (빈칸 채우기): 정답 파일과 비교하여 채점 
* `.c` (프로그램 문제):

  * 컴파일 및 실행 결과 비교(결과 파일 비교 시 대소문자 구분 완화 로직 포함) 
  * 코드 구조 비교를 위해 토큰화/트리 구성 후 비교 로직 사용


### Call Graph (채점 흐름 및 주요 함수 관계)

* `main → ssu_score → score_student`를 중심으로 전체 채점 흐름이 구성됨
* 프로그램 문제 채점 시 `compile_program`, `execute_program`,
  코드 구조 비교를 위한 `make_tree`, `compare_tree` 로직으로 분기됨

<img width="507" height="379" alt="image" src="https://github.com/user-attachments/assets/10a7882b-f0f2-4fe8-8b12-fe125f684060" />


---

## 6. 참고(개발/디버깅)

* `STD_DIR/<학번>/` 내부에 채점 대상 파일들이 존재해야 합니다.
* 이전 실행 결과(`score.csv`, `score_table.csv`, `*_error.txt`)가 남아있다면 테스트 시 결과가 섞일 수 있으니 필요 시 정리 후 실행하세요.
