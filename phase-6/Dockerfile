FROM gcc:latest

WORKDIR /app
COPY . .

RUN make clean && make server

EXPOSE 9001
CMD ["./server"]

