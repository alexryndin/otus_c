#include <bstring/bstring/bstrlib.h>
#include <dbg.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>

#define USAGE "Usage: %s port num_of_workers\n"

#define MAX_BACKLOG 200
#define MAX_EVENTS  20
#define BUF_SIZE    4096

#define SKIP_UNTIL(cursor, line, ch)  \
    cursor = bstrchr((line), (ch));   \
    if ((cursor) == BSTR_ERR) {       \
        LOG_ERR("Malformed string."); \
        continue;                     \
    }

#define BACKWARD_UNTIL_FROM(cursor, line, ch, delta) \
    cursor = bstrrchrp((line), (ch), (delta));       \
    if ((cursor) == BSTR_ERR) {                      \
        LOG_ERR("Malformed string.");                \
        continue;                                    \
    }

#define SKIP_UNTIL_FROM(cursor, line, ch, delta) \
    cursor = bstrchrp((line), (ch), (delta));    \
    if ((cursor) == BSTR_ERR) {                  \
        LOG_ERR("Malformed string.");            \
        continue;                                \
    }

typedef enum WsStates {
    MS_GOT_HEADER = 1,
    MS_BAD_REQ = 2,
    MS_HEADER_SENT = 4,
    MS_CONTENT_SENT = 8,
    MS_FILE_REQ = 16,
    MS_INTERNAL_ERR = 32,
    MS_PARSED_HEADER = 64,
    MS_FILE_RDY = 128,
    MS_PREPARED_HEADER = 256,
    MS_PREPARED_CONTENT = 512,
    MS_UNKN = 0,
} WsStates;

typedef enum WsMethods {
    M_GET,
    M_UNKN,
} WsMethods;

typedef enum WsErrors {
    WS_OK = 0,
    WS_EAGAIN,
    WS_NOT_FOUND,
    WS_ADENIED,
    WS_EOF,
    WS_ERROR,
    WS_ALREADY_DONE,
} WsErrors;

typedef enum WsProtocol {
    PROTO_HTTP_1_1,
    PROTO_UNKN,
} WsProtocol;

static int ignore_signals(void) {
    sigset_t ss = {0};
    sigaddset(&ss, SIGPIPE);
    CHECK(sigprocmask(SIG_BLOCK, &ss, NULL) == 0, "Coudln't block signals");

    return 0;
error:
    return -1;
}
static int setnonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    int rc = 0;
    CHECK(flags >= 0, "Invalid flags on nonblock.");

    rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    CHECK(rc == 0, "Can't set nonblocking.");

    return 0;

error:
    return -1;
}

static struct tagbstring header_sep = bsStatic("\r\n\r\n");
static struct tagbstring _separator = bsStatic("\r\n");

static struct tagbstring _GET = bsStatic("GET");

static struct tagbstring _HTTP_1_1 = bsStatic("HTTP/1.1");

static struct tagbstring _HTTP_200_OK = bsStatic("HTTP/1.1 200 OK\r\n");
static struct tagbstring _HTTP_404_NOT_FOUND =
    bsStatic("HTTP/1.1 404 Not Found\r\n");
static struct tagbstring _HTTP_400_BAD_REQ =
    bsStatic("HTTP/1.1 400 Bad Request\r\n");
static struct tagbstring _HTTP_500_INTERNALL_ERR =
    bsStatic("HTTP/1.1 500 Internal Server Error\r\n");
static struct tagbstring _HTTP_403_ACCESS_DENIED =
    bsStatic("HTTP/1.1 403 Access Denied\r\n");

struct WorkerControl {
    int listen_sock;
    int worker_id;
};

struct ClientHandle {
    int sock;
    int fd;
    int cursor; // multi purpose input string pointer
    int method;
    int protocol;
    int state;
    int ec;
    int64_t content_sent;
    int64_t content_size;
    int64_t header_sent;
    bstring path;
    bstring input;
    bstring output_header;
    bstring output_content;
};

static int ClientHandle_destroy(struct ClientHandle *ch) {
    if (ch != NULL) {
        if (ch->input != NULL) {
            bdestroy(ch->input);
        }
        if (ch->path != NULL) {
            bdestroy(ch->path);
        }
        if (ch->output_header != NULL) {
            bdestroy(ch->output_header);
        }
        if (ch->output_content != NULL) {
            bdestroy(ch->output_content);
        }
        if (ch->fd >= 0) {
            close(ch->fd);
        }
        if (ch->sock >= 0) {
            close(ch->sock);
        }
        free(ch);
    }
    return 0;
}

