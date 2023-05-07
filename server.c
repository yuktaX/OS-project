#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include<fcntl.h>
#include "product_cart.h"
#define PORT 5555
#define MAX_STORE_SIZE 100


//functions for locking
void ReadProductLock(int fd, struct flock lock)
{
    lock.l_len = 0;
    lock.l_type = F_RDLCK;
    lock.l_start = 0;
    lock.l_whence = SEEK_SET;
    fcntl(fd, F_SETLKW, &lock);
}

void WriteProductLock(int fd, struct flock lock)
{
    lseek(fd, (-1)*sizeof(struct product), SEEK_CUR);
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_CUR;
    lock.l_start = 0;
    lock.l_len = sizeof(struct product);
    fcntl(fd, F_SETLKW, &lock);
}

void CustomerLock(int fd_cust, struct flock lock)
{   //locks customers file
    lock.l_len = 0;
    lock.l_type = F_RDLCK;
    lock.l_start = 0;
    lock.l_whence = SEEK_SET;
    fcntl(fd_cust, F_SETLKW, &lock);
}

void Unlock(int fd, struct flock lock)
{
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLKW, &lock);
}

void LockCart(int fd_cart, struct flock lock_cart, int offset, int ch)//--whats ch
{
    lock_cart.l_whence = SEEK_SET;
    lock_cart.l_len = sizeof(struct cart);
    lock_cart.l_start = offset;
    if (ch == 1)
    {
        lock_cart.l_type = F_RDLCK;
    }
    else
    {
        lock_cart.l_type = F_WRLCK;
    }
    fcntl(fd_cart, F_SETLKW, &lock_cart);
    lseek(fd_cart, offset, SEEK_SET);
}

int getOffset(int cid, int fd_cust)
{
    if(cid  < 0)
        return -1;
    struct flock lock;
    struct index n;
    CustomerLock(fd_cust, lock);

    while(read(fd_cust, &n, sizeof(struct index)))
    {
        if(n.key == cid)
        {
            Unlock(fd_cust, lock);
            return n.offset;
        }
    }
}

void addProduct(int fd, int newsd)
{
    int id, cost, qty, flg = 0; char name[100];
    struct product p;

    read(newsd, &p, sizeof(struct product));

    id = p.prod_id;
    cost = p.cost;
    qty = p.qty;
    strcpy(name, p.pname);

    struct flock lock;
    ReadProductLock(fd, lock);
    struct product p1;

    while(read(fd, &p1, sizeof(struct product)))
    {
        if (p1.prod_id == id && p1.qty > 0)
        {
            write(newsd, "Duplicate product, please use different id\n", sizeof("Duplicate product, please use different id\n"));
            Unlock(fd, lock);
            flg = 1;
            break;
        }
    }
    if(flg == 0)
    {
        lseek(fd, 0, SEEK_END);
        p.prod_id = id;
        strcpy(p.pname, name);
        p.cost= cost;
        p.qty = qty;

        write(fd, &p, sizeof(struct product));
        write(newsd, "Added successfully\n", sizeof("Added succesfully\n"));
        Unlock(fd, lock); 
    }
}

void showProducts(int fd, int newsd)
{
    struct flock lock;
    ReadProductLock(fd, lock);

    struct product p;
    while(read(fd, &p, sizeof(struct product)))
    {
        if (p.prod_id != -1)
            write(newsd, &p, sizeof(struct product));
    }
    
    p.prod_id = -1;//to indicate end of list, we set one as -1
    write(newsd, &p, sizeof(struct product));
    Unlock(fd, lock);
}
void deleteProduct(int fd, int newsd, int id){

    struct flock lock;
    ReadProductLock(fd, lock);

    struct product p;
    int flg = 0;
    while (read(fd, &p, sizeof(struct product)))
    {
        if (p.prod_id == id)
        {
            Unlock(fd, lock);
            WriteProductLock(fd, lock);

            p.prod_id = -1; //id == -1 indicates invalid/deleted product
            strcpy(p.pname, "");
            p.cost = -1;
            p.qty = -1;

            write(fd, &p, sizeof(struct product));
            write(newsd, "Delete successful", sizeof("Delete successful"));

            Unlock(fd, lock);
            flg = 1;
            break;
        }
    }
    if (flg == 0)
    {
        write(newsd, "Invalid product id", sizeof("Invalid product id"));
        Unlock(fd, lock);
    }
}

