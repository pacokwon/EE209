/**
 * Author: Haechan Kwon (권해찬)
 * Assignment: Customer Management (Assignment 3)
 * Filename: customer_manager1.c
 */

#include "customer_manager.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNIT_ARRAY_SIZE 1024

struct UserInfo {
    // customer name
    char *name;

    // customer id
    char *id;

    // purchase amount (> 0)
    int purchase;
};

struct DB {
    // array pointer
    struct UserInfo *array;

    // current array capacity (max # of elements)
    int capacity;

    // current array size
    int size;
};

static void ExpandCustomerDB(DB_T db);
static int SearchCustomer(DB_T db, const char *id, const char *name);

/**
 * CreateCustomerDB: create a new customer db
 *
 * this function allocates resources necessary for storing customer
 * information, e.g. array for storing customer information
 *
 * returns: pointer to newly allocated database
 */
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

/**
 * DestroyCustomerDB: destroy a customer db
 *
 * this function frees all dynamically allocated resources in the
 * database
 *
 * param db: pointer to database
 */
void DestroyCustomerDB(DB_T db) {
    for (size_t i = 0; i < db->size; i++) {
        free(db->array[i].name);
        free(db->array[i].id);
    }

    free(db->array);
    free(db);
}

/**
 * RegisterCustomer: register a new customer
 *
 * param db: pointer to database
 * param id: pointer to null terminated string that contains customer's
 * id param name: pointer to null terminated string that contains
 * customer's name param purchase: purchase value of customer
 *
 * returns: 0 if customer is successfully registered. -1 otherwise
 */
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

/**
 * UnregisterCustomerByID: unregister a customer by id
 *
 * remove AND free a customer entry with a given id
 *
 * param db: pointer to database
 * param id: pointer to null terminated string that contains id
 *
 * returns: 0 if customer is successfully removed. -1 otherwise
 */
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

/**
 * UnregisterCustomerByName: unregister a customer by name
 *
 * remove AND free a customer entry with a given name
 *
 * param db: pointer to database
 * param name: pointer to null terminated string that contains name
 *
 * returns: 0 if customer is successfully removed. -1 otherwise
 */
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

/**
 * GetPurchaseByID: get the purchase field of a customer by id
 *
 * param db: pointer to database
 * param id: pointer to null terminated string that contains id
 *
 * returns: purchase field value of customer with id.
 *  -1 if customer with id does not exist
 */
int GetPurchaseByID(DB_T db, const char *id) {
    if (db == NULL || id == NULL)
        return -1;

    int idx = SearchCustomer(db, id, NULL);
    if (idx == -1)
        return -1;

    return db->array[idx].purchase;
}

/**
 * GetPurchaseByName: get the purchase field of a customer by name
 *
 * param db: pointer to database
 * param name: pointer to null terminated string that contains name
 *
 * returns: purchase field value of customer with name
 *  -1 if customer with name does not exist
 */
int GetPurchaseByName(DB_T db, const char *name) {
    if (db == NULL || name == NULL)
        return -1;

    int idx = SearchCustomer(db, NULL, name);
    if (idx == -1)
        return -1;

    return db->array[idx].purchase;
}

/**
 * GetSumCustomerPurchase: apply a given function to all customers and
 * get the sum of results
 *
 * param db: pointer to database
 * param fp: pointer to a function of type FUNCPTR_T
 *
 * returns: sum of function applications to all customers
 */
int GetSumCustomerPurchase(DB_T db, FUNCPTR_T fp) {
    if (db == NULL || fp == NULL)
        return -1;

    int sum = 0;

    for (int i = 0; i < db->size; i++)
        sum += fp(db->array[i].id, db->array[i].name,
                  db->array[i].purchase);

    return sum;
}

/**
 * ExpandCustomerDB: resize customer db array, but only if necessary
 *
 * this function resizes the database array by twice if current capacity
 * is full.
 *
 * param db: pointer to database
 */
static void ExpandCustomerDB(DB_T db) {
    if (db->capacity == db->size) {
        db->capacity <<= 1;
        db->array =
            realloc(db->array, db->capacity * sizeof(struct UserInfo));
    }
}

/**
 * SearchCustomer: search for a customer with a given id or name
 *
 * returns: the index of the array if customer with same id or name exists
 *  -1 otherwise
 */
static int SearchCustomer(DB_T db, const char *id, const char *name) {
    for (int i = 0; i < db->size; i++) {
        if ((id && !strcmp(db->array[i].id, id)) ||
            (name && !strcmp(db->array[i].name, name)))
            return i;
    }

    return -1;
}