static struct ClientHandle *ClientHandle_create(void) {
    struct ClientHandle *ch = NULL;
    ch = calloc(1, sizeof(struct ClientHandle));
    CHECK_MEM(ch);
    ch->fd = -1;
    ch->sock = -1;
    ch->path = bfromcstr("");
    ch->input = bfromcstr("");
    ch->output_header = bfromcstr("");
    ch->output_content = bfromcstr("");
    CHECK(ch->path != NULL, "Couldn't allocate string");
    return ch;
error:
    if (ch != NULL) {
        free(ch);
    }

    return NULL;
}

static int ws_is_full_header(struct ClientHandle *ch) {
    CHECK(ch != NULL, "Null handle");
    int new_cursor = 0;

    new_cursor = binstr(ch->input, 0, &header_sep);
    if (new_cursor != BSTR_ERR && new_cursor > 0) {
        ch->cursor = new_cursor;
        return 1;
    }
    return 0;

error:
    return -1;
}
static int ws_prepare_header(struct ClientHandle *ch) {
    CHECK(ch != NULL, "Null handle");

    bstring l = NULL;
    switch (ch->ec) {
    case 200:
        l = &_HTTP_200_OK;
        break;
    case 400:
        l = &_HTTP_400_BAD_REQ;
        break;
    case 404:
        l = &_HTTP_404_NOT_FOUND;
        break;
    case 403:
        l = &_HTTP_403_ACCESS_DENIED;
        break;
    case 500:
    default:
        l = &_HTTP_500_INTERNALL_ERR;
    }

    off_t content_size = ch->content_size;

    CHECK(
        bconcat(ch->output_header, l) == BSTR_OK, "Couldn't append to header");

    CHECK(
        bformata(ch->output_header, "Content-Type: text/html\r\n") == BSTR_OK,
        "Couldn't append to header");

    CHECK(
        bformata(ch->output_header, "Content-Length: %ld\r\n", content_size) ==
            BSTR_OK,
        "Couldn't append to header");

    CHECK(
        bconcat(ch->output_header, &_separator) == BSTR_OK,
        "Couldn't append to header");

    return WS_OK;
error:
    return WS_ERROR;
}

static int ws_send_str(struct ClientHandle *ch, bstring str, int64_t *sent) {
    CHECK(ch != NULL, "Null handle");
    CHECK(str != NULL && bdata(str) != NULL, "Null str");
    CHECK(sent != NULL, "Null sent bytes pointer");
    if (*sent >= blength(str)) {
        LOG_WARN("Data was sent already");
        return WS_ALREADY_DONE;
    }
    int rc = 0;

    for (;;) {
        rc = write(ch->sock, bdata(str) + *sent, blength(str) - *sent);

        if (rc == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                errno = 0;
                return WS_EAGAIN;
            }
            return WS_ERROR;
        }

        *sent += rc;
        if (*sent >= blength(str)) {
            return WS_OK;
        }
    }

error:
    return WS_ERROR;
}

static int ws_send_file(struct ClientHandle *ch) {
    CHECK(ch != NULL, "Null handle");
    if (ch->content_sent >= ch->content_size) {
        LOG_WARN("Content was sent already");
        return WS_ALREADY_DONE;
    }
    ssize_t rc = 0;

    for (;;) {
        rc = sendfile(
            ch->sock,
            ch->fd,
            &ch->content_sent,
            ch->content_size - ch->content_sent);

        if (rc == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                errno = 0;
                return WS_EAGAIN;
            }
            return WS_ERROR;
        }

        if (ch->content_sent >= ch->content_size) {
            return WS_OK;
        }
    }

error:
    return WS_ERROR;
}
static WsErrors ws_try_send(struct ClientHandle *ch) {
    int rc = 0;
    CHECKRC(ch != NULL, WS_ERROR, "Null handle");

    if (!(ch->state & MS_FILE_REQ) && !(ch->state & MS_PREPARED_CONTENT)) {
        CHECK(
            bformata(ch->output_content, "%lu\r\n", ch->ec) == BSTR_OK,
            "Couldn't prepare content");
        ch->state = ch->state | MS_PREPARED_CONTENT;
        ch->content_size = blength(ch->output_content);
    }
    if (!(ch->state & MS_PREPARED_HEADER)) {
        CHECK(ws_prepare_header(ch) == WS_OK, "Couldn't prepare header");
        ch->state = ch->state | MS_PREPARED_HEADER;
    }
    if (!(ch->state & MS_HEADER_SENT)) {
        switch (rc = ws_send_str(ch, ch->output_header, &ch->header_sent)) {
        case WS_OK:
            ch->state = ch->state | MS_HEADER_SENT;
            break;
        default:
            return rc;
        }
    }

    if ((ch->state & MS_FILE_REQ)) {
        return ws_send_file(ch);
    }
    if ((ch->state & MS_BAD_REQ)) {
        return ws_send_str(ch, ch->output_content, &ch->content_sent);
    }
    LOG_ERR("Wrong server state");

    rc = WS_ERROR;
error:
    return rc;
}
#define WS_CLOSE_CLIENT(ch, epollfd)                                \
    do {                                                            \
        LOG_DEBUG("Closing client");                                \
        CHECK(                                                      \
            epoll_ctl(epollfd, EPOLL_CTL_DEL, ch->sock, NULL) == 0, \
            "Couldn't delete client sock from epoll");              \
        ClientHandle_destroy(ch);                                   \
    } while (0)

