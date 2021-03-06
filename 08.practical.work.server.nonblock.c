#define _GNU_SOURCE

#include <arpa/inet.h>
#include <errno.h>
#include <error.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SV_PORT 8784
#define SV_MAXEVENTS 1024
#define BUFCOUNT 4096

int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (sockfd < 0) {
        perror("error creating sockfd");
        return 1;
    }

    int one = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int)) != 0) {
        perror("error setting SO_REUSEADDR");
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

    int pol = epoll_create1(0);
    if (pol <= 0) {
        perror("error creating epoll");
        return 1;
    }
    struct epoll_event *events = malloc(SV_MAXEVENTS * sizeof(struct epoll_event));
    size_t bufsize = BUFCOUNT * sizeof(char);
    char *buf = malloc(bufsize);

    // register server socket for notifications
    struct epoll_event ssopts = {0};
    ssopts.events = EPOLLIN | EPOLLET;
    ssopts.data.fd = sockfd;
    if (epoll_ctl(pol, EPOLL_CTL_ADD, sockfd, &ssopts) != 0) {
        perror("cannot set sockfd events");
        return 1;
    }

    while (1) {
        int pending = epoll_wait(pol, events, SV_MAXEVENTS, -1);
        if (pending < 0) {
            perror("error waiting for new event");
            continue;
        }
        for (int ei = 0; ei < pending; ei++) {
            uint32_t evcode = events[ei].events;
            int clifd = events[ei].data.fd;
            if (evcode & EPOLLERR || evcode & EPOLLHUP || !(evcode & EPOLLIN)) {
                fprintf(stderr, "oops");
                close(clifd);
                if (clifd == sockfd) {
                    return 1;
                }
                continue;
            } else if (clifd == sockfd) {
                // accept new connections
                while (1) {
                    struct sockaddr_in ca;
                    socklen_t clen = sizeof(struct sockaddr_in);
                    int newfd = accept4(sockfd, (struct sockaddr *)&ca, &clen, SOCK_NONBLOCK);
                    if (newfd < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            // no connections waiting for accept
                            break;
                        } else {
                            perror("error accepting connection");
                        }
                    } else {
                        printf("accepting client %d\n", newfd);
                        struct epoll_event copts = {0};
                        copts.events = EPOLLIN | EPOLLET;
                        copts.data.fd = newfd;
                        if (epoll_ctl(pol, EPOLL_CTL_ADD, newfd, &copts) != 0) {
                            perror("cannot set fd events");
                            close(newfd);
                        }
                    }
                }
            } else if (evcode & EPOLLIN) {
                // save space for \0
                ssize_t readsize = read(clifd, buf, bufsize - sizeof(char));
                buf[readsize] = '\0';
                if (readsize > 0) {
                    printf("%d: %s\n", clifd, buf);
                } else if (readsize == 0) {
                    printf("client connection done\n");
                    epoll_ctl(pol, EPOLL_CTL_DEL, clifd, NULL);
                    close(clifd);
                } else {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    } else {
                        perror("clifd complained");
                        epoll_ctl(pol, EPOLL_CTL_DEL, clifd, NULL);
                        close(clifd);
                    }
                }
                bool end = false;
                for (ssize_t i = 0; i < readsize; i++) {
                    if (buf[i] == '\0') {
                        end = true;
                    }
                }
                if (end) {
                    printf("client connection done\n");
                    epoll_ctl(pol, EPOLL_CTL_DEL, clifd, NULL);
                    close(clifd);
                }
            }
        }
    }
}
