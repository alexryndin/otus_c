#include "dbg.h"
#include <bstring/bstring/bstrlib.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <sys/resource.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#define INOTIFYBUF_SIZE 4096
#define USAGE           "Usage: %s path_to_config [-d|--daemon]\n"

#undef max
#define max(x, y) ((x) > (y) ? (x) : (y))

struct config {
    bstring file_path;
    bstring sock_path;
};

int reload_config = 0;

void signal_handler(int a) {
    LOG_DEBUG("signal %d recieved", a);
    if (a == SIGHUP) {
        reload_config = 1;
    }
}

double format_readable_size(off_t size) {
    double ret = 0;
    while ((ret = size) > 1024) {
        size = size / 1024.;
    }
    return ret;
}

char *format_readable_prefix(off_t size) {
    if (size < 1024) {
        return "B";
    } else if ((size = size / 1024) < 1024) {
        return "K";
    } else if ((size = size / 1024) < 1024) {
        return "M";
    } else if ((size = size / 1024) < 1024) {
        return "G";
    } else if ((size = size / 1024) < 1024) {
        return "G";
    } else if ((size = size / 1024) < 1024) {
        return "T";
    } else if ((size / 1024) < 1024) {
        return "P";
    }
    return "UNKN";
}

int do_daemonize() {
    pid_t pid;

   // if (getrlimit(RLIMIT_NOFILE, &rl) < 0) {
   //     LOG_ERR("Cannot get number max file desctiptor number");
   //     goto error;
   // }

    CHECK((pid = fork()) >= 0, "Could't prefork");
    if (pid != 0) {
        exit(0);
    }

    setsid();

    CHECK((pid = fork()) >= 0, "Could't fork");
    if (pid != 0) {
        printf("%d\n", pid);
        exit(0);
    }

    return 0;
error:
    return -1;
}

int inotify_prepare(int *ifd, int *iwd, char *file_path) {
    int new_ifd = 0, new_iwd = 0;
    int nfds = 0;
    CHECK(ifd != NULL && iwd != NULL, "Null pointer to inotify desctiptor.");

    CHECK((new_ifd = inotify_init()) > 0, "Couldn't create new inotify fd.");

    CHECK((new_iwd = inotify_add_watch(new_ifd, file_path,
                                       IN_MODIFY | IN_DELETE_SELF)) > 0,
          "Couldn't create new inotify watcher.");
    if (*ifd != 0)
        close(*ifd);
    *ifd = new_ifd;
    *iwd = new_iwd;
    nfds = max(nfds, *ifd);
    return nfds;
error:
    if (new_ifd)
        close(new_ifd);
    return -1;
}

int read_conf(char *conf_path, struct config *conf) {
    FILE *fp = NULL;

    CHECK(conf_path != NULL, "Null conf_path.");
    CHECK(conf != NULL, "Null config.");

    CHECK((fp = fopen(conf_path, "r")) != NULL,
          "Couldn't open config for reading");

    conf->file_path = bgets((bNgetc)fgetc, fp, '\n');
    CHECK(conf->file_path != NULL, "Failed to read file_path");
    CHECK(btrimws(conf->file_path) == BSTR_OK, "Failed to trim input");
    CHECK(blength(conf->file_path) > 0, "Empty config");

    conf->sock_path = bgets((bNgetc)fgetc, fp, '\n');
    CHECK(conf->sock_path != NULL, "Failed to read sock_path");
    CHECK(btrimws(conf->sock_path) == BSTR_OK, "Failed to trim input");
    CHECK(blength(conf->sock_path) > 0, "Empty config");

    LOG_DEBUG("read file_path %s", bdata(conf->file_path));
    LOG_DEBUG("read sock_path %s", bdata(conf->sock_path));
    if (fp != NULL)
        fclose(fp);
    return 0;

error:
    if (fp != NULL)
        fclose(fp);
    if (conf->sock_path != NULL)
        bdestroy(conf->sock_path);
    if (conf->file_path != NULL)
        bdestroy(conf->file_path);
    return -1;
}

#define max_clients 20