#define WS_TRY_SEND_CLOSE(ch, epollfd)                                  \
    {                                                                   \
        switch (ws_try_send(ch)) {                                      \
        case WS_EAGAIN:                                                 \
            ev.data = e_ptr->data;                                      \
            ev.events = EPOLLIN | EPOLLET | EPOLLOUT;                   \
            CHECK(                                                      \
                epoll_ctl(epollfd, EPOLL_CTL_MOD, ch->sock, &ev) == 0,  \
                "Couldn't modify epoll to add EPOLLOUT for client "     \
                "sock");                                                \
            break;                                                      \
        case WS_OK:                                                     \
            ev.data = e_ptr->data;                                      \
            ev.events = EPOLLIN | EPOLLET;                              \
            CHECK(                                                      \
                epoll_ctl(epollfd, EPOLL_CTL_MOD, ch->sock, &ev) == 0,  \
                "Couldn't modify epoll to remove EPOLLOUT for "         \
                "client "                                               \
                "sock");                                                \
            break;                                                      \
        default:                                                        \
            LOG_ERR("Error communicating with client");                 \
            CHECK(                                                      \
                epoll_ctl(epollfd, EPOLL_CTL_DEL, ch->sock, NULL) == 0, \
                "Couldn't delete client sock from epoll");              \
            ClientHandle_destroy(ch);                                   \
            continue;                                                   \
        }                                                               \
    }

static int ws_read(struct ClientHandle *ch, char *buf) {
    CHECK(ch != NULL, "Null handle");
    CHECK(buf != NULL, "Null buffer");
    int rc = 0;

    for (;;) {
        rc = read(ch->sock, buf, BUF_SIZE - 1);
        LOG_DEBUG("read = %d", rc);
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            LOG_DEBUG("Got wouldblock on sock %d", ch->sock);
            errno = 0;
            return WS_EAGAIN;
        }
        if (rc == 0) {
            return WS_EOF;
        }
        CHECK(rc > 0, "Couldn't read data from socket %d", ch->sock);
        buf[rc] = '\0';
        LOG_DEBUG("read %d bytes", rc);
        CHECK(
            bcatcstr(ch->input, buf) == BSTR_OK,
            "Couldn't concatenate buffer to string");
    }

error:
    return WS_ERROR;
}

static WsErrors ws_open_file(struct ClientHandle *ch) {
    struct stat st;
    int rc = 0;
    int fd = 0;
    CHECK(ch != NULL, "Null handle");
    CHECK(
        bdata(ch->path) != NULL && blength(ch->path) > 0 &&
            bdata(ch->path)[0] == '/',
        "HTTP path must exist and start with /");

    struct tagbstring filepath = {0};
    if (blength(ch->path) == 1 && bdata(ch->path)[0] == '/') {
        filepath = (struct tagbstring)bsStatic("index.html");
    } else {
        bmid2tbstr(filepath, ch->path, 1, blength(ch->path));
    }

    rc = fstatat(
        AT_FDCWD, bdata(&filepath) != NULL ? bdata(&filepath) : "", &st, 0);

    if (rc != 0) {
        switch (errno) {
        case EACCES:
            rc = WS_ADENIED;
            errno = 0;
            goto exit;
        case ENOENT:
            rc = WS_NOT_FOUND;
            errno = 0;
            goto exit;
        default:
            rc = WS_ERROR;
            goto exit;
        }
    }
    CHECKRC(
        (st.st_mode & S_IFMT) == S_IFREG, WS_NOT_FOUND, "Not a regular file");

    CHECK(
        (fd = openat(
             AT_FDCWD,
             bdata(&filepath) != NULL ? bdata(&filepath) : "",
             O_RDONLY,
             0)) > 0,
        "Couldn't open file %s",
        bdata(&filepath));
    ch->fd = fd;
    // FIXME: Can filesize be negative?
    ch->content_size = st.st_size;
    rc = WS_OK;
    LOG_DEBUG("Opened file %s", bdata(&filepath));
    goto exit;

exit:
    return rc;
error:
    rc = WS_ERROR;
    goto exit;
}

