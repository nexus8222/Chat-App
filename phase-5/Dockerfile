# Use official GCC image as build environment
FROM gcc:latest

# Set working directory
WORKDIR /app

# Copy only necessary files first to leverage Docker cache
COPY Makefile ./
COPY server.c client.c commands.c log.c banlist.c mute.c utils.c admin.c motd.c ./
COPY *.h ./

# Build the server binary
RUN make clean && make server

# Copy runtime dependencies if needed (e.g., MOTD, banlist, config files)
# COPY motd.txt banlist.txt ./  # Uncomment if you use runtime files

# Expose the chat server port
EXPOSE 9001

# Set the default command
CMD ["./server"]
