#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "product_cart.h"
#define PORT 8080

//todo: figure out buying products and payment part, when to check for valid purchase

int main()
{
    struct sockaddr_in client;
    int socketfd, new_sd, connectfd;
    socketfd = socket (AF_INET, SOCK_STREAM, 0);//creating socket filed

    client.sin_family = AF_INET;
    client.sin_addr.s_addr = INADDR_ANY;
    client.sin_port = htons (PORT);

    connectfd = connect(socketfd, (struct sockaddr_*)&client, sizeof(client)); //connecting to server
    if(connectfd == -1)
        perror("connect failed");

    int choice = 0;
    
    while(choice != 6)
    {
        printf("******************Online Retail Store********************\n");
        printf("Enter the number what you would like to do \n1)View all products \n2)View your cart \n3)Buy a product \n4)Edit cart \n5)Confirm cart and go to payment \n6)Exit");
        scanf("%d", &choice);


        if(choice == 1)
        {
            int val = 1;
            write(socketfd, &val, 4); //send request to server to get all product details

            struct product p;

            printf("Product ID          Product Name          Cost       Qty\n");

            read(socketfd, &p, sizeof(struct product));
            while(p.prod_id != 0)
            {
                printf("%d      %s     %d       %d\n", p.prod_id, p.pname, p.cost, p.qty);
            }
        }
        if(choice == 2)
        {
            int val = 2, custid = 1;
            write(socketfd, &val, 4);
            // printf("Enter customer id: ");
            // scanf("%d", &custid);
            write(socketfd, &custid, 4);
            struct cart mycart;

            read(socketfd, &mycart, sizeof(struct cart));

            for(int i = 0; i < 10; i++)
            {
                if(mycart.items[i].prod_id != -1)
                {
                    printf("");
                }
            }
        }
        if(choice == 3)
        {
            int id, val = 3, qty;
            printf("Enter product id of product you want to buy: ");
            scanf("%d", &id);
            printf("Enter quantity of product you want to buy: ");
            scanf("%d", &qty);
            write(socketfd, &val, 4);
            write(socketfd, &id, 4);
            write(socketfd, &qty, 4);
            //how to do this, wym by adding multiple?

        }
        if(choice == 4)
        {
            int id, val = 3, qty;
            printf("Enter product id of product you want to change: ");
            scanf("%d", &id);
            printf("Enter quantity of product you want to change to: ");
            scanf("%d", &qty);
            write(socketfd, &val, 4);
            write(socketfd, &id, 4);
            write(socketfd, &qty, 4);
            //to delete a product make its qty 0 
        }











    }
    
}