#include <iostream>
#include <fstream>
#include <string.h>
#include <curl/curl.h>
#include "llist.h"
#include "hash.h"
#include "common_util.h"

using namespace std;

//struct mbuf
//{
//    char buf[512];
//    int  len; 
//};
//
//void mbuf_dtor(void *a, void *b)
//{
//    if (b != NULL) {
//        free(b);
//    }
//} void test_01()
//{
//    // 1. 创建 struct mbuf 的结构体
//    struct curl_llist *buf_list = Curl_llist_alloc(mbuf_dtor);
//    // 2. 添加成员
//    struct mbuf *a = (struct mbuf*)malloc(sizeof(struct mbuf));
//    snprintf(a->buf, sizeof(a->buf), "test_a");
//    a->len = strlen(a->buf);
//    Curl_llist_insert_next(buf_list, buf_list->head, a);
//    // 3. 获取成员数量
//    size_t cnt = Curl_llist_count(buf_list);
//    printf("cnt:%d\n", cnt);
//    // 4. 销毁成员
//    Curl_llist_destroy(buf_list, NULL);
//}
//
//void my_dtor(void* p)
//{
//   int *ptr = (int*)p;
//   free(ptr);
//}
//
//void test_02()
//{
//    // 1. 创建 hash 表
//    struct curl_hash h;
//    Curl_hash_init(&h, 7, Curl_hash_str, Curl_str_key_compare, my_dtor);
//    // 2. 添加 hash 参数
//    int *value;
//    int *value2;
//    int *nodep;
//    size_t klen = sizeof(int);
//
//    int key = 20;
//    int key2 = 25;
//
//    value = (int*)malloc(sizeof(int));
//    *value = 199;
//    nodep = (int*)Curl_hash_add(&h, &key, klen, value);
//    if (!nodep)
//        free(value);
//    Curl_hash_clean(&h);
//
//    value2 = (int*)malloc(sizeof(int));
//    *value2 = 204;
//    nodep = (int*)Curl_hash_add(&h, &key2, klen, value2);
//    if (!nodep)
//        free(value2);
//    Curl_hash_clean(&h);
//    // 3. 销毁
//}

typedef struct tagHTTPHeader { int status;
	int contentLength;
    struct mbuf X_Subject_Token;
} HTTPHeader;

typedef struct tagCurlWriteData {
	struct mbuf body;
} CurlWriteData;

typedef struct tagCurlReadData {
	void* ptr;
	int size;
	int pos;
} CurlReadData;

typedef struct tagCurlRestfulData {
	HTTPHeader header;
	CurlWriteData writeData;
	CurlReadData readData;
} CurlRestfulData;

/**
 @brief 处理HTTP返回的Header的回调接口
 @note
 @see
*/
static size_t CB_ParseHeader(void *ptr, size_t size, size_t nmemb, void *stream)
{
	int r;  
	int len = 0;  
	CurlRestfulData* restfulData = (CurlRestfulData*)stream;
	HTTPHeader* header = &restfulData->header;

	r = sscanf((const char*)ptr, "HTTP/1.0  %d\n", &len);
	if (r)
		header->status = len;

	r = sscanf((const char*)ptr, "HTTP/1.1  %d\n", &len);
	if (r)
		header->status = len;

	r = sscanf((const char*)ptr, "Content-Length:  %d\n", &len);
	if (r)
		header->contentLength = len;

    if (0 == strncasecmp((const char*)ptr, "X-Subject-Token:", 16))
    {
		char *tmp_token = (char*)ptr + 16;
		size_t token_len = strlen(tmp_token);
		// 过滤掉空格
		while (token_len > 0 && isspace((int) *tmp_token)) 
		{
			tmp_token = tmp_token + 1;
			token_len--;
		}
		if (token_len > 2)
		{
			// 需要剔除"\r\n"
			mbuf_insert(&header->X_Subject_Token, 0, tmp_token, token_len-2);
		}
    }
    
	return size * nmemb;
}

static size_t CB_WriteData(void *ptr, size_t size, size_t nmemb, void *pUsrData)
{
	CurlRestfulData* restfulData = (CurlRestfulData*)pUsrData;
	mbuf_append(&restfulData->writeData.body, ptr, size*nmemb);
	return size*nmemb;
}

static size_t CB_ReadData(char *bufptr, size_t size, size_t nitems, void *pUsrData)
{
	CurlRestfulData* restfulData = (CurlRestfulData*)pUsrData;
	CurlReadData* data = &restfulData->readData;

	int left = data->size - data->pos;
	int copyLen = (left >= size*nitems)?size*nitems:left;
	memcpy(bufptr, data->ptr + data->pos, copyLen);
	data->pos += copyLen;

	return copyLen;
}

void common_util_save2file(const char *filename, void *s, int len, int type)
{
	if (NULL == filename || NULL == s || len <= 0)
	{
		return;
	}

	std::ofstream openfile;
	if (0 == type)
	{
		openfile.open(filename);
	}
	else
	{
		openfile.open(filename, std::ios::app);
	}
	openfile.write((char*)s, len);
	openfile.close();
}

static void test_curl_httpget()
{
	CurlRestfulData restfulData;

	memset(&restfulData, 0, sizeof(restfulData));
    mbuf_init(&restfulData.header.X_Subject_Token, 0);
	mbuf_init(&restfulData.writeData.body, 0);

    curl_global_init(CURL_GLOBAL_ALL);
    CURL *curl = curl_easy_init();

    // 设置url 
    const char *url = "http://192.168.3.128/Network/P2PV2";
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_USERPWD, "admin:");
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, CB_ParseHeader);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &restfulData);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CB_WriteData);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &restfulData);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 2000L);
    
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
		fprintf(stderr, "curl_easy_perform() failed: %s\n", 
				curl_easy_strerror(res));
    }

    common_util_save2file("./result.txt", restfulData.writeData.body.buf, restfulData.writeData.body.len, 1);

    mbuf_free(&restfulData.writeData.body);
    mbuf_free(&restfulData.header.X_Subject_Token);

    curl_easy_cleanup(curl);
    curl_global_cleanup();
}

int main(int argc, char** argv)
{
    //test_01();
    //test_02();
    test_curl_httpget();

    return 0;
}