void updateProduct(int fd, int newsd, int choice)
{
    int id, val = -1, flg = 0;
    struct product p1;
    read(newsd, &p1, sizeof(struct product));
    id = p1.prod_id;
    
    if (choice == 1)//update cost
        val = p1.cost;
    else
        val = p1.qty; //update quantity

    if (choice == 2 && val == 0)
    {
        deleteProduct(fd, newsd, id);
        return;
    }

    struct flock lock;
    ReadProductLock(fd, lock);
    
    struct product p;
    while (read(fd, &p, sizeof(struct product)))
    {
        if (p.prod_id == id)
        {
            Unlock(fd, lock);
            WriteProductLock(fd, lock);

            if (choice == 1)
                p.cost = val;
            else
                p.qty = val;

            write(fd, &p, sizeof(struct product));

            if (choice == 1)
                write(newsd, "Price modified", sizeof("Price modified"));
            else
                write(newsd, "Quantity modified", sizeof("Quantity modified"));               

            Unlock(fd, lock);
            flg = 1;
            break;
        }
    }

    if(flg == 0)
    {
        write(newsd, "Invalid product id", sizeof("Invalid product id"));
        Unlock(fd, lock);
    }
}

//client functions
void AddCustomer(int fd_cart, int fd_custs, int newsd)
{
    char buf;
    read(newsd, &buf, sizeof(char));
    if (buf == 'c')
    {
        struct flock lock;
        CustomerLock(fd_custs, lock);
        
        int max_id = -1; 
        struct index id;
        while(read(fd_custs, &id, sizeof(struct index))) //finding the lastest id added to file/store
        {
            if (id.key > max_id)
                max_id = id.key;
        }

        max_id++;
        
        id.key = max_id; //adding new id and cart id to file
        id.offset = lseek(fd_cart, 0, SEEK_END);
        lseek(fd_custs, 0, SEEK_END);
        write(fd_custs, &id, sizeof(struct index));

        struct cart c;
        c.cust_id = max_id;
        for(int i = 0; i < MAX_CART; i++) //initialising cart for new customer
        {
            c.items[i].prod_id = -1;
            strcpy(c.items[i].pname , "");
            c.items[i].qty = -1;
            c.items[i].cost = -1;
        }

        write(fd_cart, &c, sizeof(struct cart));
        Unlock(fd_custs, lock);
        write(newsd, &max_id, sizeof(int));
    }
}

void ViewCart(int fd_cart, int newsd, int fd_custs)
{
    int cid = -1;
    read(newsd, &cid, sizeof(int));

    printf("um\n");

    int offset = getOffset(cid, fd_custs);//get offset from file
    struct cart c;

    if(offset == -1)//no cid found, so set to -1
    {
        struct cart c;
        c.cust_id = -1;
        write(newsd, &c, sizeof(struct cart));
        
    }
    else
    {
        struct cart c;
        struct flock lock_cart;
        
        LockCart(fd_cart, lock_cart, offset, 1);
        read(fd_cart, &c, sizeof(struct cart));
        write(newsd, &c, sizeof(struct cart));
        Unlock(fd_cart, lock_cart);
    }
}
void BuyProduct(int fd, int fd_cart, int fd_custs, int newsd)
{
    int cid = -1;
    read(newsd, &cid, sizeof(int));
    int offset = getOffset(cid, fd_custs);

    write(newsd, &offset, sizeof(int));

    if (offset == -1)
        return;

    struct flock lock_cart;
    
    int i = -1;
    LockCart(fd_cart, lock_cart, offset, 1);
    struct cart c;
    read(fd_cart, &c, sizeof(struct cart));//get customers cart

    struct flock lock_prod;
    ReadProductLock(fd, lock_prod);//lock products as buying is happening
    
    struct product p;struct product p1;
    read(newsd, &p, sizeof(struct product));//which prod to be added

    // int prod_id = p.prod_id;
    // int qty = p.qty;

    int found = 0;
    while (read(fd, &p1, sizeof(struct product)))//search for product in store
    {
        if (p1.prod_id == p.prod_id)
        {
            if (p1.qty >= p.qty)
            {
                found = 1;
                break;
            }
        }
    }
    Unlock(fd_cart, lock_cart);
    Unlock(fd, lock_prod);

    if(found == 0)
    {
        write(newsd, "Invalid product id or out of stock\n", sizeof("Invalid product id or out of stock\n"));
        return;
    }

    int flg = 0, flg1 = 0;

    for (int i = 0; i < MAX_CART; i++) //check if product already exists
    {
        if(c.items[i].prod_id == p.prod_id)
        {
            flg1 = 1;
            break;
        }
    }

    if(flg == 1)
    {
        write(newsd, "Product already exists in cart\n", sizeof("Product already exists in cart\n"));
        return;
    }

    for(int i = 0; i < MAX_CART; i++)//adding product to cart
    {
        if (c.items[i].prod_id == -1) //find the first empty location and enter the product there
        {
            flg = 1;
            c.items[i].prod_id = p.prod_id;
            c.items[i].qty = p.qty;
            strcpy(c.items[i].pname, p1.pname);
            c.items[i].cost = p1.cost;
            break;

        }
        else if(c.items[i].qty <= 0)//checking another condition if qty <= 0
        {
            flg = 1;
            c.items[i].prod_id = p.prod_id;
            c.items[i].qty = p.qty;
            strcpy(c.items[i].pname, p1.pname);
            c.items[i].cost = p1.cost;
            break;
        }
    }

    if(flg == 0)
    {
        write(newsd,"Cart limit reached\n", sizeof("Cart limit reached\n"));
        return;
    }

    write(newsd, "Item added to cart\n", sizeof("Item added to cart\n"));

    LockCart(fd_cart, lock_cart, offset, 2);    
    write(fd_cart, &c, sizeof(struct cart));
    Unlock(fd_cart, lock_cart);
}

