#include "customer_manager.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNIT_BUCKET_SIZE    1024
#define THRESHOLD_RATIO     0.75f

enum { HASH_MULTIPLIER = 65599 };

struct UserInfo {
    char *name;   // customer name
    char *id;     // customer id
    int purchase; // purchase amount (> 0)

    unsigned int idHash;
    unsigned int nameHash;

    struct UserInfo *idNext;
    struct UserInfo *nameNext;
};

struct DB {
    struct UserInfo **idTable;
    struct UserInfo **nameTable;
    unsigned int capacity; // current bucket size (max # of elements)
    unsigned int size; // current number of elements in the hash table
    unsigned int threshold;
};

static unsigned int hashfunc_raw(const char *key);
static int hashfunc(const char *key, unsigned int bucketSize);
static struct UserInfo *SearchCustomerById(DB_T db, const char *id);
static struct UserInfo *SearchCustomerByName(DB_T db, const char *name);
static struct UserInfo *UnlinkCustomerById(DB_T db, const char *id);
static struct UserInfo *UnlinkCustomerByName(DB_T db, const char *name);
static void rehash(DB_T db);

/*--------------------------------------------------------------------*/
DB_T CreateCustomerDB(void) {
    DB_T db;

    db = (DB_T)calloc(1, sizeof(struct DB));
    if (db == NULL) {
        fprintf(stderr, "Can't allocate a memory for DB_T\n");
        return NULL;
    }
    db->capacity = UNIT_BUCKET_SIZE; // start with 1024 elements
    db->threshold = (int)(THRESHOLD_RATIO *
                          UNIT_BUCKET_SIZE); // start with 1024 elements
    db->size = 0;
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

    struct UserInfo *idsearch = SearchCustomerById(db, id);
    struct UserInfo *namesearch = SearchCustomerByName(db, name);

    if (idsearch != NULL || namesearch != NULL) {
        return -1;
    }

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

    unsigned int idHash = hashfunc_raw(id);
    int idHashModulo = (int)(idHash & (db->capacity - 1));
    newUser->idNext = db->idTable[idHashModulo];
    newUser->idHash = idHash;
    db->idTable[idHashModulo] = newUser;

    unsigned int nameHash = hashfunc_raw(name);
    int nameHashModulo = (int)(nameHash & (db->capacity - 1));
    newUser->nameNext = db->nameTable[nameHashModulo];
    newUser->nameHash = nameHash;
    db->nameTable[nameHashModulo] = newUser;

    db->size++;
    rehash(db);

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

    db->size--;

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

    db->size--;

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

static unsigned int hashfunc_raw(const char *key) {
    unsigned int hash = 0U;
    for (int i = 0; key[i] != '\0'; i++)
        hash =
            hash * (unsigned int)HASH_MULTIPLIER + (unsigned int)key[i];
    return hash;
}

static inline int hashfunc(const char *key, unsigned int bucketSize) {
    return (int)(hashfunc_raw(key) & (bucketSize - 1));
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
    for (p = db->idTable[hash]; p != NULL; before = p, p = p->idNext) {
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

static struct UserInfo *UnlinkCustomerByName(DB_T db,
                                             const char *name) {
    struct UserInfo *p, *before = NULL;
    int hash = hashfunc(name, db->capacity);
    for (p = db->nameTable[hash]; p != NULL;
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

static void rehash(DB_T db) {
    // if size is not over threshold, simply return
    if (db->size < db->threshold)
        return;

    struct UserInfo *p, *nextp;
    // increase capacity. bucket size is now twice as large
    unsigned int newCapacity = db->capacity << 1;

    struct UserInfo **newIdTable =
        calloc(newCapacity, sizeof(struct UserInfo *));
    if (newIdTable == NULL) {
        assert(1);
    }
    struct UserInfo **newNameTable =
        calloc(newCapacity, sizeof(struct UserInfo *));
    if (newNameTable == NULL) {
        free(newIdTable);
        assert(1);
    }

    // iterating one table addresses all registered customers.
    // here, we iterate over idTable
    for (unsigned int i = 0; i < db->capacity; i++) {
        for (p = db->idTable[i]; p != NULL; p = nextp) {
            nextp = p->idNext;

            int newIdHash = (int)(p->idHash & (newCapacity - 1));
            int newNameHash = (int)(p->nameHash & (newCapacity - 1));

            p->idNext = newIdTable[newIdHash];
            newIdTable[newIdHash] = p;

            p->nameNext = newNameTable[newNameHash];
            newNameTable[newNameHash] = p;
        }
    }

    // we are done migrating.
    // update the db to contain new members
    db->capacity = newCapacity;
    db->threshold = (int)(db->capacity * THRESHOLD_RATIO);
    free(db->idTable);
    free(db->nameTable);
    db->idTable = newIdTable;
    db->nameTable = newNameTable;
}