static int ws_parse_header(struct ClientHandle *ch) {
    CHECK(ch != NULL, "Null handle");
    struct tagbstring tline, ttmp;
    int new_cursor = 0;
    int delta = 0;
    new_cursor = binstr(ch->input, 0, &_separator);
    CHECK(new_cursor > 0, "Couldn't fetch line from header.");
    bmid2tbstr(tline, ch->input, 0, new_cursor + 2);

    new_cursor = bstrchr(&tline, ' ');
    CHECK(new_cursor > 0, "Couldn't fetch method from header.");
    bmid2tbstr(ttmp, &tline, 0, new_cursor);

    // ttmp is our method now
    if (!bstrcmp(&ttmp, &_GET)) {
        ch->method = M_GET;
    } else {
        ch->method = M_UNKN;
    }

    new_cursor++;
    delta = bstrchrp(&tline, ' ', new_cursor);
    CHECK(delta > new_cursor, "Couldn't fetch path from header.");
    CHECK(
        bassignmidstr(ch->path, &tline, new_cursor, delta - new_cursor),
        "Couldn't extract path from header");

    new_cursor = delta + 1;
    delta = bstrchrp(&tline, '\r', new_cursor);
    CHECK(delta > new_cursor, "Couldn't fetch protocol from header.");
    bmid2tbstr(ttmp, &tline, new_cursor, delta - new_cursor);

    // ttmp is our protocol now
    if (!bstrcmp(&ttmp, &_HTTP_1_1)) {
        ch->protocol = PROTO_HTTP_1_1;
    } else {
        ch->protocol = PROTO_UNKN;
    }

    return 0;

error:
    return -1;
}

static void *worker(struct WorkerControl *wc) {
    int nfds = 0;
    int epollfd = 0;
    int listen_sock = 0;
    int conn_sock = 0;
    int worker_id = 0;
    int rc = 0;
    struct epoll_event ev, events[MAX_EVENTS];
    struct ClientHandle *ch = NULL;
    struct epoll_event const *e_ptr = NULL;
    char *buf = NULL;

    CHECK(ignore_signals() == 0, "Coudln't block SIGPIPE");

    buf = malloc(BUF_SIZE);
    CHECK_MEM(buf);

    CHECK(wc != NULL, "Null WorkerControl");
    listen_sock = wc->listen_sock;
    worker_id = wc->worker_id;

    CHECK((epollfd = epoll_create1(0)) >= 0, "Couldn't create epoll socket");
    ev.events = EPOLLIN | EPOLLET | EPOLLEXCLUSIVE;
    ev.data.fd = listen_sock;

    CHECK(
        epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_sock, &ev) == 0,
        "Couldn't add listening sock to epoll interests list");
    LOG_DEBUG("Worker %d started", worker_id);
    for (;;) {
        LOG_DEBUG("Worker %d want epoll_wait", worker_id);
        CHECK(
            (nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1)) >= 0,
            "epoll_wait failed");
        LOG_DEBUG("Worker %d woke up to process %d events", worker_id, nfds);
        for (int n = 0; n < nfds; n++) {

            e_ptr = &events[n];

            if (e_ptr->data.fd == listen_sock) {

                for (;;) {
                    conn_sock = accept(listen_sock, NULL, NULL);
                    if (conn_sock == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            errno = 0;
                            break;
                        }
                    }
                    CHECK(
                        conn_sock >= 0,
                        "Worker %d: Couldn't accept connection",
                        worker_id);
                    ch = ClientHandle_create();
                    CHECK(ch != NULL, "Couldn't allocate Client Handle");
                    ch->sock = conn_sock;
                    CHECK(
                        setnonblocking(conn_sock) == 0,
                        "Couldn't make socket nonblocking");
                    ev.events = EPOLLIN | EPOLLET;
                    ev.data.ptr = ch;
                    CHECK(
                        epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_sock, &ev) == 0,
                        "Couldn't add connection sock to epoll interests list");
                    LOG_DEBUG("Worker %d accepted %d", worker_id, conn_sock);
                }
            } else {
                ch = (struct ClientHandle *)e_ptr->data.ptr;
                if (e_ptr->events & EPOLLIN) {
                    LOG_DEBUG("Got EPOLLIN for sock %d", ch->sock);
                    switch (ws_read(ch, buf)) {

                    case WS_EOF:
                        WS_CLOSE_CLIENT(ch, epollfd);
                        continue;

                    case WS_EAGAIN:
                        break;
                    default:
                        WS_CLOSE_CLIENT(ch, epollfd);
                        continue;
                    }

                    LOG_DEBUG(
                        "Worker %d got str %s from socket %d",
                        worker_id,
                        bdata(ch->input),
                        ch->sock);

                    if (!(ch->state & MS_GOT_HEADER)) {
                        if (ws_is_full_header(ch)) {
                            ch->state = ch->state | MS_GOT_HEADER;
                        }
                    }

                    if ((ch->state & MS_GOT_HEADER) &&
                        !(ch->state & MS_PARSED_HEADER)) {
                        rc = ws_parse_header(ch);

                        if (rc) {
                            ch->ec = 500;
                            ch->state = ch->state | MS_BAD_REQ;
                        } else {
                            switch (ws_open_file(ch)) {
                            case WS_OK:
                                ch->ec = 200;
                                ch->state = ch->state | MS_FILE_REQ;
                                break;
                            case WS_NOT_FOUND:
                                ch->state = ch->state | MS_BAD_REQ;
                                ch->ec = 404;
                                break;
                            case WS_ADENIED:
                                ch->state = ch->state | MS_BAD_REQ;
                                ch->ec = 403;
                                break;
                            default:
                                ch->state = ch->state | MS_BAD_REQ;
                                ch->ec = 500;
                                break;
                            }
                        }
                        ch->state = ch->state | MS_PARSED_HEADER;
                    }

                    if ((ch->state & MS_FILE_REQ) || (ch->state & MS_BAD_REQ)) {
                        WS_TRY_SEND_CLOSE(ch, epollfd);
                    }
                }
                if (e_ptr->events & EPOLLOUT) {
                    LOG_DEBUG("Got EPOLLOUT for sock %d", ch->sock);
                    if ((ch->state & MS_FILE_REQ) || (ch->state & MS_BAD_REQ)) {
                        WS_TRY_SEND_CLOSE(ch, epollfd);
                    }
                }
            }
        }
    }
