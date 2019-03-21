#include "cachelab.h"
#include "malloc.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
/* 节点结构定义 */
typedef struct Node{
    int data;
    struct Node* next;
} Node;
/* 新建节点函数 */
Node* newNode(int data){
    Node* tmp = (Node*)malloc(sizeof(Node));
    tmp -> data = data;
    tmp -> next = 0;
    return tmp;
}

/* 链表定义 */
typedef struct{
    Node* head;
    Node* tail;
} LList;
/* 链表初始化函数 */
void initLList(LList* list){
    list -> head = list -> tail = newNode(0);
}
/* 尾部插入包含指定数据的节点 */
void insertTail(LList* list, int d){
    Node* tmp = newNode(d);
    list -> tail -> next = tmp;
    list -> tail = list -> tail -> next;
}
/* 删除表头节点，并返回其数据元素 */
int deleteFront(LList* list){
    Node* tmp = list -> head -> next;
    if(!tmp) return -1;
    list -> head -> next = tmp -> next;
    int data = tmp -> data;
    if(tmp == list -> tail)
        list -> tail = list -> head;
    free(tmp);
    return data;
}
/* 删除包含指定数据的节点 */
void deleteData(LList* list, int d){
    Node* curr = list -> head;
    Node* tail = list -> tail;
    while(curr != tail){
        if(curr -> next -> data == d)
            break;
        curr = curr -> next;
    }

    if(curr == list -> tail) return;
    Node* tmp = curr -> next;
    if(tmp == list -> tail)
        list -> tail = curr;
    curr -> next = tmp -> next;
    free(tmp);
}
/* 输出链表所有节点内的数据 */
void print(LList* list){
    Node* curr = list -> head;
    Node* tail = list -> tail;
    printf("< ");
    while(curr != tail){
        printf("%d ", curr -> next -> data);
        curr = curr -> next;
    }
    printf(">\n");
}
/* 行结构定义 */
typedef struct{
    int v;      // 有效位
    int tag;    // 标志
} Line;
/* 行初始化函数 */
void initLine(Line* ln){
    ln -> v = 0;
    ln -> tag = 0;
}
/* 行定位函数 */
int locateLine(Line* ln,int tag){
    if(!ln -> v) return 0;
    if(ln -> tag == tag)
        return 1;
    else
        return 0;
}
/* 加载块函数 */
void loadBlock(Line* ln,int tag){
    ln -> v = 1;    // 有效位置1
    ln -> tag = tag;    // 覆盖标志
}

/* 组结构定义 */
typedef struct{
    int E;      // 行数
    Line* ln;   // 首行指针
    bool isFull;        // 组是否已满
    LList* LRUList;     // LRU表
                        // 表头元素的值总是组内最近最少使用的行的索引号
} Set;
/* 组初始化函数 */
void initSet(Set* st, int E){
    Line* ln = (Line*)malloc(sizeof(Line) * E); // 一组E行
    for(int i = 0;i < E;i++){   // 逐行初始化
        initLine(ln + i);
    }
    // 用链表保存各行的最近访问时序
    LList* list = (LList*)malloc(sizeof(LList));
    initLList(list);
    st -> isFull = false;
    st -> ln = ln;
    st -> E = E;
    st -> LRUList = list;
}
/* 组内空行查找函数 */
int findInvalidLine(Set* st){
    int n = st -> E;
    Line* ln = st -> ln;
    for(int i = 0;i < n;i++)
        if((ln + i) -> v == 0)
            return i;
    return -1;
}
/*  组定位函数
    命中，返回1；
    冷不命中，返回0
    冲突不命中，返回-1；
*/
int locateSet(Set* st,int tag){
    // 命中
    int n = st -> E;
    for(int i = 0;i < n;i++)
        // 是否成功定位在某行
        if(locateLine(st -> ln + i, tag)){
            // 在LRU表中，删除数据元素i
            deleteData(st -> LRUList, i);
            // 将行索引号i插入LRU表的尾部
            insertTail(st -> LRUList, i);
            return 1;
        }

    // 冷不命中
    if(!st -> isFull){
        // 查找第一个无效行的索引
        int idx = findInvalidLine(st);
        // 向该行加载数据块
        loadBlock(st -> ln + idx, tag);
        // 将行索引号idx插入到LRU表的尾部
        insertTail(st -> LRUList, idx);
        if(idx == n - 1)    // 组满，更新isFull
            st -> isFull = true;
        return 0;
    }

    // 冲突不命中
    // 删除LRU表的表头元素，并返回其值给idx
    int idx = deleteFront(st -> LRUList);
    // 向该驱逐行中，加载新的数据块
    loadBlock(st -> ln + idx, tag);
    // 将行索引号idx插入到LRU表的尾部
    insertTail(st -> LRUList, idx);
    return -1;
}

