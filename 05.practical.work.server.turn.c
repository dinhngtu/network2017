#define _GNU_SOURCE

#include <arpa/inet.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define SV_PORT 8784
#define BUFCOUNT 4096

int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("error creating sockfd");
        return 1;
    }

    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(struct sockaddr_in));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    sa.sin_port = htons(SV_PORT);

    if (bind(sockfd, (struct sockaddr *)&sa, sizeof(struct sockaddr_in)) < 0) {
        perror("error binding");
        return 1;
    }

    if (listen(sockfd, 5) < 0) {
        perror("error listening");
        return 1;
    }

    while (1) {
        struct sockaddr_in ca;
        socklen_t clen = sizeof(struct sockaddr_in);
        int clifd = accept(sockfd, (struct sockaddr *)&ca, &clen);
        if (clifd < 0) {
            perror("error accepting connection");
            continue;
        }

        if (write(clifd, "Welcome\n", 8 * sizeof(char)) <= 0) {
            perror("error writing to client");
            close(clifd);
            continue;
        }

        size_t bufsize = BUFCOUNT * sizeof(char);
        char *buf = malloc(bufsize);

        while (1) {
            write(1, "< ", 2 * sizeof(char));
            ssize_t readsize;
            while ((readsize = read(clifd, buf, bufsize)) > 0) {
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
                perror("error reading from client");
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
            if (write(clifd, buf, numchars * sizeof(char)) < 0) {
                perror("error writing to client");
                break;
            }
        }

        if (write(clifd, "Bye\n", 4 * sizeof(char)) < 0) {
            perror("error writing to client");
            close(clifd);
            continue;
        }

        if (shutdown(clifd, SHUT_WR) != 0) {
            perror("error shutting down socket");
            close(clifd);
            continue;
        }

        char dummy;
        while (read(clifd, &dummy, sizeof(char)) > 0);

        if (close(clifd) != 0) {
            perror("error closing client connection");
            continue;
        }

        printf("client connection done\n");
    }
}
