#include <iostream>
#include <thread>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>


char message[] = "Hello, this is a test message!";

void writer(int fd) {
    std::cout << "Writer thread started\n";
    for(int i = 0; i < 10; i++){
            ssize_t bytes_written = write(fd, message, sizeof(message)-1);
            std::cout << "Writer wrote " << bytes_written << " bytes\n";
    }

}

void reader(int fd) {
    std::cout << "Reader thread started\n";
    int size_to_read = 10*(sizeof(message)-1);

    char buffer[size_to_read + 1];
    ssize_t bytes_read = read(fd, buffer, size_to_read);
    if (bytes_read < 0) {
        std::cerr << "Error reading from device\n";
        return;
    }
    buffer[bytes_read] = '\0';
    std::cout << "Reader read " << bytes_read << " bytes: " << buffer << std::endl;
}

int main() {
    int fd_reader = open("/dev/LIFO_CHAR_DEVICE0", O_RDONLY);
    if (fd_reader < 0) {
        std::cerr << "Error opening device\n";
        return 1;
    }

    int fd_writer = open("/dev/LIFO_CHAR_DEVICE1", O_WRONLY);
    if (fd_writer < 0) {
        std::cerr << "Error opening device\n";
        return 1;
    }
    std::thread t1(writer, fd_writer);
    std::thread t2(reader, fd_reader);
    t1.join();
    t2.join();

    close(fd_reader);
    close(fd_writer);

    return 0;
}
