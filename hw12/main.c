#include <bstring/bstring/bstrlib.h>
#include "dbg.h"
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#define USAGE "Usage: %s message [font]\n\
where font is one of the following:\n\
  3-d                3x5                5lineoblique       acrobatic\n\
  alligator          alligator2         alphabet           avatar\n\
  banner             banner3            banner3-D          banner4\n\
  barbwire           basic              bell               big\n\
  bigchief           binary             block              bubble\n\
  bulbhead           calgphy2           caligraphy         catwalk\n\
  chunky             coinstak           colossal           computer\n\
  contessa           contrast           cosmic             cosmike\n\
  cricket            cursive            cyberlarge         cybermedium\n\
  cybersmall         diamond            digital            doh\n\
  doom               dotmatrix          drpepper           eftichess\n\
  eftifont           eftipiti           eftirobot          eftitalic\n\
  eftiwall           eftiwater          epic               fender\n\
  fourtops           fuzzy              goofy              gothic\n\
  graffiti           hollywood          invita             isometric1\n\
  isometric2         isometric3         isometric4         italic\n\
  ivrit              jazmine            jerusalem          katakana\n\
  kban               larry3d            lcd                lean\n\
  letters            linux              lockergnome        madrid\n\
  marquee            maxfour            mike               mini\n\
  mirror             mnemonic           morse              moscow\n\
  nancyj             nancyj-fancy       nancyj-underlined  nipples\n\
  ntgreek            o8                 ogre               pawp\n\
  peaks              pebbles            pepper             poison\n\
  puffy              pyramid            rectangles         relief\n\
  relief2            rev                roman              rot13\n\
  rounded            rowancap           rozzo              runic\n\
  runyc              sblood             script             serifcap\n\
  shadow             short              slant              slide\n\
  slscript           small              smisome1           smkeyboard\n\
  smscript           smshadow           smslant            smtengwar\n\
  speed              stampatello        standard           starwars\n\
  stellar            stop               straight           tanja\n\
  tengwar            term               thick              thin\n\
  threepoint         ticks              ticksslant         tinker-toy\n\
  tombstone          trek               tsalagi            twopoint\n\
  univers            usaflag            wavy               weird\n"

int nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    int rc = 0;
    CHECK(flags >= 0, "Invalid flags on nonblock.");

    rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    CHECK(rc == 0, "Can't set nonblocking.");

    return 0;

error:
    return -1;
}

// \r + \n + '.' = 3
#define FULLPROMPTLENGTH 3

#define CHECK_TIMEOUT(timeout)  \
    do {                        \
        if ((timeout) < 0) {    \
            LOG_ERR("Timeout"); \
            goto error;         \
        }                       \
    } while (0)


int main(int argc, char *argv[]) {
    int rc = 0;
    int _read = 0;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct addrinfo *addresses, *address = {0};
    struct addrinfo hints = {0};
    int bufsize = 4096;
    int timeout = 10;
    char *buf = NULL;
    char *font = NULL;
    bstring msg = NULL;
    bstring _output = NULL;
    bstring str_addr = bfromcstralloc(16, "");
    bstring res = NULL;
    if (argc != 2 && argc != 3) {
        goto usage;
    }
    if (argc == 3) {
        font = argv[2];
    }
    buf = calloc(bufsize, 1);
    CHECK_MEM(buf);
    CHECK(sock > 0, "Couldn't create socket.");
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    rc = getaddrinfo("telehack.com", "23", &hints, &addresses);
    CHECK(rc == 0, "Couldn't get addresses, error -- %s.", gai_strerror(rc));
    for (address = addresses; address != NULL; address = address->ai_next) {
        uint8_t ch;
        in_port_t port;
        for (int i = 0; i < 4; i++) {
            ch = ((uint8_t *)(&(
                ((struct sockaddr_in *)address->ai_addr)->sin_addr)))[i];
            bformata(str_addr, "%hu.", ch);
        }
        btrunc(str_addr, blength(str_addr) - 1);
        port = htons(((struct sockaddr_in *)address->ai_addr)->sin_port);
        LOG_DEBUG("trying addr %s:%hu", bdata(str_addr), port);
        rc = connect(sock, address->ai_addr, address->ai_addrlen);
        if (rc != 0) {
            LOG_WARN("Couldn't connect to %s (%s:%hu)", argv[1],
                     bdata(str_addr), port);
        } else {
            break;
        }
    }
    CHECK(rc == 0, "Couldn't connect");
    freeaddrinfo(addresses);
    addresses = NULL;
    errno = 0;
    nonblock(sock);
    res = bfromcstr("");

    if (font != NULL) {
        msg = bformat("figlet /%s %s\r\n", font, argv[1]);
    } else {
        msg = bformat("figlet %s\r\n", argv[1]);
    }

    CHECK(msg != NULL, "Message formating failed.");
    struct tagbstring _msg;
    bmid2tbstr(_msg, msg, 0, blength(msg));

    struct tagbstring prompt = bsStatic("\r\n.");

    while (timeout > 0) {
        if ((_read = read(sock, buf, bufsize - 1)) < 0 && errno == EAGAIN) {
            errno = 0;
            sleep(1);
            timeout--;
            continue;
        }
        CHECK(errno == 0, "Couldn't read data from the server.");
        CHECK(bcatblk(res, buf, _read) == BSTR_OK, "Could't append to str");
        if (binstr(res, 0, &prompt) != BSTR_ERR) {
            break;
        }
    }
    CHECK_TIMEOUT(timeout);
    btrunc(res, 0);
    errno = 0;
    int to_write = blength(&_msg), wrote = 0;
    while (to_write > 0 && timeout > 0) {
        if ((wrote = write(sock, bdata(msg), to_write)) < 0 &&
            errno == EAGAIN) {
            errno = 0;
            sleep(1);
            timeout--;
            continue;
        }
        CHECK(errno == 0, "Couldn't send data to the server.");
        to_write -= wrote;
    }
    CHECK_TIMEOUT(timeout);
    struct tagbstring output;
    int cursor = 0;
    errno = 0;
    while (timeout > 0) {
        if ((_read = read(sock, buf, bufsize - 1)) < 0 && errno == EAGAIN) {
            errno = 0;
            sleep(1);
            timeout--;
        } else {
            CHECK(errno == 0, "Couldn't read data from the server.");
            CHECK(bcatblk(res, buf, _read) == BSTR_OK, "Could't append to str");
            if ((cursor = binstr(res, 0, &prompt)) == BSTR_ERR) {
                cursor = 0;
            } else
                break;
        }
    }
    CHECK_TIMEOUT(timeout);

    // strip prompt
    bmid2tbstr(output, res, blength(msg),
               blength(res) - blength(msg) - FULLPROMPTLENGTH);

    _output = bfromcstralloc(blength(&output), "");
    CHECK_MEM(_output);

    // filter '\r'
    for (int i = 0; i < blength(&output); i++) {
        char ch = bdata(&output)[i];
        if (isgraph(ch) || (isspace(ch) && ch != '\r')) {
            bconchar(_output, ch);
        }
    }

    printf("%s\n", bdata(_output));

    rc = 0;
    goto exit;

error:
    rc = 1;
exit:
    if (buf != NULL) {
        free(buf);
    }
    if (res != NULL) {
        bdestroy(res);
    }
    if (_output != NULL) {
        bdestroy(_output);
    }
    if (msg != NULL) {
        bdestroy(msg);
    }
    if (str_addr != NULL) {
        bdestroy(str_addr);
    }
    return rc;
usage:
    printf(USAGE, argv[0]);
    goto error;
}
