#include<stdio.h>
#define MAX_CART 50

struct index{
    int key;
    int offset;
};

struct product{
    int prod_id;
    char pname[100];
    int cost;
    int qty;
};

struct cart{
    int cust_id;
    struct product items[MAX_CART];
};
