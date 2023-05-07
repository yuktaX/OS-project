#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "product_cart.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#define PORT 5555

void displayproduct(struct product p)
{
    if(p.prod_id != -1 && p.qty > 0)
        printf("%d\t%s\t%d\t%d\n", p.prod_id, p.pname, p.qty, p.cost);
}

void displayStore(int socketfd)
{
    printf("Connecting to store...\n");
    printf("ID\tName\tStock\tCost\n");
    while(1)
    {
        struct product p;
        read(socketfd, &p, sizeof(struct product));
        //printf("hello prod %d %d\n", p.prod_id, p.qty);
        if(p.prod_id != -1)
            displayproduct(p);
        else
            break;
    }
}

int getTotal(struct cart c)
{
    int total = 0;
    for(int i = 0; i < MAX_CART; i++)
    {
        if(c.items[i].prod_id != -1)
        {
            total += (c.items[i].cost * c.items[i].qty);
        }
    }
    return total;
}

void generateReceipt(struct cart c, int total)
{
    int fd_rec = open("receipt.txt", O_CREAT | O_RDWR, 0777);
    write(fd_rec, "ID\tName\tQuantity\tPrice\n", strlen("ID\tName\tQuantity\tPrice\n"));
    char tmp[200];
    for (int i=0; i < MAX_CART; i++)
    {
        if (c.items[i].prod_id != -1)
        {
            sprintf(tmp, "%d\t%s\t%d\t%d\n", c.items[i].prod_id, c.items[i].pname, c.items[i].qty, c.items[i].cost);
            write(fd_rec, tmp, strlen(tmp));
        }
    }
    sprintf(tmp, "Total - %d\n", total);
    write(fd_rec, tmp, strlen(tmp));
    close(fd_rec);
}

int getCustomerID()
{
    int id = -1;
    while(1)
    {
        printf("Enter customer id: \n");
        scanf("%d", &id);
        if(id < 0)
            printf("Please enter a postive number\n");
        else    
            break;
    }
    return id;
}

int setProductID()
{
    int id = 0;
    while(1)
    {
        printf("Enter product id: \n");
        scanf("%d", &id);
        if(id <= 0)
            printf("Please enter a postive number\n");
        else    
            break;
    }
    return id;
}

char * setProductName()
{
    char name[100];
    printf("Enter product name: \n");
    scanf("%s", name);
    return name;
}

int setProductCost()
{
    int id = 0;
    while(1)
    {
        printf("Enter product price: \n");
        scanf("%d", &id);

        if(id <= 0)
            printf("Please enter a postive number, price cannot be negative\n");
        else    
            break;
    }
    return id;
}

int setProductQty()
{
    int id = -1;
    while(1)
    {
        printf("Enter product quantity: \n");
        scanf("%d", &id);

        if(id < 0)
            printf("Please enter a postive number, quantity cannot be negative\n");
        else    
            break;
    }
    return id;
}

void displayMycart(struct cart c)
{
    if(c.cust_id != -1)
    {
        printf("Customer ID %d\n", c.cust_id);
        printf("ProductID\tProductName\tQuantity Available\tPrice\n");
        for (int i = 0; i < MAX_CART; i++)
        {
            displayproduct(c.items[i]);
        }
    }
    else
        printf("Wrong customer id provided\n");
}


