#ifndef __mapreduce_h__
#define __mapreduce_h__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

typedef struct KeyValuePair {
    char *key;
    char *value;
    struct KeyValuePair *next;
} KeyValuePair;

KeyValuePair *KV = NULL;
pthread_mutex_t kv_mutex = PTHREAD_MUTEX_INITIALIZER;

// reduce data structures
typedef struct ValueNode {
    char *value;
    struct ValueNode *next;
} ValueNode;

typedef struct KeyNode {
    char *key;
    ValueNode *values; // linked list of values
    int value_count;
    int next_idx;          // idx used by getter
    pthread_mutex_t lock;  // per-key
    struct KeyNode *next;
} KeyNode;

KeyNode **partitions = NULL;
int g_num_partitions = 0;

/* function pointer types */
typedef char *(*Getter)(char *key, int partition_number);
typedef void (*Mapper)(char *file_name);
typedef void (*Reducer)(char *key, Getter get_func, int partition_number);
typedef unsigned long (*Partitioner)(char *key, int num_partitions);

static void free_partitions(int num_partitions);

void MR_Emit(char *key, char *value)
{
    KeyValuePair *n = malloc(sizeof(KeyValuePair));
    if (!n) { perror("malloc"); exit(1); }
    n->key = strdup(key);
    n->value = strdup(value);
    n->next = NULL;

    pthread_mutex_lock(&kv_mutex);
    n->next = KV;
    KV = n;
    pthread_mutex_unlock(&kv_mutex);
}

unsigned long MR_DefaultHashPartition(char *key, int num_partitions)
{
    unsigned long hash = 5381;
    int c;
    while ((c = *key++) != 0)
        hash = ((hash << 5) + hash) + c;
    return hash % (unsigned long)num_partitions;
}

static KeyNode *create_keynode(const char *key, const char *value)
{
    KeyNode *kn = malloc(sizeof(KeyNode));
    if (!kn) { perror("malloc"); exit(1); }
    kn->key = strdup(key);
    kn->values = NULL;
    kn->value_count = 0;
    kn->next_idx = 0;
    pthread_mutex_init(&kn->lock, NULL);
    kn->next = NULL;

    ValueNode *vn = malloc(sizeof(ValueNode));
    if (!vn) { perror("malloc"); exit(1); }
    vn->value = strdup(value);
    vn->next = NULL;
    kn->values = vn;
    kn->value_count = 1;
    return kn;
}

void R_Emit(char *key, char *value, int partition_number)
{
    if (partition_number < 0 || partition_number >= g_num_partitions) return;

    KeyNode *cur = partitions[partition_number];
    KeyNode *prev = NULL;
    while (cur && strcmp(cur->key, key) != 0) {
        prev = cur;
        cur = cur->next;
    }

    if (!cur) { // new key
        KeyNode *kn = create_keynode(key, value);
        if (prev == NULL) {
            kn->next = partitions[partition_number];
            partitions[partition_number] = kn;
        } else {
            kn->next = prev->next;
            prev->next = kn;
        }
        return;
    }

	// existing key
    pthread_mutex_lock(&cur->lock);
    ValueNode *vn = malloc(sizeof(ValueNode));
    if (!vn) { perror("malloc"); exit(1); }
    vn->value = strdup(value);
    vn->next = NULL;

    if (cur->values == NULL) cur->values = vn;
    else {
        ValueNode *t = cur->values;
        while (t->next) t = t->next;
        t->next = vn;
    }
    cur->value_count++;
    pthread_mutex_unlock(&cur->lock);
}

char *MR_GetNext(char *key, int partition_number)
{
    if (!partitions || partition_number < 0 || partition_number >= g_num_partitions) return NULL;

    KeyNode *cur = partitions[partition_number];
    while (cur) {
        if (strcmp(cur->key, key) == 0) {
            pthread_mutex_lock(&cur->lock);
            if (cur->next_idx >= cur->value_count) {
                pthread_mutex_unlock(&cur->lock);
                return NULL;
            }
            ValueNode *vn = cur->values;
            int idx = cur->next_idx;
            for (int i = 0; i < idx && vn; ++i) vn = vn->next;
            cur->next_idx++;
            char *ret = vn ? vn->value : NULL;
            pthread_mutex_unlock(&cur->lock);
            return ret;
        }
        cur = cur->next;
    }
    return NULL;
}

