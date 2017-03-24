#define _GNU_SOURCE

#include <arpa/inet.h>
#include <assert.h>
#include <error.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define SV_PORT 8784
#define BUFCOUNT 4096

int main(int argc, char **argv) {
    size_t namesize;
    char *name = NULL;

    if (argc == 1) {
        printf("Enter hostname: ");
        int nchar = getline(&name, &namesize, stdin);
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
        printf("no such domain\n");
        return 2;
    }

    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(struct sockaddr_in));
    sa.sin_family = AF_INET;
    memcpy((char *)&sa.sin_addr.s_addr, he->h_addr_list[0], he->h_length);
    sa.sin_port = htons(SV_PORT);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("error creating sockfd");
        return 1;
    }

    if (connect(sockfd, (struct sockaddr *)&sa, sizeof(struct sockaddr_in)) < 0) {
        perror("error connecting");
        return 1;
    }

    size_t bufsize = BUFCOUNT * sizeof(char);
    char *buf = malloc(bufsize);

    while (1) {
        write(1, "< ", 2 * sizeof(char));
        ssize_t readsize;
        while ((readsize = read(sockfd, buf, bufsize)) > 0) {
            write(1, buf, readsize);
            int el = 0;
            for (ssize_t i = 0; i < readsize; i++) {
                if (buf[i] == '\n') {
                    el = 1;
                }
            }
            if (el) {
                break;
            }
        }
        if (strcmp("Bye\n", buf) == 0) {
            break;
        }
        if (readsize == -1) {
            perror("error reading from server");
            break;
        }

        write(1, "> ", 2 * sizeof(char));
        if (!fgets(buf, BUFCOUNT, stdin)) {
            break;
        }
        size_t numchars = strlen(buf);
        if (numchars > BUFCOUNT) {
            numchars = BUFCOUNT;
        }
        if (numchars == 0) {
            break;
        }
        if (write(sockfd, buf, numchars * sizeof(char)) < 0) {
            perror("error writing to server");
            break;
        }
    }

    if (write(sockfd, "Bye\n", 4 * sizeof(char)) < 0) {
        perror("error writing to client");
        close(sockfd);
        return 1;
    }

    if (shutdown(sockfd, SHUT_WR) != 0) {
        perror("error shutting down socket");
        close(sockfd);
        return 1;
    }

    char dummy;
    while (read(sockfd, &dummy, sizeof(char)) > 0);

    if (close(sockfd) != 0) {
        perror("error closing connection");
        return 1;
    }

    return 0;
}
