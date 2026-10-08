FROM gcc:latest

RUN apt-get update && apt-get install -y --no-install-recommends libssl-dev && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN make clean && make

EXPOSE 9001
CMD ["./server"]
