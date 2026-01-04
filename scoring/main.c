#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include "ssu_score.h"

#define SECOND_TO_MICRO 1000000		// 초 단위 -> 마이크로초 단위로 변환

void ssu_runtime(struct timeval *begin_t, struct timeval *end_t);	// 프로그램 실행 시간 측정 함수

int main(int argc, char *argv[])
{
	// Usage : ssu_score <STUDENTDIR> <TRUEDIR> [OPTION] 
	// 이므로 인자 최소 3개 이상이어야 함
 	if (argc < 3) {
		print_usage();
		exit(1);
	}

	struct timeval begin_t, end_t;	// 실행 시간 측정 위한 구조체 변수 선언
	gettimeofday(&begin_t, NULL);	// 시작 시간을 측정해 begin_t 구조체에 저장

	ssu_score(argc, argv);			// 채점 함수 호출

	gettimeofday(&end_t, NULL);		// 종료 시간을 측정해 end_t 구조체에 저장
	ssu_runtime(&begin_t, &end_t);	// 실행 시간 계산 함수 호출

	exit(0);
}

// 프로그램 실행 시간 측정 함수
void ssu_runtime(struct timeval *begin_t, struct timeval *end_t)
{
	// 종료 시간 -= 시작 시간  (프로그램 실행 시간 계산)
	end_t->tv_sec -= begin_t->tv_sec;	

	// 계산한 실행 시간이 시작 시간보다 작을 경우, 이전 초 단위의 마이크로초 값 보정
	if(end_t->tv_usec < begin_t->tv_usec){		
		end_t->tv_sec--;
		end_t->tv_usec += SECOND_TO_MICRO;		
	}

	// 보정 값으로 프로그램 실행 시간 다시 계산
	end_t->tv_usec -= begin_t->tv_usec;
	printf("Runtime: %ld:%06ld(sec:usec)\n", end_t->tv_sec, end_t->tv_usec);	// 프로그램 실행 시간 출력
}
