#include "cachelab.h"
#include "malloc.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct Node{
    int data;
    struct Node* next;
}Node;
Node*  initNode(int n){
    Node* tmp = (Node* ) malloc(sizeof(Node));
    tmp->data=n;
    tmp->next=0;
    return tmp;
}
typedef struct LList{
    Node* head;
    Node*   tail;
}LList;
void initLList(LList* ls){
    ls->tail=initNode(-1);
    ls->head=initNode(-1);
    ls->head->next=ls->tail;
}

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
int deleteFront(LList* ls){
    //printf(">\n deleteFront 1");
    //print(ls);
    if(ls->head->next == ls->tail) return -1;
    int res = ls->head->next->data;
    Node* tmp = ls->head;
    ls->head = tmp->next;
    ls->head->data=-1;
    free(tmp);
   // printf(">\n deleteFront 2");
    //print(ls);
    return res;
}
void addBack(LList* ls, int a){
    //printf(">\n addBack 1");
   // print(ls);
    ls->tail->data = a;
    Node* tmp =initNode(-1);
    ls->tail->next = tmp;
    ls ->tail =tmp;
    //printf(">\n addBack 2");
    //print(ls);
}
void deleteIn(LList* ls,int b){
    //printf(">\n deleteIn 1");
    //print(ls);
    Node* tmp = ls->head;
    Node* tail = ls->tail;
   while(tmp->next!=tail && tmp->next->data!=b){
        tmp = tmp->next;
   }
   if(tmp->next == tail ) return;//never found
   if(tmp->next->data == b)   {
        if(tmp->next->next == tail) {
        ls->tail = tmp->next;//miss
        ls->tail->data=-1;
        }//find at the last one
        else{
            Node* n0 = tmp->next;
            tmp->next = n0->next;
            free(n0);
        }
   }

    //printf(">\n deleteIn 2");
    //print(ls);
   return;
}

typedef struct Line{
    int valid;
    int tag;
}Line;
void initLine(Line* ln){
    ln->valid = ln->tag = 0;
}
int locateLine(Line* ln,int tag)
{
    if(ln->valid &&(ln->tag == tag) ) return 1;
    else return 0;
}
void loadLine(Line*ln,int tag){
    ln->valid=1;
    ln->tag=tag;
}

typedef struct Set{
    int E;
    LList* LRUList;
    Line* ln;
    int isFull;
}Set;
void initSet(Set* st, int E){
    Line* ln = (Line*) malloc(sizeof(Line)*E);
    for(int i=0;i<E;++i){
    initLine(ln+i);
    }
    LList* ls = (LList*)malloc(sizeof(LList));
    initLList(ls);
    st->LRUList=ls;         st->ln = ln;
    st->isFull = 0;             st->E = E;
}
int findEmptyLine(Set* st){
    int n = st -> E;
    Line* ln = st -> ln;
    for(int i = 0;i < n;i++)
        if((ln + i) -> valid == 0)
            return i;
    return -1;
}
int locateSet(Set* st,int tag){
    int n = st->E;
    for(int i=0;i<n;++i){
        if(locateLine(st->ln+i,tag)){
            deleteIn(st->LRUList,i);
            addBack(st->LRUList,i);
            return 1;
        }
    }
    if(!st->isFull){
        int loc = findEmptyLine(st);
        loadLine(st->ln+loc,tag);
        addBack(st->LRUList,loc);
        if(loc == n-1) st->isFull=1;//miss
        return 0;
    }
    else{
        int loc=deleteFront(st->LRUList);
        loadLine(st->ln+loc,tag);
        addBack(st->LRUList,loc);
        return -1;
    }
}
typedef struct Cache{
    int S;
    int B;
    int E;
    Set* st;
}Cache;
void initCache(Cache* ch,int s,int b,int e){
    Set* st=(Set*)malloc(sizeof(Set)*s);
    for(int i=0;i<s;++i){
        initSet(st+i,e);
    }
    ch->st = st;     ch->S=s;
    ch->B=b;        ch->E=e;
}
int locateCache(Cache* ch,unsigned addr){
    unsigned tmp = addr / (ch->B);
     int set = tmp %(ch->S);
     int tag = tmp /(ch->S);
     return locateSet(ch->st+set,tag);
}
void showMess(int msg){
    if(msg == 1)    printf(" hit");
    else{
        if(msg == 0)        printf(" miss");
        else    printf(" miss eviction");
    }
}
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
    int isVisible = 0;
    int hasOpt = 0;
    int opt, S, E, B;
    FILE* pFile;

    while(-1 != (opt = getopt(argc, argv, "s:E:b:t:vh"))){
        hasOpt = 1;
        switch(opt) {
            case 's':
                S = 1 << atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'b':
                B = 1 << atoi(optarg);
                break;
            case 'v':
                isVisible = 1;
                break;
            case 't':
                pFile = fopen(optarg,"r");
                break;
            case 'h':
                printUsage();
                return 0;
            default:
                printf("Missing required command line argument");
                printUsage();
                return 0;
        }
    }
    if(!hasOpt){
        printf("./csim: Missing required command line argument\n");
        printUsage();
        return 0;
    }
Cache *c = (Cache*)malloc(sizeof(Cache));
    initCache(c,S,B,E);
    char ident;
    unsigned addr;
    int size;

    int hit_count, miss_count, eviction_count;
    hit_count = miss_count = eviction_count = 0;

    while(fscanf(pFile," %c %x,%d",&ident, &addr, &size) > 0){
        int t;
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
                showMess(msg);
        }
        if(isVisible)
            printf("\n");
    }
    fclose(pFile);
    printSummary(hit_count, miss_count, eviction_count);
}






























