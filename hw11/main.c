#include <bstring/bstring/bstrlib.h>
#include <chan.h>
#include <darray.h>
#include <dbg.h>
#include <fcntl.h>
#include <glob.h>
#include <hashmap_utils.h>
#include <open_hashmap.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

extern const uint32_t CRCTable[256];

#define USAGE "Usage: %s num_of_workers block_size [glob]\n"

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

static int free_node(OpenHashmapNode *node, void *userdata) {
    // suppress warning;
    (void)userdata;


    bdestroy(node->key);
    free(node->data);
    return 0;
}

static int sort_vals_dec(const OpenHashmapNode **a, const OpenHashmapNode **b) {
    char is_a_vacant = 0, is_b_vacant = 0;
    if (*a == NULL) {
        is_a_vacant = 1;
    } else {
        is_a_vacant = IS_VACANT(*a);
    }
    if (*b == NULL) {
        is_b_vacant = 1;
    } else {
        is_b_vacant = IS_VACANT(*b);
    }
    if (is_a_vacant && is_b_vacant)
        return 0;
    if (is_a_vacant && !is_b_vacant)
        return 1;
    if (!is_a_vacant && is_b_vacant)
        return -1;
    return uint64_cmp_dec((*a)->data, (*b)->data);
}

struct PathJob {
    bstring path;
    off_t filesize;
};

struct JobControl {
    chan_t *new_jobs;
    pthread_mutex_t stat_m;
    size_t block_size;
    size_t page_size;
    int lines;
};

struct Job {
    bstring filepath;
    off_t filesize;
    long offset;
    int length;
};

struct JobResult {
    uint64_t sum_bytes_send;
    OpenHashmap *sum_by_url_bytes;
    OpenHashmap *count_by_referer;
};

int traverse_print_map(OpenHashmapNode *node, void *userdata) {
    // suppress warnings
    (void)userdata;

    printf("KEY: %s, VALUE: %lu\n", bdata((bstring)node->key),
           *(uint64_t *)node->data);
    return 0;
}

int traverse_combine(OpenHashmapNode *node, struct JobResult *userdata) {
    uint64_t *tmp;
    bstring key;
    CHECK(userdata != NULL, "Callback provided null JobResults struct.");
    tmp = OpenHashmap_get((OpenHashmap *)userdata, node->key);
    if (tmp == NULL) {
        CHECK((key = bstrcpy(node->key)) != NULL,
              "Worker: Could't copy key for the map.");
        tmp = calloc(1, sizeof(uint64_t));
        (*tmp) += *(uint64_t *)node->data;
        CHECK_MEM(tmp);
        CHECK(OpenHashmap_set((OpenHashmap *)userdata, key, tmp) == 0,
              "Worker: Couldn't set value to map");
    } else {
        (*tmp) += *(uint64_t *)node->data;
    }
    return 0;
error:
    return -1;
}

DArray *get_log_paths(char *gl) {
    glob_t globbuf = {0};
    struct stat filestat;
    struct PathJob *path_task;
    int rc = 0;

    DArray *pathlist = DArray_create(10);
    CHECK(pathlist != 0, "Couldn't create list of log paths.");

    if (gl != NULL) {
        rc = glob(gl, 0, NULL, &globbuf);
        CHECK(rc != GLOB_ABORTED && rc != GLOB_NOSPACE, "glob failed.");
    } else {
        rc = glob("./*.log", 0, NULL, &globbuf);
        CHECK(rc != GLOB_ABORTED && rc != GLOB_NOSPACE, "glob failed.");
        rc = glob("./*.log.[0-9]", GLOB_APPEND, NULL, &globbuf);
        CHECK(rc != GLOB_ABORTED && rc != GLOB_NOSPACE, "glob failed.");
    }

    LOG_DEBUG("Glob returned %lu paths", globbuf.gl_pathc);

    for (size_t i = 0; i < globbuf.gl_pathc; i++) {
        CHECK(stat(globbuf.gl_pathv[i], &filestat) == 0,
              "Couldn't get filestat for file %s.", globbuf.gl_pathv[i]);
        if (!S_ISREG(filestat.st_mode))
            continue;
        path_task = malloc(sizeof(struct PathJob));
        CHECK_MEM(path_task);
        path_task->filesize = filestat.st_size;
        path_task->path = bfromcstr(globbuf.gl_pathv[i]);
        DArray_push(pathlist, path_task);
        CHECK(path_task->path != NULL, "Couldn't make string from path.");
    }

    globfree(&globbuf);
    return pathlist;
error:
    globfree(&globbuf);
    if ((pathlist) != NULL) {
        DArray_clear_destroy(pathlist);
    }
    return NULL;
}

