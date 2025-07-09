#include "csapp.h"
#include "sbuf.h"


// 제한된 용량의 빈 FIFO 버퍼 초기화
// n: 최대 슬롯 수
void sbuf_init(sbuf_t *sp, int n){
    sp -> buf = Calloc(n, sizeof(int));
    sp -> n = n;                            // 최대 n개 아이템
    sp -> front = sp -> rear = 0;           // front, rear가 0일 때"만" 버퍼가 빔.
    Sem_init(&sp->mutex, 0, 1);             // 잠금용
    Sem_init(&sp->slots, 0, n);             // 처음 가용 슬롯은 n개
    Sem_init(&sp->items, 0, 0);             // 처음 가용 슬롯은 0개
}


void sbuf_deinit(sbuf_t *sp){
    Free(sp -> buf);                        // 버퍼 비우기
}

// 버퍼의 끝에 아이템을 추가
void sbuf_insert(sbuf_t *sp, int item){
    P(&sp->slots);                          // 슬롯이 없는 경우 생길 때까지 기다림
    P(&sp->mutex);                          // 버퍼를 잠금
    (sp -> buf)[(++(sp -> rear))%(sp -> n)] = item;
    V(&sp->mutex);                          // 잠금 해제
    V(&sp->items);                          // 사용중 아이템 하나 추가
}

// 버퍼에서 첫 아이템을 제거하고 반환
int sbuf_remove(sbuf_t *sp){
    int item;
    P(&sp->items);                          // 아이템이 없는 경우 생길 때까지 기다림
    P(&sp->mutex);
    item = (sp -> buf)[(++(sp -> front)) % (sp -> n)];
    V(&sp->mutex);
    V(&sp->slots);
    return item;
}