void EditCart(int fd, int fd_cart, int fd_custs, int newsd){

    int cid = -1;
    read(newsd, &cid, sizeof(int));

    int offset = getOffset(cid, fd_custs);

    write(newsd, &offset, sizeof(int));
    if (offset == -1)
        return;
    

    struct flock lock_cart;
    LockCart(fd_cart, lock_cart, offset, 1);
    struct cart c;
    read(fd_cart, &c, sizeof(struct cart));

    int pid, qty;
    struct product p;
    read(newsd, &p, sizeof(struct product));


    int flg = -1, i;
    for (i = 0; i < MAX_CART; i++)//search for product in cart
    {
        if (c.items[i].prod_id == p.prod_id)
        {
            struct flock lock_prod;
            ReadProductLock(fd, lock_prod);

            struct product p1;
            while(read(fd, &p1, sizeof(struct product)))//check if product is there in store
            {
                if (p1.prod_id == pid && p1.qty > 0) 
                {
                    flg = 0;
                    if (p1.qty >= qty)
                    {
                        flg = 1;
                        break;
                    }
                }
            }

            Unlock(fd, lock_prod);
            break;
        }
    }
    Unlock(fd_cart, lock_cart);

    if(flg == 0)
    {
        write(newsd, "Product out of stock\n", sizeof("Product out of stock\n"));
        return;
    }
    if(flg == -1)
    {
        write(newsd, "Product not in cart \n", sizeof("Product not in cart\n"));
        return;
    }

    c.items[i].qty = qty;
    write(newsd, "Update successful\n", sizeof("Update successful\n"));
    LockCart(fd_cart, lock_cart, offset, 2);
    write(fd_cart, &c, sizeof(struct cart));
    Unlock(fd_cart, lock_cart);
}

void GetPayment(int fd, int fd_cart, int fd_custs, int newsd){
    int cid = -1;
    read(newsd, &cid, sizeof(int));

    int offset = getOffset(cid, fd_custs); //get the cart for resp customer id

    write(newsd, &offset, sizeof(int));
    if (offset == -1)
        return;

    struct flock lock_cart;
    LockCart(fd_cart, lock_cart, offset, 1);

    struct cart c;
    read(fd_cart, &c, sizeof(struct cart));
    Unlock(fd_cart, lock_cart);
    write(newsd, &c, sizeof(struct cart));

    int total = 0;

    for (int i = 0; i < MAX_CART; i++)
    {
        if (c.items[i].prod_id != -1)
        {
            write(newsd, &c.items[i].qty, sizeof(int));//send all products to get the total

            struct flock lock_prod;
            ReadProductLock(fd, lock_prod);
            lseek(fd, 0, SEEK_SET);

            struct product p;
            while (read(fd, &p, sizeof(struct product)))
            {
                if (p.prod_id == c.items[i].prod_id && p.qty > 0) 
                {
                    int min;
                    if(c.items[i].qty >= p.qty)//whymin
                        min = p.qty;
                    else
                        min = c.items[i].qty;

                    write(newsd, &min, sizeof(int));
                    write(newsd, &p.cost, sizeof(int));
                }
            }
            Unlock(fd, lock_prod);
        }      
    }

    char ch;
    read(newsd, &ch, sizeof(char));

    for (int i = 0; i < MAX_CART; i++)
    {
        struct flock lock_prod;
        ReadProductLock(fd, lock_prod);
        lseek(fd, 0, SEEK_SET);

        struct product p;
        while (read(fd, &p, sizeof(struct product)))
        {
            if (p.prod_id == c.items[i].prod_id) 
            {
                int min;
                if (c.items[i].qty >= p.qty)
                    min = p.qty;
                else
                    min = c.items[i].qty;
                
                Unlock(fd, lock_prod);
                WriteProductLock(fd, lock_prod);
                p.qty = p.qty - min;//update stock available

                write(fd, &p, sizeof(struct product));
                Unlock(fd, lock_prod);
            }
        }

        Unlock(fd, lock_prod);
    }
    
    LockCart(fd_cart, lock_cart, offset, 2);

    for(int i = 0; i < MAX_CART; i++)//reset cart for new customer
    {
        c.items[i].prod_id = -1;
        strcpy(c.items[i].pname, "");
        c.items[i].cost = -1;
        c.items[i].qty = -1;
    }

    write(fd_cart, &c, sizeof(struct cart));
    write(newsd, &ch, sizeof(char));
    Unlock(fd_cart, lock_cart);
}