void *worker(struct JobControl *jc) {
    struct Job *j = NULL;
    struct JobResult *jr = NULL;
    bstring cur_path = NULL;
    char *buf = NULL;
    FILE *f = NULL;
    size_t buf_size = 0;
    int read = 0;
    int cursor = 0;
    int delta = 0;
    struct tagbstring tbulk = {0}, tline = {0}, turl = {0};
    bstring bulk = &tbulk, line = &tline, url = &turl;
    struct tagbstring treferer = {0}, tbytes_send_str = {0};
    bstring referer = &treferer, bytes_send_str = &tbytes_send_str;
    bstring key = NULL;
    uint64_t *tmp;
    uint64_t num_of_bytes = 0;

    CHECK(jc != NULL, "Worker: null JobControl handle.");
    CHECK(jc->block_size > 0, "Worker: wrong block size.");
    CHECK(jc->page_size > 4095, "Worker: wrong page size.");

    CHECK_MEM((jr = calloc(1, sizeof(struct JobResult))));
    CHECK((jr->count_by_referer = OpenHashmap_create((Hashmap_compare)str_cmp,
                                                     (Hashmap_hash)str_hash)),
          "Worker: couldn't create hashmap.");
    CHECK((jr->sum_by_url_bytes = OpenHashmap_create((Hashmap_compare)str_cmp,
                                                     (Hashmap_hash)str_hash)),
          "Worker: couldn't create hashmap.");

    LOG_DEBUG("Worker started.");

    buf = malloc(jc->block_size + jc->page_size);
    CHECK_MEM(buf);
    buf_size = jc->block_size + jc->page_size;

    while (1) {
        if (j != NULL) {
            free(j);
            j = NULL;
        }
        if (chan_recv(jc->new_jobs, (void **)&j) != 0) {
            break;
        }

        if (j->length < 0) {
            LOG_ERR("Worker: wrong block length.");
            continue;
        }
        CHECK((size_t)j->length < buf_size,
              "Worker: block length > buffer size.");

        LOG_DEBUG("Worker got new job, size %ld, offset %ld, length %d path %s",
                  j->filesize, j->offset, j->length, bdata(j->filepath));

        // Check if current filepath is that we need to process
        if (cur_path == NULL || bstrcmp(cur_path, j->filepath)) {
            cur_path = j->filepath;
            LOG_DEBUG("cur_path = %s, j->filepath = %s, !bstrcmp(cur_path, "
                      "j->filepath) = %d",
                      bdata(cur_path), bdata(j->filepath),
                      !bstrcmp(cur_path, j->filepath));
            if (f != NULL) {
                fclose(f);
                f = NULL;
            }
            CHECK((f = fopen(bdata(j->filepath), "r")) != NULL,
                  "Worker couldn't open file %s for reading",
                  bdata(j->filepath));
            LOG_DEBUG("Worker: opened file %s", bdata(j->filepath));
        }
        if (j->offset > 0) {
            CHECK(fseek(f, (j->offset), SEEK_SET) == 0,
                  "Worker couldn't seek file position.");
        }

        LOG_DEBUG("block_size = %zu", jc->block_size);
        CHECK((read = fread(buf, 1, j->length, f)) == j->length ||
                  ferror(f) == 0,
              "Couldn't read data from file %s.", bdata(cur_path));
        LOG_DEBUG("read = %d, feof %d", read, feof(f));

        if (buf[read - 1] != '\0') {
            buf[read - 1] = '\0';
        }
        // reset str pointers
        bulk = &tbulk, line = &tline, url = &turl;
        bytes_send_str = &tbytes_send_str, referer = &treferer;
        blk2tbstr(tbulk, buf, read);
        for (char exit = 0; exit != 1;) {
            //  LOG_DEBUG("For loop started");
            cursor = bstrchr(bulk, '\n');
            if (cursor == BSTR_ERR) {
                line = bulk;
                exit = 1;
            } else {
                bmid2tbstr(tline, bulk, 0, cursor);
                bmid2tbstr(tbulk, bulk, cursor + 1, read);
            }
            if (blength(line) < 2) {
                continue;
            }
            SKIP_UNTIL(cursor, line, '"');
            cursor++;
            SKIP_UNTIL_FROM(delta, line, '"', cursor);
            // we found url boundaries.
            // Now let's try to break it in parts --
            // TYPE URL PROTO
            {
                int _cursor = 0;
                int _delta = 0;
                SKIP_UNTIL_FROM(_cursor, line, ' ', cursor);
                _cursor++;
                BACKWARD_UNTIL_FROM(_delta, line, ' ', delta);
                if (_cursor > cursor && _delta < delta && _cursor < _delta) {
                    // url is ok
                    bmid2tbstr(turl, line, _cursor, _delta - _cursor);
                } else {
                    // request is messed up
                    bmid2tbstr(turl, line, cursor, delta - cursor);
                }
            }
            SKIP_UNTIL_FROM(cursor, line, ' ', delta + 1);
            SKIP_UNTIL_FROM(cursor, line, ' ', cursor + 1);
            cursor++;
            SKIP_UNTIL_FROM(delta, line, ' ', cursor);

            // we found number of bytes send
            bmid2tbstr(tbytes_send_str, line, cursor, delta - cursor);
            SKIP_UNTIL_FROM(cursor, line, '"', delta);
            cursor++;
            SKIP_UNTIL_FROM(delta, line, '"', cursor);

            // we found referer
            bmid2tbstr(treferer, line, cursor, delta - cursor);

            // parse bytes send in dedicated block
            {
                errno = 0;
                char *end;
                const long _num_of_bytes =
                    strtol(bdatae(bytes_send_str, "should not happen"), &end, 10);
                if (bdata(bytes_send_str) == end) {
                    LOG_ERR("Malformed string.");
                    continue;
                }
                const char range_error = errno == ERANGE;
                if (range_error) {
                    LOG_ERR("Malformed string -- range error.");
                    errno = 0;
                }
                num_of_bytes = _num_of_bytes;
            }

            //
            // increase num of bytes send for url
            tmp = OpenHashmap_get(jr->sum_by_url_bytes, url);
            if (tmp == NULL) {
                CHECK((key = bstrcpy(url)) != NULL,
                      "Worker: Could't copy key for the map.");
                tmp = calloc(1, sizeof(uint64_t));
                (*tmp) += num_of_bytes;
                CHECK_MEM(tmp);
                CHECK(OpenHashmap_set(jr->sum_by_url_bytes, key, tmp) == 0,
                      "Worker: Couldn't set value to map");
            } else {
                (*tmp) += num_of_bytes;
            }
            // increase num of referers ecnountered
            tmp = OpenHashmap_get(jr->count_by_referer, referer);
            if (tmp == NULL) {
                CHECK((key = bstrcpy(referer)) != NULL,
                      "Worker: Could't copy key for the map.");
                tmp = calloc(1, sizeof(uint64_t));
                *tmp = 1;
                CHECK_MEM(tmp);
                CHECK(OpenHashmap_set(jr->count_by_referer, key, tmp) == 0,
                      "Worker: Couldn't set value to map");
            } else {
                (*tmp)++;
            }
            jr->sum_bytes_send += num_of_bytes;
        }
    }
    LOG_DEBUG("worker done");

exit:
    if (f != NULL) {
        fclose(f);
    }
    if (j != NULL) {
        free(j);
    }
    if (buf != NULL) {
        free(buf);
    }
    return jr;
error:
    if (jr != NULL) {
        free(jr);
        jr = NULL;
    }
    goto exit;
}