#define SEND_TO_CLIENT(str, msg, ...)                                \
    str = bformat(msg, ##__VA_ARGS__);                               \
    rc = send(clients[i], bdata(str), blength(str), MSG_NOSIGNAL);   \
    if (rc == -1 && errno == EPIPE) {                                \
        close(clients[i]);                                           \
        LOG_DEBUG("Client communication failire, closing socket %d", \
                  clients[i]);                                       \
        clients[i] = 0;                                              \
        continue;                                                    \
    }                                                                \
    CHECK(rc > 0, "Couldn't send message to client %d", clients[i]); \
    bdestroy(str);                                                   \
    str = NULL;

#define NOTIFY_CLIENT(event)                                                   \
    {                                                                          \
        rc = send(clients[i], MSG_##event, strlen(MSG_##event), MSG_NOSIGNAL); \
        if (rc == -1 && errno == EPIPE) {                                      \
            close(clients[i]);                                                 \
            LOG_DEBUG("Client communication failire, closing socket %d",       \
                      clients[i]);                                             \
            clients[i] = 0;                                                    \
            continue;                                                          \
        }                                                                      \
        CHECK(rc > 0, "Couldn't send message to client %d", clients[i]);       \
    }

const char *MSG_BUSY = "Server is busy: too many clients\n";
const char *MSG_RDY = "Ready\n";
const char *MSG_EVNT = "Event found\n";
const char *MSG_IN_CLOSE_NOWRITE = "File closed without write\n";
const char *MSG_IN_CLOSE_WRITE = "File was written and closed\n";
const char *MSG_IN_OPEN = "File opened\n";
const char *MSG_IN_MODIFY = "File modified\n";
const char *MSG_IN_DELETE_SELF = "File deleted\n";
const char *MSG_IN_CREATE = "File created\n";

int main(int argc, char *argv[]) {
    int data_socket;
    int ret, ready;
    int ifd = 0, iwd = 0;
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    int nfds = 0, new_nfds = 0;
    int clients[max_clients] = {0};
    int clients_num = 0;
    int rc = 0;
    int daemonize = 0;
    char *inotify_buf = NULL;
    char *stat_error = NULL;
    char *conf_path = NULL;
    char end_loop = 0;
    bstring str = NULL;
    struct sigaction sa = {0};
    struct config conf = {0};
    struct stat filestat;
    fd_set readfds;
    fd_set readfds_mod;

    if (argc < 2) {
        goto usage;
    }

    conf_path = argv[1];
    CHECK(read_conf(conf_path, &conf) == 0, "Couldn't read config");
    if (argc > 2) {
        if (!strcmp(argv[2], "--daemon") || !strcmp(argv[2], "-d")) {
            daemonize = 1;
        } else {
            goto usage;
        }
    }

    sa.sa_handler = signal_handler;
    CHECK(sigaction(SIGHUP, &sa, NULL) >= 0, "Could't set signal handler");

    if (daemonize) {
        CHECK(do_daemonize() == 0, "Couldn't create a daemon.");
    }

    /* (from man 7 inotify):
     * Some systems cannot read integer variables if they are not
     * properly aligned. On other systems, incorrect alignment may
     * decrease performance. Hence, the buffer used for reading from
     * the inotify file descriptor should have the same alignment as
     * struct inotify_event. */
    inotify_buf =
        aligned_alloc(__alignof__(struct inotify_event), INOTIFYBUF_SIZE);

    CHECK_MEM(inotify_buf);
    // struct sigaction sa = {0};
    // sa.sa_handler = SIG_IGN;
    // CHECK(sigaction(SIGPIPE, &sa, NULL) == 0, "Could't set signal
    // handler");

    const struct inotify_event *in_event = NULL;

    CHECK((nfds = inotify_prepare(&ifd, &iwd, bdata(conf.file_path))) > 0,
          "Couldn't create inotify fd.");

    nfds = max(nfds, ifd);

    CHECK(sock >= 0, "Cannot create a socket");

    nfds = max(nfds, sock);

    struct sockaddr_un saddr = {.sun_family = AF_UNIX};
    strncpy(saddr.sun_path, bdatae(conf.sock_path, ""), sizeof(saddr.sun_path) - 1);

    CHECK(bind(sock, (struct sockaddr *)&saddr, sizeof(saddr)) == 0,
          "Couldn't bind a socket.");

    CHECK(listen(sock, 20) == 0, "Couldn't listen.");

    FD_ZERO(&readfds);
    FD_SET(sock, &readfds);
    FD_SET(ifd, &readfds);
    for (;;) {

        if (end_loop)
            goto exit;

        readfds_mod = readfds;

        if (reload_config) {
            if (read_conf(conf_path, &conf) != 0) {
                LOG_ERR("Could't reload config. Continue with old config.");
            } else {

                if ((new_nfds = inotify_prepare(&ifd, &iwd,
                                                bdata(conf.file_path))) < 0) {
                    LOG_ERR("Could't reload config. Continue with old config.");
                } else {

                    FD_ZERO(&readfds);
                    FD_SET(sock, &readfds);
                    FD_SET(ifd, &readfds);
                    readfds_mod = readfds;
                    nfds = new_nfds;
                }
            }
            reload_config = 0;
        }

        ready = select(nfds + 1, &readfds_mod, NULL, NULL, NULL);

        if (ready == -1 && errno == EINTR) {
            continue;
        }

        CHECK(ready >= 0, "select() failed.");

        if (FD_ISSET(sock, &readfds_mod)) {
            if (clients_num < max_clients) {
                CHECK((clients[clients_num] = accept(sock, NULL, NULL)) > 0,
                      "Couldn't accept connection.");
                CHECK(write(clients[clients_num], MSG_RDY, strlen(MSG_RDY)) > 0,
                      "Couldn't send message to client with fd %d",
                      clients[clients_num]);
                clients_num++;
            } else {
                data_socket = accept(sock, NULL, NULL);
                if (write(data_socket, MSG_BUSY, strlen(MSG_BUSY)) < 1) {
                    LOG_ERR("Couldn't send data to client");
                };
                close(data_socket);
            }
            continue;
        }

        if (FD_ISSET(ifd, &readfds_mod)) {

            LOG_DEBUG("event found");
            CHECK((ret = read(ifd, inotify_buf, INOTIFYBUF_SIZE)) > 0,
                  "Couldn't read inotify event");

            for (char *ptr = inotify_buf; ptr < inotify_buf + ret;
                 ptr += sizeof(struct inotify_event) + in_event->len) {
                in_event = (const struct inotify_event *)ptr;

                ret = lstat(bdatae(conf.file_path, ""), &filestat);
                if (ret == -1) {
                    stat_error = errno == 0 ? NULL : strerror(errno);
                }
                if (in_event->mask & IN_DELETE_SELF) {
                    end_loop = 1;
                }

                for (int i = 0; i < max_clients; i++) {
                    if (clients[i] != 0) {
                        LOG_DEBUG("notifying client %d", clients[i]);
                        NOTIFY_CLIENT(EVNT);
                        if (in_event->mask & IN_OPEN) {
                            NOTIFY_CLIENT(IN_OPEN);
                        }
                        if (in_event->mask & IN_CLOSE_WRITE) {
                            NOTIFY_CLIENT(IN_CLOSE_WRITE);
                        }
                        if (in_event->mask & IN_CLOSE_NOWRITE) {
                            NOTIFY_CLIENT(IN_CLOSE_NOWRITE);
                        }
                        if (in_event->mask & IN_MODIFY) {
                            NOTIFY_CLIENT(IN_MODIFY);
                        }
                        if (in_event->mask & IN_DELETE_SELF) {
                            NOTIFY_CLIENT(IN_DELETE_SELF);
                        }
                        if (in_event->mask & IN_CREATE) {
                            NOTIFY_CLIENT(IN_CREATE);
                        }
                        if (stat_error == NULL) {
                            SEND_TO_CLIENT(
                                str, "Current size is %0.3f%s\n",
                                format_readable_size(filestat.st_size),

                                format_readable_prefix(filestat.st_size));
                        }
                    }
                }
            }
        }
    }

usage:
    printf(USAGE, argv[0]);
error:
    rc = 1;
    goto exit;
exit:
    if (inotify_buf != NULL)
        free(inotify_buf);
    if (sock) {
        shutdown(sock, SHUT_RDWR);
        close(sock);
    }
    if (conf.sock_path != NULL)
        unlink(bdatae(conf.sock_path, ""));
    bdestroy(conf.sock_path);
    bdestroy(conf.file_path);
    return rc;
}