int main()
{
    printf("setting up server...\n");
    //char option;
    int fd_products = open("products.txt", O_RDWR | O_CREAT, 0777);
    int fd_cart = open("orders.txt", O_RDWR | O_CREAT, 0777);
    int fd_index = open("index.txt", O_RDWR | O_CREAT, 0777);

    if(fd_products == -1 || fd_cart == -1 || fd_index == -1)
    {
        perror("open file");
    }

    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if(socketfd == -1)
    {
        perror("socket");
        return -1;
    }

    struct sockaddr_in server, client;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    int opt = 1;
    if(setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("Error setsock: ");
        return -1;
    }

    if(bind(socketfd, (struct sockaddr *)&server, sizeof(server)) == -1)
    {
        perror("bind");
        return -1;
    }
    if(listen(socketfd, 5) == -1)
    {
        perror("listen");
        return -1;
    } //wait for client

    printf("server ready\n");

    int s = sizeof(client);

    while(1)
    {
        int newsd = accept(socketfd, (struct sockaddr *)&client, &s);
        if(newsd == -1)
        {
            perror("error newsd"); 
            return -1;
        }

        printf("before fork\n");

        if(!fork())
        {
            printf("Connection established with client\n");
            close(socketfd);

            int login;
            read(newsd, &login, sizeof(int));

            printf("login %d\n", login);

            if(login == 1)//customer
            {
                int choice;
                while(1)
                {
                    read(newsd, &choice, sizeof(int));
                    //set all file pointers to beginning of file
                    lseek(fd_products, 0, SEEK_SET);
                    lseek(fd_cart, 0, SEEK_SET);
                    lseek(fd_index, 0, SEEK_SET);

                    if(choice == 0)
                    {
                        AddCustomer(fd_cart, fd_index, newsd);
                    }
                    else if(choice == 1)
                    {
                        showProducts(fd_products, newsd);
                    }
                    else if(choice == 2)
                    {
                        ViewCart(fd_cart, fd_index, newsd);
                    }
                    else if(choice == 3)
                    {
                        BuyProduct(fd_products, fd_cart, fd_index, newsd);
                    }
                     else if(choice == 4)
                    {
                        EditCart(fd_products, fd_cart, fd_index, newsd);
                    }
                    else if(choice == 5)
                    {
                        GetPayment(fd_products, fd_cart, fd_index, newsd);
                    }
                    else if(choice == 6)
                    {
                        close(newsd);
                        break;
                    }
                }
                printf("Connection closed\n");
            }

            else if(login == 2)
            {
                int choice = 0;
                while(1)
                {
                    //set all file pointers to beginning of file
                    lseek(fd_products, 0, SEEK_SET);
                    lseek(fd_cart, 0, SEEK_SET);
                    lseek(fd_index, 0, SEEK_SET);

                    read(newsd, &choice, sizeof(int));

                    if(choice == 1)
                    {
                        addProduct(fd_products,newsd);
                    }
                    else if(choice == 2)
                    {
                        updateProduct(fd_products, newsd, 1);
                    }
                    else if(choice == 3)
                    {
                        updateProduct(fd_products, newsd, 2);
                    }
                    else if(choice == 4)
                    {
                        int pid; read(newsd, &pid, 4);
                        deleteProduct(fd_products, newsd, pid);
                    }
                    else if(choice == 5)
                    {
                        showProducts(fd_products, newsd);
                    }
                    else if(choice == 6)
                    {
                        close(newsd);
                        break;
                    }

                    //printf("Connection closed\n");
                }
            }
            printf("Connection closed\n");
        }
        else
        {
            close(newsd);
        }
    }

    printf("Shutting down server...\n");
}