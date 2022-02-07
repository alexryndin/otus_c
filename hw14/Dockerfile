FROM ubuntu
RUN apt update
RUN apt-get install -y git gcc sqlite3 libsqlite3-dev make
RUN git clone --recursive https://github.com/alexryndin/otus_c.git  /build
RUN apt-get install -y libpq-dev
RUN cd /build/hw14 && \
    make
WORKDIR /build/hw14
CMD /build/hw14/tests.sh
