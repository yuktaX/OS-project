#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include<fcntl.h>
#include "product_cart.h";
#define PORT 8080
#define MAX_STORE_SIZE 100

int main()
{
    
    struct product store[MAX_STORE_SIZE]; //array of struct of products, read into this from file
    struct cart orders[MAX_STORE_SIZE]; //purchases of the customers, each struct corresp to one customer
    int fd_products, fd_cart;
    char option;
    fd_products = open("products.txt", O_CREAT | O_EXCL | O_RDWR);
    if(fd_products == -1)
    {
        perror("open file");
    }
    fd_cart = open("cart.txt", O_CREAT | O_EXCL | O_RDWR);
    if(fd_products == -1)
    {
        perror("open file");
    }

    for(int i = 0; i < MAX_STORE_SIZE; i++)
    {
        read(fd_products, &store[i], sizeof(struct product));
    }
    for(int i = 0; i < MAX_STORE_SIZE; i++)
    {
        read(fd_cart, &orders[i], sizeof(struct cart));
    }



    printf("Enter 'c' to receive requests from client\n Enter 'a' to execute admin tasks\n");
    scanf("%c", &option);

    if(option == 'c')
    {
        struct sockaddr_in server, client;
        fd_set readfds;
        int socketfd, new_sd, max_clients = 10, maxsd, sd;
        int client_socket[10];
        
        for(int i = 0; i < max_clients; i++)
        {
            client_socket[i] = 0; //initialise all clients socket to 0 which is unchecked
        }

        socketfd = socket (AF_INET, SOCK_STREAM, 0);//creating master socketfd

        server.sin_family = AF_INET;
        server.sin_addr.s_addr = INADDR_ANY;
        server.sin_port = htons (PORT);

        bind(socketfd, (struct sockaddr *)&server, sizeof (server));
        if(listen (socketfd, 1) < 0){
            perror("listen");
            exit(EXIT_FAILURE);
        } //wait for client

        int val;
        while(1)
        {
            FD_ZERO(&readfds);//ckear socket set

            FD_SET(socketfd, &readfds);
            maxsd = socketfd;

             //add child sockets to set 
            for (int i = 0 ; i < max_clients ; i++)  
            {  
                //socket descriptor 
                sd = client_socket[i];  

                //if valid socket descriptor then add to read list 
                if(sd > 0)  
                    FD_SET( sd , &readfds);  

                //highest file descriptor number, need it for the select function 
                if(sd > maxsd)  
                    maxsd = sd;  
            }  

            if (FD_ISSET(socketfd, &readfds))  
            {  
            if ((new_sd = accept(socketfd, (struct sockaddr *)&server, (socklen_t*)&server))<0)  
            {  
                perror("accept");  
                exit(EXIT_FAILURE);  
            }  
             
            //inform user of socket number - used in send and receive commands 
            printf("New connection , socket fd is %d , ip is : %s , port : %d\n" , new_sd , inet_ntoa(server.sin_addr) , ntohs(server.sin_port));  
           
            // //send new connection greeting message 
            // if( send(new_sd, message, strlen(message), 0) != strlen(message) )  
            // {  
            //     perror("send");  
            // }  
                 
            //add new socket to array of sockets 
            for (int i = 0; i < max_clients; i++)  
            {  
                //if position is empty 
                if( client_socket[i] == 0 )  
                {  
                    client_socket[i] = new_sd;  
                    printf("Adding to list of sockets as %d\n" , i);  
                         
                    break;  
                }  
            }  
        }

            new_sd = accept(socketfd, (struct sockaddr *)&client, sizeof(client));; //connection request from client, ready to comm
            read(new_sd, &val, 4);
            
            if(val == 1)
            {
                for(int i = 0; i < MAX_STORE_SIZE; i++)
                {
                    if(store[i].qty != 0)
                    {
                        write(new_sd, &store[i], sizeof(struct product));
                    }
                }
            }
            if(val == 2)
            {
                int custid;
                read(new_sd, &custid, 4);
                for(int i = 0; i < MAX_STORE_SIZE; i++)
                {
                    if(orders[i].cust_id == custid)
                    {
                        write(new_sd, &orders[i], sizeof(struct cart));
                    }
                }
            }
            if(val == 3)
            {
                int quantity, id, custid = 1;
                read(new_sd, &id,4);
                read(new_sd, &quantity,4);
                for(int i = 0; i < MAX_STORE_SIZE; i++)
                {
                    if(orders[i].cust_id == custid)
                    {
                        for(int j = 0; j < MAX_STORE_SIZE; j++)
                        {
                            if(store[j].prod_id == id)
                            {
                                struct product newp;
                                newp = store[j];
                                newp.qty = quantity;
                                orders[i].items[orders[i].uniq_count] = newp;
                                orders[i].uniq_count++;
                            }
                        }
                    }
                }

            }
            if(val == 4)
            {
                int quantity, id, custid = 1;
                read(new_sd, &id,4);
                read(new_sd, &quantity,4);
                for(int i = 0; i < MAX_STORE_SIZE; i++)
                {
                    if(orders[i].cust_id == custid)
                    {
                        for(int j = 0; j < MAX_STORE_SIZE; j++)
                        {
                            if(store[j].prod_id == id)
                            {
                                struct product newp;
                                newp = store[j];
                                newp.qty = quantity;
                                for(int k = 0; k < 50; k++)
                                {
                                    if(orders[i].items[k].prod_id == id)
                                    {
                                        orders[i].items[k].qty = quantity;
                                        if(quantity == 0)
                                            orders[i].items[k].prod_id = 0;//this deletes the item from cart
                                    }
                                }
                            }
                        }
                    }
                }

            }
            
        }
    }

    if(option == 's')
    {
        printf("----------ADMIN-----------\n");

        int choice = 0;
        while(choice < 5)
        {
            printf("Enter the option of what you want to do \n1)Add a product \n2)Modify price of existing product\n3)Modify quantity of existing product\4)Delete product \n5)Exit");

            if(choice == 1)
            {
                struct product newp;
                printf("Enter product name: ");
                scanf("%s", &newp.pname);
                printf("Enter product id: ");
                scanf("%s", &newp.prod_id);
                printf("Enter product cost: ");
                scanf("%s", &newp.cost);
                printf("Enter product quantity: ");
                scanf("%s", &newp.qty);

                struct flock lock; int flg = 0;
                lock.l_len = sizeof(struct product);
                lock.l_start = SEEK_SET;
                lock.l_type = F_WRLCK;

                for(int i = 0; i < MAX_STORE_SIZE; i++)
                {
                    if(store[i].prod_id == -1) //find empty loc to write new product
                    {
                        lock.l_whence = (i + 1)*sizeof(struct product);
                        fcntl(fd_products, F_SETLKW, &lock);
                        write(fd_products, &newp, sizeof(struct product));
                        sleep(3);
                        store[i] = newp;
                        fcntl(fd_products, F_UNLCK); 
                        flg = 1;
                        break;
                    }
                }
                if(flg == 0)
                {
                    printf("Store full, cannot add more items\n");
                }

            }
            if(choice == 2)
            {
                int id;
                printf("Enter product id of product price you want to change: ");
                scanf("%d", &id);

                struct flock lock; int flg = 0;
                lock.l_len = sizeof(struct product);
                lock.l_start = SEEK_SET;
                lock.l_type = F_WRLCK;

                for (int i = 0; i < MAX_STORE_SIZE; i++)
                {
                    if(store[i].prod_id == id)
                    {
                        int newcost;
                        printf("\nEnter new price: ");
                        scanf("%d", &newcost);
                        struct product tmp;
                        lock.l_whence = (i + 1)*sizeof(struct product);
                        fcntl(fd_products, F_SETLKW, &lock);
                        //read(fd_products, &tmp, sizeof(stuct product));
                        store[i].cost = newcost;
                        write(fd_products, &store[i], sizeof(struct product));
                        sleep(3);
                        fcntl(fd_products, F_UNLCK); 
                        flg = 1;
                        break;
                    }
                }
                if(flg == 0)
                {
                    printf("That product id does not exist, try again.\n");
                }
                
            }
            if(choice == 3)
            {
                int id;
                printf("Enter product id of product quantity you want to change: ");
                scanf("%d", &id);

                struct flock lock; int flg = 0;
                lock.l_len = sizeof(struct product);
                lock.l_start = SEEK_SET;
                lock.l_type = F_WRLCK;

                for (int i = 0; i < MAX_STORE_SIZE; i++)
                {
                    if(store[i].prod_id == id)
                    {
                        int newqty;
                        printf("\nEnter new quantity: ");
                        scanf("%d", &newqty);
                        struct product tmp;
                        lock.l_whence = (i + 1)*sizeof(struct product);
                        fcntl(fd_products, F_SETLKW, &lock);
                        //read(fd_products, &tmp, sizeof(stuct product));
                        store[i].qty = newqty;
                        write(fd_products, &store[i], sizeof(struct product));
                        sleep(3);
                        fcntl(fd_products, F_UNLCK); 
                        flg = 1;
                        break;
                    }
                }
                if(flg == 0)
                {
                    printf("That product id does not exist, try again.\n");
                }
                
            }if(choice == 4)
            {
                int id;
                printf("Enter product id of product you want to delete: ");
                scanf("%d", &id);

                struct flock lock; int flg = 0;
                lock.l_len = sizeof(struct product);
                lock.l_start = SEEK_SET;
                lock.l_type = F_WRLCK;

                for (int i = 0; i < MAX_STORE_SIZE; i++)
                {
                    if(store[i].prod_id == id)
                    {
                        lock.l_whence = (i + 1)*sizeof(struct product);
                        fcntl(fd_products, F_SETLKW, &lock);
                        store[i].prod_id = -1;
                        write(fd_products, &store[i], sizeof(struct product));
                        sleep(3);
                        fcntl(fd_products, F_UNLCK); 
                        flg = 1;
                        break;
                    }
                }
                if(flg == 0)
                {
                    printf("That product id does not exist, try again.\n");
                }
                
            }
        }




    }

}