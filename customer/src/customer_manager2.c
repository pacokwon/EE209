#include "customer_manager.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNIT_BUCKET_SIZE 1024

enum { HASH_MULTIPLIER = 65599 };

struct UserInfo {
    char *name;   // customer name
    char *id;     // customer id
    int purchase; // purchase amount (> 0)

    struct UserInfo *idNext;
    struct UserInfo *nameNext;
};

struct DB {
    struct UserInfo **idTable;
    struct UserInfo **nameTable;
    int capacity; // current bucket size (max # of elements)
};

static int hashfunc(const char *key, int bucketSize);
static struct UserInfo *SearchCustomerById(DB_T db, const char *id);
static struct UserInfo *SearchCustomerByName(DB_T db, const char *name);
static struct UserInfo *UnlinkCustomerById(DB_T db, const char *id);
static struct UserInfo *UnlinkCustomerByName(DB_T db, const char *name);

/*--------------------------------------------------------------------*/
DB_T CreateCustomerDB(void) {
    DB_T db;

    db = (DB_T)calloc(1, sizeof(struct DB));
    if (db == NULL) {
        fprintf(stderr, "Can't allocate a memory for DB_T\n");
        return NULL;
    }
    db->capacity = UNIT_BUCKET_SIZE; // start with 1024 elements
    db->idTable = calloc(db->capacity, sizeof(struct UserInfo *));
    if (db->idTable == NULL) {
        fprintf(stderr,
                "Can't allocate a memory for array of size %d\n",
                db->capacity);
        free(db);
        return NULL;
    }

    db->nameTable = calloc(db->capacity, sizeof(struct UserInfo *));
    if (db->nameTable == NULL) {
        fprintf(stderr,
                "Can't allocate a memory for array of size %d\n",
                db->capacity);
        free(db->idTable);
        free(db);
        return NULL;
    }

    return db;
}

/*--------------------------------------------------------------------*/
void DestroyCustomerDB(DB_T db) {
    struct UserInfo *p, *nextp;

    for (size_t i = 0; i < db->capacity; i++)
        for (p = db->idTable[i]; p != NULL; p = nextp) {
            nextp = p->idNext;
            free(p->id);
            free(p->name);
            free(p);
        }

    free(db->idTable);
    free(db->nameTable);
    free(db);
}

/*--------------------------------------------------------------------*/
int RegisterCustomer(DB_T db, const char *id, const char *name,
                     const int purchase) {
    if (db == NULL || id == NULL || name == NULL || purchase <= 0)
        return -1;

    if (SearchCustomerById(db, id) != NULL ||
        SearchCustomerByName(db, name) != NULL)
        return -1;

    struct UserInfo *newUser = calloc(1, sizeof(struct UserInfo));
    if (newUser == NULL) {
        fprintf(stderr, "Can't allocate memory for new user\n");
        return -1;
    }

    newUser->id = strdup(id);
    if (newUser->id == NULL) {
        free(newUser);
        fprintf(stderr, "Can't allocate memory for new user id\n");
        return -1;
    }

    newUser->name = strdup(name);
    if (newUser->name == NULL) {
        free(newUser->id);
        free(newUser);
        fprintf(stderr, "Can't allocate memory for new user name\n");
        return -1;
    }

    newUser->purchase = purchase;

    int idHash = hashfunc(id, db->capacity);
    newUser->idNext = db->idTable[idHash];
    db->idTable[idHash] = newUser;

    int nameHash = hashfunc(name, db->capacity);
    newUser->nameNext = db->nameTable[nameHash];
    db->nameTable[nameHash] = newUser;

    return 0;
}

/*--------------------------------------------------------------------*/
int UnregisterCustomerByID(DB_T db, const char *id) {
    if (db == NULL || id == NULL)
        return -1;

    struct UserInfo *p = UnlinkCustomerById(db, id);
    if (p == NULL)
        return -1;

    p = UnlinkCustomerByName(db, p->name);
    assert(p != NULL);

    free(p->name);
    free(p->id);
    free(p);

    return 0;
}

/*--------------------------------------------------------------------*/
int UnregisterCustomerByName(DB_T db, const char *name) {
    if (db == NULL || name == NULL)
        return -1;

    struct UserInfo *p = UnlinkCustomerByName(db, name);
    if (p == NULL)
        return -1;

    p = UnlinkCustomerById(db, p->id);
    assert(p != NULL);

    free(p->name);
    free(p->id);
    free(p);

    return 0;
}

/*--------------------------------------------------------------------*/
int GetPurchaseByID(DB_T db, const char *id) {
    if (db == NULL || id == NULL)
        return -1;

    struct UserInfo *p = SearchCustomerById(db, id);
    if (p == NULL)
        return -1;

    return p->purchase;
}

/*--------------------------------------------------------------------*/
int GetPurchaseByName(DB_T db, const char *name) {
    if (db == NULL || name == NULL)
        return -1;

    struct UserInfo *p = SearchCustomerByName(db, name);
    if (p == NULL)
        return -1;

    return p->purchase;
}

/*--------------------------------------------------------------------*/
int GetSumCustomerPurchase(DB_T db, FUNCPTR_T fp) {
    if (db == NULL || fp == NULL)
        return -1;

    int sum = 0;

    for (int i = 0; i < db->capacity; i++)
        for (struct UserInfo *p = db->idTable[i]; p != NULL;
             p = p->idNext)
            sum += fp(p->id, p->name, p->purchase);

    return sum;
}

static int hashfunc(const char *key, int bucketSize) {
    unsigned int hash = 0U;
    for (int i = 0; key[i] != '\0'; i++)
        hash =
            hash * (unsigned int)HASH_MULTIPLIER + (unsigned int)key[i];
    return (int)(hash % (unsigned int)bucketSize);
}

static struct UserInfo *SearchCustomerById(DB_T db, const char *id) {
    if (id == NULL)
        return NULL;

    int hash = hashfunc(id, db->capacity);
    for (struct UserInfo *p = db->idTable[hash]; p != NULL;
         p = p->idNext)
        if (strcmp(p->id, id) == 0)
            return p;

    return NULL;
}

static struct UserInfo *SearchCustomerByName(DB_T db,
                                             const char *name) {
    if (name == NULL)
        return NULL;

    int hash = hashfunc(name, db->capacity);
    for (struct UserInfo *p = db->nameTable[hash]; p != NULL;
         p = p->nameNext)
        if (strcmp(p->name, name) == 0)
            return p;

    return NULL;
}

static struct UserInfo *UnlinkCustomerById(DB_T db, const char *id) {
    struct UserInfo *p, *before = NULL;

    int hash = hashfunc(id, db->capacity);
    for (struct UserInfo *p = db->idTable[hash]; p != NULL;
         before = p, p = p->idNext) {

        if (strcmp(p->id, id) != 0)
            continue;

        if (before == NULL)
            db->idTable[hash] = db->idTable[hash]->idNext;
        else
            before->idNext = p->idNext;

        return p;
    }

    return NULL;
}

static struct UserInfo *UnlinkCustomerByName(DB_T db, const char *name) {
    struct UserInfo *p, *before = NULL;
    int hash = hashfunc(name, db->capacity);
    for (struct UserInfo *p = db->nameTable[hash]; p != NULL;
         before = p, p = p->nameNext) {

        if (strcmp(p->name, name) != 0)
            continue;

        if (before == NULL)
            db->nameTable[hash] = db->nameTable[hash]->nameNext;
        else
            before->nameNext = p->nameNext;

        return p;
    }

    return NULL;
}