int main(int argc, char *argv[]) {
    int rc = 0;
    int number_of_workers = 0;
    size_t block_size = 0;
    long page_size = sysconf(_SC_PAGE_SIZE);
    DArray *pathlist = NULL;
    struct JobControl jc = {0};
    struct PathJob *pj = NULL;
    struct Job *j;
    struct JobResult *jr;
    struct JobResult jrc = {0};
    pthread_t *workers = NULL;
    FILE *logf = NULL;
    char *gl = NULL;

    if (page_size < 4096) {
        page_size = 4096;
    }

    //
    // bstring test,test2,test3;
    // test = bfromcstr("stroka\ntest\n");
    // test2 = bmidstr(test, 0, bstrchr(test, '\n'));
    // test3 = bmidstr(test, bstrchr(test, '\n')+1, blength(test));
    // printf("%s\n", bdata(test2));
    // printf("%s\n", bdata(test3));
    // exit(0);

    //
    if (argc != 3 && argc != 4)
        goto usage;

    if (argc == 4) {
        gl = argv[3];
    }

    number_of_workers = atoi(argv[1]);
    block_size = atoi(argv[2]);
    CHECK(block_size > 0 && block_size < 2000, "Wrong block size.");
    CHECK(number_of_workers > 0 && number_of_workers <= 64,
          "Wrong number of workers.");

    workers = malloc(sizeof(pthread_t) * number_of_workers);
    CHECK_MEM(workers);

    LOG_DEBUG("page size is %ld", page_size);

    block_size = block_size * 1024 * 1024;
    jc.block_size = block_size;
    jc.page_size = page_size;

    // CHECK((fd = open(argv[1], O_RDONLY)) > 0, "Couldn't open file");
    pathlist = get_log_paths(gl);
    CHECK(pathlist != NULL, "Couldn't get list of paths.");
    if (DArray_len(pathlist) <= 0) {
        printf("No files found.\n");
        rc = 1;
        goto exit;
    }

    CHECK((jc.new_jobs = chan_init(number_of_workers)) != NULL,
          "Couldn't create channel.");
    CHECK((pthread_mutex_init(&jc.stat_m, NULL)) == 0,
          "Couldn't create mutex.");

    //  void *ptr = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, fd, 0);
    //  CHECK_MEM(ptr);
    //  munmap((char *)ptr, page_size);

    //  printf("%x\n", ((char *)ptr)[page_size + 1000000000]);
    for (int i = 0; i < number_of_workers; i++) {
        CHECK(pthread_create(&workers[i], NULL, (void *(*)(void *))worker,
                             &jc) == 0,
              "Couldn't create worker %d", i);
    }

    char ch = 0;
    // save two bytes for \n and for \0
    int expand_limit = page_size - 2;
    int _expand_limit = expand_limit;
    for (size_t i = 0; i < DArray_len(pathlist); i++) {
        pj = (struct PathJob *)DArray_index(pathlist, i);

        logf = fopen(bdata(pj->path), "r");
        for (size_t offset = 0, len = block_size; offset < (size_t)pj->filesize;
             offset += len, len = block_size) {
            _expand_limit = expand_limit;

            // seek to upper block limit and skip until \n
            CHECK(fseek(logf, (offset + len) - 1, SEEK_SET) == 0,
                  "Worker couldn't seek file position.");
            while ((ch = fgetc(logf)) != EOF && ch != '\n') {
                len++;
                CHECK(_expand_limit-- > 0,
                      "Master couldn't find newline separator. Logifile is "
                      "possibly corrupted or not a text file.");
            }
            if (ch == '\n') {
                len++;
            }
            CHECK(ferror(logf) == 0, "Couldn't get offset from file %s.",
                  bdata(pj->path));
            j = malloc(sizeof(struct Job));
            CHECK_MEM(j);
            j->filepath = pj->path;
            LOG_DEBUG("Filesize is %ld, b = %zu, path is %s", pj->filesize,
                      offset, bdata(j->filepath));
            j->filesize = pj->filesize;
            j->offset = offset;
            j->length = len;
            CHECK(chan_send(jc.new_jobs, j) == 0,
                  "Master couldn't send job to workers");
        }
        printf("%s\n",
               bdata(((struct PathJob *)DArray_index(pathlist, i))->path));
    }

    printf("\n");

    chan_close(jc.new_jobs);
    LOG_DEBUG("master send all jobs, joining workers.");

    if (number_of_workers == 1) {
        for (size_t i = 0; i < (size_t)number_of_workers; i++) {
            CHECK(pthread_join(workers[i], (void **)&jr) == 0,
                  "Couldn't join worker %zu", i);
            CHECK(jr != NULL, "Worker didn't return result.");
            jrc.count_by_referer = jr->count_by_referer;
            jrc.sum_by_url_bytes = jr->sum_by_url_bytes;
            jrc.sum_bytes_send = jr->sum_bytes_send;
        }
    } else {
        CHECK((jrc.count_by_referer = OpenHashmap_create(
                   (Hashmap_compare)str_cmp, (Hashmap_hash)str_hash)),
              "Master: couldn't create hashmap.");
        CHECK((jrc.sum_by_url_bytes = OpenHashmap_create(
                   (Hashmap_compare)str_cmp, (Hashmap_hash)str_hash)),
              "Master: couldn't create hashmap.");
        for (size_t i = 0; i < (size_t)number_of_workers; i++) {
            CHECK(pthread_join(workers[i], (void **)&jr) == 0,
                  "Couldn't join worker %zu", i);
            CHECK(jr != NULL, "Worker didn't return result.");
            OpenHashmap_traverse(jr->count_by_referer,
                                 (OpenHashmap_traverse_cb)traverse_combine,
                                 (void *)jrc.count_by_referer);
            OpenHashmap_traverse(jr->count_by_referer,
                                 (OpenHashmap_traverse_cb)free_node, NULL);
            OpenHashmap_traverse(jr->sum_by_url_bytes,
                                 (OpenHashmap_traverse_cb)traverse_combine,
                                 (void *)jrc.sum_by_url_bytes);
            OpenHashmap_traverse(jr->sum_by_url_bytes,
                                 (OpenHashmap_traverse_cb)free_node, NULL);
            jrc.sum_bytes_send += jr->sum_bytes_send;
            LOG_DEBUG("Joined result for worker %zu", i);
        }
        // OpenHashmap_traverse(jrc.count_by_referer, traverse_print_map, NULL);
    }

    qsort(jrc.count_by_referer->buckets->contents,
          jrc.count_by_referer->number_of_buckets, sizeof(void *),
          (int (*)(const void *, const void *))sort_vals_dec);

    qsort(jrc.sum_by_url_bytes->buckets->contents,
          jrc.sum_by_url_bytes->number_of_buckets, sizeof(void *),
          (int (*)(const void *, const void *))sort_vals_dec);

    for (int i = 0; i < 10; i++) {
        OpenHashmapNode *node = jrc.count_by_referer->buckets->contents[i];
        if (node != NULL) {
            printf("Referer: %s, count: %lu\n", bdata((bstring)node->key),
                   *(uint64_t *)node->data);
        } else {
            break;
        }
    }
    printf("\n");
    for (int i = 0; i < 10; i++) {
        OpenHashmapNode *node = jrc.sum_by_url_bytes->buckets->contents[i];
        if (node != NULL) {
            printf("Url: %s, sum bytes: %lu\n", bdata((bstring)node->key),
                   *(uint64_t *)node->data);
        } else {
            break;
        }
    }
    printf("\n");
    printf("Total bytes send = %lu\n", jrc.sum_bytes_send);

    rc = 0;
    goto exit;

error:
    rc = 1;
exit:
    if (pathlist != NULL) {
        for (size_t i = 0; i < DArray_len(pathlist); i++) {
            bdestroy(((struct PathJob *)DArray_index(pathlist, i))->path);
            free(DArray_index(pathlist, i));
        }
        DArray_destroy(pathlist);
    }
    if (workers != NULL) {
        free(workers);
    }
    if (jc.new_jobs != NULL) {
        chan_dispose(jc.new_jobs);
    }
    if (jrc.count_by_referer != NULL) {
        OpenHashmap_traverse(jrc.count_by_referer,
                             (OpenHashmap_traverse_cb)free_node, NULL);
    }
    if (jrc.sum_by_url_bytes != NULL) {
        OpenHashmap_traverse(jrc.sum_by_url_bytes,
                             (OpenHashmap_traverse_cb)free_node, NULL);
    }
    return rc;
usage:
    printf(USAGE, argv[0]);
    goto error;
}