/* Cache结构定义 */
typedef struct{
    int B;  // 块的字节数
    int S;  // 组数
    Set* st;    // 首组指针
} Cache;
/* Cache初始化函数 */
void initCache(Cache* cache,int S,int E,int B){
    Set* st = (Set*)malloc(sizeof(Set) * S);    // S组
    for(int i = 0;i < S;i++)    // 初始化每组
        initSet(st + i, E);

    cache -> st = st;
    cache -> S = S;
    cache -> B = B;
}
/* Cache定位函数 */
int locateCache(Cache* cache,unsigned addr){
    unsigned indexOfBlocks = addr / cache -> B; // 块索引
    int CI = indexOfBlocks % cache -> S;    // 组索引
    int CT = indexOfBlocks / cache -> S;    // 标记
    return locateSet(cache -> st + CI, CT);
}

void show(int msg){
    if(msg == 1)
        printf(" hit");
    else if(msg == 0)
        printf(" miss");
    else
        printf(" miss eviction");
}
/* 选项用法打印函数 */
void printUsage(){
    printf("Usage: ./csim [-hv] -s <num> -E <num> -b <num> -t <file>\n");
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num>   Number of set index bits.\n");
    printf("  -E <num>   Number of lines per set.\n");
    printf("  -b <num>   Number of block offset bits.\n");
    printf("  -t <file>  Trace file.\n");
    printf("\n");
    printf("Examples:\n");
    printf("  linux>  ./csim -s 4 -E 1 -b 4 -t traces/yi.trace\n");
    printf("  linux>  ./csim -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
}
int main(int argc,char *argv[]){
    // freopen("test/test.txt","w",stdout);
    bool isVisible = false; // 是否可追踪每条指令的命中情况
    bool hasOpt = false;    // 是否有选项
    int opt, S, E, B;
    FILE* pFile;

    while(-1 != (opt = getopt(argc, argv, "s:E:b:t:vh"))){
        hasOpt = true;  // 附带选项运行
        switch(opt) {
            case 's':
                S = 1 << atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);   // 字符串转整数函数
                break;
            case 'b':
                B = 1 << atoi(optarg);
                break;
            case 'v':
                isVisible = true;
                break;
            case 't':
                pFile = fopen(optarg,"r");
                break;
            case 'h':
                printUsage();   // 输出选项用法
                return 0;
            default:
                // 无效选项
                printf("Missing required command line argument");
                printUsage();   // 输出选项用法
                return 0;
        }
    }
    if(!hasOpt){ // 无选项情况
        printf("./csim: Missing required command line argument\n");
        printUsage();   // 输出选项用法
        return 0;
    }

    // 创建cache，并初始化
    Cache *c = (Cache*)malloc(sizeof(Cache));
    initCache(c,S,E,B);

    char ident;
    unsigned addr;
    int size;

    int hit_count, miss_count, eviction_count;
    hit_count = miss_count = eviction_count = 0;

    while(fscanf(pFile," %c %x,%d",&ident, &addr, &size) > 0){
        int t;  // locateCache函数调用次数
        if(ident == 'L' || ident == 'S')
            t = 1;
        else if(ident == 'M')
            t = 2;
        else
            t = 0;
        if(t == 0)
            continue;
        if(isVisible)
            printf("%c %x,%d",ident, addr, size);
        while(t--){
            int msg = locateCache(c,addr);
            if(msg == 1)
                hit_count += 1;
            else if(msg == 0)
                miss_count += 1;
            else{
                miss_count += 1;
                eviction_count += 1;
            }
            if(isVisible)
                show(msg);
        }
        if(isVisible)
            printf("\n");
    }
    fclose(pFile);
    printSummary(hit_count, miss_count, eviction_count);
}
