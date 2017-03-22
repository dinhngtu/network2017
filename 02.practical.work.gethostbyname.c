#define _BSD_SOURCE
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char **argv) {
    size_t bufsize;
    char *name = NULL;

    if (argc == 1) {
        printf("Enter hostname: ");
        int nchar = getline(&name, &bufsize, stdin);
        if (nchar < 1) {
            fprintf(stderr, "Bad name\n");
            return 1;
        }
        name[nchar - 1] = 0; // erase the \n
    } else {
        name = argv[1];
    }

    struct hostent *he = gethostbyname(name);
    if (!he) {
        printf("no such domain");
        return 2;
    }
    for (char **addr = he->h_addr_list; *addr; addr++) {
        struct in_addr ip;
        ip.s_addr = *(in_addr_t *)*addr;
        printf("resolved ip is %s\n", inet_ntoa(ip));
    }

    return 0;
}
