#include<stdio.h>

struct product{
    int prod_id;
    char pname[100];
    int cost;
    int qty;
};

struct cart{
    int cart_id;
    int cust_id;
    int uniq_count = 0;
    struct product items[50];
};