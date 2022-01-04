#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// キューの定義
typedef struct {
  unsigned char front, rear, count, size;
  unsigned char *buff;
} Queue;

// キューの生成
Queue *make_queue(char n)
{
    printf("Queue malloc: %ld\n",sizeof(Queue));
  Queue *que = malloc(sizeof(Queue));
  if (que != NULL) {
    que->front = 0;
    que->rear = 0;
    que->count = 0;
    que->size = n;
    printf("malloc: %ld\n", sizeof(long)*n);
    que->buff = malloc(sizeof(long) * n );
    if (que->buff == NULL) {
      free(que);
      return NULL;
    }
  }
  return que;
}

// キューは満杯か
bool is_full(Queue *que)
{
  return que->count == que->size;
}

// キューは空か
bool is_empty(Queue *que)
{
  return que->count == 0;
}

// データの挿入
bool enqueue(Queue *que, unsigned char x)
{
  if (is_full(que)) return false;
  que->buff[que->rear++] = x;
  que->count++;
  if (que->rear == que->size)
    que->rear = 0;
  return true;
}

// データを取り出す
char dequeue(Queue *que, bool *err)
{
  if (is_empty(que)) {
    *err = false;
    return 0;
  }
  double x = que->buff[que->front++];
  que->count--;
  *err = true;
  if (que->front == que->size)
    que->front = 0;
  return x;
}

// 先頭データを参照する
char top(Queue *que, bool *err)
{
  if (is_empty(que)) {
    *err = false;
    return 0;
  }
  return que->buff[que->front];
}

// キューをクリアする
void clear(Queue *que)
{
  que->front = que->rear = que->count = 0;
}

// キューの個数を求める
int queue_length(Queue *que)
{
  return que->count;
}

// キューの削除
void queue_delete(Queue *que)
{
  free(que->buff);
  free(que);
}

// 簡単なテスト
char main(void)
{
  Queue *que = make_queue(127);
  bool err;
  for (int i = 0; i < 16; i++)
    enqueue(que, i);
  printf("%d\n", queue_length(que));
  printf("%d\n", is_empty(que));
  printf("%d\n", is_full(que));
  while (!is_empty(que))
    printf("%d\n", dequeue(que, &err));
  printf("%d\n", queue_length(que));
  printf("%d\n", is_empty(que));
  printf("%d\n", is_full(que));
  for (int i = 0; i < 64; i++)
    enqueue(que, i);
  printf("%d\n", queue_length(que));
  printf("%d\n", is_empty(que));
  printf("%d\n", is_full(que));
  while (!is_empty(que))
    printf("%d\n", dequeue(que, &err));
  queue_delete(que);
  return 0;
}