void buildPartitions(int num_partitions, Partitioner partition)
{
    g_num_partitions = num_partitions;
    partitions = calloc(num_partitions, sizeof(KeyNode *));
    if (!partitions) { perror("calloc"); exit(1); }

    pthread_mutex_lock(&kv_mutex);
    KeyValuePair *cur = KV;
    KV = NULL; // clear global KV
    pthread_mutex_unlock(&kv_mutex);

    while (cur) {
        KeyValuePair *next = cur->next;
        unsigned long p = partition(cur->key, num_partitions);
        if ((int)p < 0) { p += num_partitions; p %= num_partitions; }

        R_Emit(cur->key, cur->value, (int)p);

        free(cur->key);
        free(cur->value);
        free(cur);
        cur = next;
    }
}

void sortPartitions()
{
    for (int i = 0; i < g_num_partitions; ++i)
    {
        KeyNode *kn = partitions[i];
        KeyNode *sorted = NULL;
        while (kn) {
            KeyNode *next = kn->next;
            kn->next = NULL;
            if (!sorted || strcmp(kn->key, sorted->key) < 0) {
                kn->next = sorted;
                sorted = kn;
            } else {
                KeyNode *cur = sorted;
                while (cur->next && strcmp(cur->next->key, kn->key) <= 0) cur = cur->next;
                kn->next = cur->next;
                cur->next = kn;
            }
            kn = next;
        }
        partitions[i] = sorted;
    }
}

static void free_partitions(int num_partitions)
{
    if (!partitions) return;
    for (int p = 0; p < num_partitions; ++p) {
        KeyNode *kn = partitions[p];
        while (kn) {
            KeyNode *knt = kn->next;
            pthread_mutex_destroy(&kn->lock);
            free(kn->key);
            ValueNode *v = kn->values;
            while (v) {
                ValueNode *vt = v->next;
                free(v->value);
                free(v);
                v = vt;
            }
            free(kn);
            kn = knt;
        }
    }
    free(partitions);
    partitions = NULL;
    g_num_partitions = 0;
}

static void *mapper_wrapper(void *arg)
{
    char *fname = (char *)arg;
    extern void Map(char *file_name);
    Map(fname);
    return NULL;
}

static void *reducer_wrapper(void *arg)
{
    int partition_number = (int)(long)arg;
    extern void Reduce(char *key, Getter get_next, int partition_number);

    KeyNode *kn = partitions[partition_number];
    while (kn) {
        pthread_mutex_lock(&kn->lock);
        kn->next_idx = 0;
        pthread_mutex_unlock(&kn->lock);

        Reduce(kn->key, MR_GetNext, partition_number);
        kn = kn->next;
    }
    return NULL;
}

void MR_Run(int argc, char *argv[],
            Mapper map, int num_mappers,
            Reducer reduce, int num_reducers,
            Partitioner partition)
{
    if (!partition) partition = MR_DefaultHashPartition;

    g_num_partitions = 0;
    partitions = NULL;

    int file_count = (argc > 1) ? (argc - 1) : 0;
    if (file_count == 0) {
        fprintf(stderr, "MR_Run: no input files\n");
        return;
    }

    int mthreads = file_count;
    pthread_t *mtids = malloc(sizeof(pthread_t) * mthreads);
    if (!mtids) { perror("malloc"); exit(1); }
    for (int i = 0; i < mthreads; ++i) {
        char *filename = argv[i + 1];
        int rc = pthread_create(&mtids[i], NULL, mapper_wrapper, (void *)filename);
        if (rc) { fprintf(stderr, "pthread_create(mapper) failed\n"); exit(1); }
    }
    for (int i = 0; i < mthreads; ++i) pthread_join(mtids[i], NULL);
    free(mtids);

    buildPartitions(num_reducers, partition);

    sortPartitions();

    pthread_t *rtids = calloc(num_reducers, sizeof(pthread_t));
    if (!rtids) { perror("calloc"); exit(1); }
    for (int i = 0; i < num_reducers; ++i) {
        int rc = pthread_create(&rtids[i], NULL, reducer_wrapper, (void *)(long)i);
        if (rc) { fprintf(stderr, "pthread_create(reducer) failed\n"); continue; }
    }
    for (int i = 0; i < num_reducers; ++i) pthread_join(rtids[i], NULL);
    free(rtids);

    free_partitions(num_reducers);
}

#endif 
