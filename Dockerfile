FROM ubuntu:24.04

# gcc/make
RUN apt-get update && apt-get install -y build-essential gdb && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY Makefile .
COPY src/ ./src

RUN make all
RUN make debug

CMD ["./lc3"]
