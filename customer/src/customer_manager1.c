#include "customer_manager.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNIT_ARRAY_SIZE 1024

struct UserInfo {
    char *name;   // customer name
    char *id;     // customer id
    int purchase; // purchase amount (> 0)
};

struct DB {
    struct UserInfo *array; // pointer to the array
    int capacity; // current array capacity (max # of elements)
    int size;     // current array size
};

static void ExpandCustomerDB(DB_T db);
static int SearchCustomer(DB_T db, const char *id, const char *name);

/*--------------------------------------------------------------------*/
DB_T CreateCustomerDB(void) {
    DB_T db;

    db = (DB_T)calloc(1, sizeof(struct DB));
    if (db == NULL) {
        fprintf(stderr, "Can't allocate a memory for DB_T\n");
        return NULL;
    }
    db->capacity = UNIT_ARRAY_SIZE; // start with 1024 elements
    db->array = (struct UserInfo *)calloc(db->capacity,
                                          sizeof(struct UserInfo));
    if (db->array == NULL) {
        fprintf(stderr,
                "Can't allocate a memory for array of size %d\n",
                db->capacity);
        free(db);
        return NULL;
    }

    return db;
}

/*--------------------------------------------------------------------*/
void DestroyCustomerDB(DB_T db) {
    for (size_t i = 0; i < db->size; i++) {
        free(db->array[i].name);
        free(db->array[i].id);
    }

    free(db->array);
    free(db);
}

/*--------------------------------------------------------------------*/
int RegisterCustomer(DB_T db, const char *id, const char *name,
                     const int purchase) {
    if (db == NULL || id == NULL || name == NULL || purchase <= 0)
        return -1;

    // check for already existing item
    if (SearchCustomer(db, id, name) != -1)
        return -1;

    struct UserInfo *newUser = db->array + db->size;

    newUser->id = strdup(id);
    if (newUser->id == NULL) {
        fprintf(stderr, "Can't allocate memory for new user id\n");
        return -1;
    }

    newUser->name = strdup(name);
    if (newUser->name == NULL) {
        free(newUser->id);
        fprintf(stderr, "Can't allocate memory for new user name\n");
        return -1;
    }

    newUser->purchase = purchase;
    db->size++;
    ExpandCustomerDB(db); // expand customer DB if necessary

    return 0;
}

/*--------------------------------------------------------------------*/
int UnregisterCustomerByID(DB_T db, const char *id) {
    if (db == NULL || id == NULL)
        return -1;

    int idx = SearchCustomer(db, id, NULL);
    if (idx == -1)
        return -1;

    free(db->array[idx].id);
    free(db->array[idx].name);

    for (int i = idx + 1; i < db->size; i++)
        db->array[i - 1] = db->array[i];
    db->size--;

    return 0;
}

/*--------------------------------------------------------------------*/
int UnregisterCustomerByName(DB_T db, const char *name) {
    if (db == NULL || name == NULL)
        return -1;

    int idx = SearchCustomer(db, NULL, name);
    if (idx == -1)
        return -1;

    free(db->array[idx].id);
    free(db->array[idx].name);

    for (int i = idx + 1; i < db->size; i++)
        db->array[i - 1] = db->array[i];
    db->size--;

    return 0;
}

/*--------------------------------------------------------------------*/
int GetPurchaseByID(DB_T db, const char *id) {
    if (db == NULL || id == NULL)
        return -1;

    int idx = SearchCustomer(db, id, NULL);
    if (idx == -1)
        return -1;

    return db->array[idx].purchase;
}

/*--------------------------------------------------------------------*/
int GetPurchaseByName(DB_T db, const char *name) {
    if (db == NULL || name == NULL)
        return -1;

    int idx = SearchCustomer(db, NULL, name);
    if (idx == -1)
        return -1;

    return db->array[idx].purchase;
}

/*--------------------------------------------------------------------*/
int GetSumCustomerPurchase(DB_T db, FUNCPTR_T fp) {
    if (db == NULL || fp == NULL)
        return -1;

    int sum = 0;

    for (int i = 0; i < db->size; i++)
        sum += fp(db->array[i].id, db->array[i].name,
                  db->array[i].purchase);

    return sum;
}

static void ExpandCustomerDB(DB_T db) {
    if (db->capacity == db->size) {
        db->capacity *= 2;
        db->array =
            realloc(db->array, db->capacity * sizeof(struct UserInfo));
    }
}

/**
 * return the index of the array if customer with same id or name exists
 * return -1 otherwise
 */
static int SearchCustomer(DB_T db, const char *id, const char *name) {
    for (int i = 0; i < db->size; i++) {
        if ((id && !strcmp(db->array[i].id, id)) ||
            (name && !strcmp(db->array[i].name, name)))
            return i;
    }

    return -1;
}