int main()
{
    printf("Connecting to server....\n");

    //int socketfd, new_sd, connectfd;

    int socketfd = socket(AF_INET, SOCK_STREAM, 0);//creating socket filed
    if(socketfd == -1)
    {
        perror("socket error: ");
        return -1;
    }

    struct sockaddr_in client;

    client.sin_family = AF_INET;
    client.sin_addr.s_addr = INADDR_ANY;
    client.sin_port = htons(PORT);

    //connectfd = connect(socketfd, (struct sockaddr *)&client, sizeof(client)); //connecting to server
    //printf("%d", connectfd);
    if(connect(socketfd, (struct sockaddr *)&client, sizeof(client)) == -1) //connecting to server
    {
        perror("connect failed");
        return -1;
    }
    
    int login;
    printf("******************Online Retail Store********************\n");
    printf("Who are you? \n1)Customer \n2)Admin\n");
    scanf("%d", &login);
    printf("login is %d", login);
    write(socketfd, &login, sizeof(int));

    if(login == 1)
    {
        while(1)
        {
            int choice;
            printf("\nWhat would you like to do? If you're a new customer please register first. Enter 0 to register. \n1)View all products in store \n2)View your cart \n3)Buy a product \n4)Edit cart \n5)Confirm cart and go to payment \n6)Exit\nEnter choice:");
            scanf("%d", &choice);
            write(socketfd, &choice, sizeof(int));

            if(choice == 0)
            {
                printf("Enter 'b' if you want to go back or 'c' to continue\n");
                char confirm;
                //printf("Enter 'b' if you want to go back or 'c' to continue");
                scanf("%c", &confirm);
                scanf("%c", &confirm);
                write(socketfd, &confirm, sizeof(confirm));
                if(confirm == 'b')
                    printf("terminated\n");
                else
                {
                    int id;
                    read(socketfd, &id, 4);
                    printf("Your customer id is: %d", id);
                }
            }
            else if(choice == 1)
            {
                displayStore(socketfd);
            }

            else if(choice == 2)
            {
                int cid = getCustomerID();

                write(socketfd, &cid, sizeof(int));

                struct cart c_rcvd;
                read(socketfd, &c_rcvd, sizeof(struct cart));

                displayMycart(c_rcvd);
            }
            else if(choice == 3)
            {
                int cid = getCustomerID();

                write(socketfd, &cid, sizeof(int));

                int res;
                read(socketfd, &res, sizeof(int));
                if (res == -1)
                {
                    printf("Invalid customer id\n");
                    continue;
                }

                char response[80];
                int pid, qty;
                pid = setProductID();
                qty = setProductQty();
                
                struct product p;
                p.prod_id = pid;
                p.qty = qty;

                //passing product data to server
                write(socketfd, &p, sizeof(struct product));
                read(socketfd, response, sizeof(response));
                printf("%s", response);
            }
            else if(choice == 4)
            {
                int cid = getCustomerID();

                write(socketfd, &cid, sizeof(int));

                int res;
                read(socketfd, &res, sizeof(int));
                if (res == -1)
                {
                    printf("Invalid customer id\n");
                    continue;
                }

                int pid = setProductID();
                int qty = setProductQty();

                struct product p;
                p.prod_id = pid;
                p.qty = qty;

                //passing product data to server
                char response[80];
                write(socketfd, &p, sizeof(struct product));
                read(socketfd, response, sizeof(response));
                printf("%s", response);
            }
            else if(choice == 5)
            {
                int cid = getCustomerID();

                write(socketfd, &cid, sizeof(int));

                int res;
                read(socketfd, &res, sizeof(int));
                if (res == -1)
                {
                    printf("Invalid customer id\n");
                    continue;
                }

                struct cart c;
                read(socketfd, &c, sizeof(struct cart));

                printf("Here is a summary of your cart\n");

                int bought, stock, cost;

                for(int i = 0; i < MAX_CART; i++)
                {
                    if(c.items[i].prod_id != -1)
                    {
                        read(socketfd, &bought, sizeof(int));
                        read(socketfd, &stock, sizeof(int));
                        read(socketfd, &cost, sizeof(int));
                        printf("Product id- %d\n", c.items[i].prod_id);
                        printf("Ordered - %d; In stock - %d; Price - %d\n", bought, stock, cost);
                        c.items[i].qty = stock;
                        c.items[i].cost = cost;//why
                    }
                }

                printf("---end----\n");

                int total = getTotal(c); int payed;
                printf("Total amount to be payed: %d\n", total);

                while(1)
                {
                    printf("Enter amount you want to pay: ");
                    scanf("%d", &payed);
                    if(payed != total)
                        printf("Wrong amount, kindly re-enter.\n");
                    else
                        break;
                }

                char ch = 'y';
                printf("-------Payment successful!------\n");
                write(socketfd, &ch, sizeof(char));
                read(socketfd, &ch, sizeof(char));
                generateReceipt(c, total);

            }
            else if(choice == 6)
            {
                printf("Thank you for shopping with us!\n");
                break;
            }
            else if(choice != 0 || choice != 1 || choice != 2 || choice != 3 || choice != 4 || choice != 5 || choice != 6)
            {
                printf("Invalid option try again\n");
            }

        }
    }
    
    //admin
    if(login == 2)
    {
        printf("----------ADMIN-----------\n");

        while(1)
        {
            printf("\nWhat would you like to do \n1)Add a product \n2)Modify price of existing product\n3)Modify quantity of existing product\n4)Delete product\n5)View all products in store \n6)Exit\nEnter:");
            int choice = 0;
            scanf("%d", &choice);
            write(socketfd, &choice, 4);

            if(choice == 1)
            {
                struct product p;
                p.prod_id = setProductID();
                char name[100];
                printf("Enter product name: \n");
                scanf("%s", name);
                strcpy(p.pname, name);
                //strcpy(p.pname, setProductName());
                p.cost = setProductCost();
                p.qty = setProductQty();
                

                write(socketfd, &p, sizeof(struct product));

                char response[80];
                int n = read(socketfd, response, sizeof(response));
                response[n] = '\0';
                printf("%s", response);
            }
            else if(choice == 2)
            {
                struct product p;
                p.prod_id = setProductID();
                p.cost = setProductCost();
                write(socketfd, &p, sizeof(struct product));

                char response[80];
                read(socketfd, response, sizeof(response));
                printf("%s", response);

            }
            else if(choice == 3)
            {
                struct product p;
                p.prod_id = setProductID();
                p.qty = setProductQty();
                write(socketfd, &p, sizeof(struct product));
                
                char response[80];
                read(socketfd, response, sizeof(response));
                printf("%s", response);

            }
            else if(choice == 4)
            {
                //for product to be deleted, its id = -1;
                int id = setProductID();
                write(socketfd, &id, 4);

                char response[80];
                read(socketfd, response, sizeof(response));
                printf("%s", response);
            }
            else if(choice == 5)
            {
                displayStore(socketfd);
            }
            else if(choice == 6)
            {
                break;
            }
            else
                printf("Invalid option, try again 2\n");
        }
    }
    // else
    // {
    //     printf("Invalid option, try again\n");
    // }
    close(socketfd);
    printf("Exiting...");
    return 0;
    
}