FROM gcc

COPY . /usr/src/wrk/
WORKDIR /usr/src/wrk/

RUN \
  make

ENTRYPOINT [ "./wrk" ]
