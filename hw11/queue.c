#include <bstring/bstring/bstrlib.h>
#include <dbg.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define PATHNAME  "queue.c"
#define PROJECTID 1

struct Job {
    int length;
};

struct JobControl {
    const void *const job_queue;
    int (*const get_job)(void *job_queue, struct Job *j);
};

struct JobQueued {
    long type;
    char job[sizeof(struct Job)];
};

int get_queue_job(void *job_queue, struct Job *j) {}

int worker_f(const struct JobControl *const jc) {
    int rc = 0;
exit:
    return rc;
error:
    goto exit;
}

int main() {
    int rc = 0;
    key_t qk = ftok(PATHNAME, PROJECTID);
    int queue = 0;
    pthread_t worker = {0};
    CHECK(qk >= 0, "Couldn't create queue identifyer");
    CHECK((queue = msgget(qk, 0666 | IPC_CREAT)) >= 0,
          "Couldn't create message queue.");
    struct JobControl jc = {.job_queue = &queue, .get_job = get_queue_job};
    CHECK(pthread_create(&worker, NULL, (void *(*)(void *))worker_f, &jc) == 0,
          "Couldnt create worker");
    return 0;

error:
    rc = 1;
exit:
    return rc;
}
