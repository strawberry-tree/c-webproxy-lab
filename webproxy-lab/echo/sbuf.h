typedef struct {
    int *buf;       // 버퍼, 즉 배열
    int n;          // 최대 슬롯 개수
    int front;      // buf[(front + 1) % n] -> 첫 아이템
    int rear;       // buf[rear % n] -> 마지막 아이템
    sem_t mutex;    // buf 상호배타적 접근
    sem_t slots;    // 사용 가능 슬롯 수
    sem_t items;    // 사용 가능 아이템 수
} sbuf_t;

void sbuf_init(sbuf_t *sp, int n);
void sbuf_deinit(sbuf_t *sp);
void sbuf_insert(sbuf_t *sp, int item);
int sbuf_remove(sbuf_t *sp);