error:
    if (ch != NULL) {
        free(ch);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    int rc = 1;
    int listen_sock;
    int port = 0;
    int number_of_workers = 0;
    pthread_t *workers = NULL;
    struct WorkerControl wc = {0};

    if (argc != 3) {
        goto usage;
    }

    CHECK(ignore_signals() == 0, "Coudln't block SIGPIPE");

    port = atoi(argv[1]);
    number_of_workers = atoi(argv[2]);
    CHECK(port > 0 && port < 65536, "Port out of range");
    CHECK(
        number_of_workers > 0 && number_of_workers < 32,
        "Number of wrokers out of range");
    workers = malloc(sizeof(pthread_t) * number_of_workers);
    CHECK_MEM(workers);

    CHECK(
        (listen_sock = socket(AF_INET, SOCK_STREAM, 0)) >= 0,
        "Couldn't create listen socket");
    struct sockaddr_in iaddr = {
        .sin_family = AF_INET,
        .sin_addr = (struct in_addr){.s_addr = INADDR_ANY},
        .sin_port = (in_port_t)htons(port)};

    CHECK(
        setsockopt(
            listen_sock, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) == 0,
        "Couldn't set SO_REUSEADDR on listen "
        "socket");
    CHECK(
        bind(listen_sock, (struct sockaddr *)&iaddr, sizeof(iaddr)) == 0,
        "Couldn't bind a socket, port: %d.",
        port);

    CHECK(listen(listen_sock, MAX_BACKLOG) == 0, "Couldn't listen.");
    CHECK(
        setnonblocking(listen_sock) == 0,
        "Couldn't make listen socket nonblocking");

    wc.listen_sock = listen_sock;

    for (int i = 0; i < number_of_workers; i++) {
        struct WorkerControl *_wc = calloc(1, sizeof(struct WorkerControl));
        CHECK_MEM(_wc);
        *_wc = wc;
        _wc->worker_id = i;

        CHECK(
            pthread_create(&workers[i], NULL, (void *(*)(void *))worker, _wc) ==
                0,
            "Couldn't create worker %d",
            i);
    }
    for (size_t i = 0; i < (size_t)number_of_workers; i++) {
        CHECK(
            pthread_join(workers[i], NULL) == 0, "Couldn't join worker %zu", i);
    }

error:
    rc = 1;
exit:
    return rc;
usage:
    printf(USAGE, argv[0]);
    goto error;
}
