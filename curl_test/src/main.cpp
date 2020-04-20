#include <iostream>
#include <string.h>
#include <curl/curl.h>
#include "llist.h"

using namespace std;

struct mbuf
{
    char buf[512];
    int  len; 
};

void mbuf_dtor(void *a, void *b)
{
    if (b != NULL) {
        free(b);
    }
}

void test_01()
{
    // 1. 创建 struct mbuf 的结构体
    struct curl_llist *buf_list = Curl_llist_alloc(mbuf_dtor);
    // 2. 添加成员
    struct mbuf *a = (struct mbuf*)malloc(sizeof(struct mbuf));
    snprintf(a->buf, sizeof(a->buf), "test_a");
    a->len = strlen(a->buf);
    Curl_llist_insert_next(buf_list, buf_list->head, a);
    // 3. 获取成员数量
    size_t cnt = Curl_llist_count(buf_list);
    printf("cnt:%d\n", cnt);
    // 4. 销毁成员
    Curl_llist_destroy(buf_list, NULL);
}

int main(int argc, char** argv)
{
    test_01();

    return 0;
}
