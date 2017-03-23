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

int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("error creating sockfd");
        return 1;
    }

    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(struct sockaddr_in));
    sa.sin_family = AF_INET;
    assert(inet_aton("127.0.0.1", &sa.sin_addr) != 0);
    sa.sin_port = htons(SV_PORT);

    if (connect(sockfd, (struct sockaddr *)&sa, sizeof(struct sockaddr_in)) < 0) {
        perror("error connecting");
        return 1;
    }

    size_t bufsize = BUFCOUNT * sizeof(char);
    char *buf = malloc(bufsize);

    ssize_t readsize;
    while ((readsize = read(sockfd, buf, bufsize)) > 0) {
        if (write(1, buf, readsize * sizeof(char)) == -1) {
            perror("error writing to stdout");
            return 1;
        }
    }

    if (readsize == -1) {
        perror("error reading from server");
        return 1;
    }

    if (shutdown(sockfd, SHUT_WR) != 0) {
        perror("error shutting down socket");
        close(sockfd);
        return 1;
    }

    if (close(sockfd) != 0) {
        perror("error closing connection");
        return 1;
    }

    return 0;
}
