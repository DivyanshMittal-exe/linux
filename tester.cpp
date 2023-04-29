#include <iostream>
#include <thread>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>

#include <poll.h>

using namespace std;

#include <sys/ioctl.h>


#define LIFO_IOC_MAGIC  'D'

#define LIFO_IOCRESET    _IO(LIFO_IOC_MAGIC, 0)

#define LIFO_DATA_AVBL  _IOR(LIFO_IOC_MAGIC, 1,int)
#define LIFO_REF_COUNT  _IOR(LIFO_IOC_MAGIC, 2,int)


struct pollfd fds[1];



char message[] = "Hello, this is a test message!";

void writer(int fd) {


    std::cout << "Writer thread started\n";
    for(int i = 0; i < 10; i++){
            ssize_t bytes_written = write(fd, message, sizeof(message)-1);
            std::cout << "Writer wrote " << bytes_written << " bytes\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            // if(i == 4){
            //     break;
            // }
    }

    close(fd);

}

void reader(int fd) {

    int count;
    if (ioctl(fd, LIFO_IOCRESET, &count) == -1) {
        cout << "Oh no " << endl;
        // return;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::cout << "Reader thread started\n";
    int size_to_read = 10*(sizeof(message)-1);

    char buffer[size_to_read + 1];


    if (ioctl(fd, LIFO_DATA_AVBL, &count) == -1) {
        cout << "Oh no " << endl;
        // return;
    }

    cout << "Data Count :" << count << endl;

    fds[0].fd = fd; 
    fds[0].events = POLLIN; 

    int ret = poll(fds, 1, 100);

    if (ret == -1) {
        cout << "Umm" << endl;
    } else if (ret == 0) {
        cout << "timeout occurred" << endl;
    } else {
        if (fds[0].revents & POLLIN) {
            cout << "input is available" << endl;
        }

         if (fds[0].revents & POLLOUT) {
            cout << "Space to write is available " << endl;
        }

    }

    ssize_t bytes_read = read(fd, buffer, size_to_read);
    if (bytes_read < 0) {
        std::cerr << "Error reading from device\n";
        return;
    }
    buffer[bytes_read] = '\0';
    std::cout << "Reader read " << bytes_read << " bytes: " << buffer << std::endl;
}

int main() { 
    int fd_reader = open("/dev/LIFO_CHAR_DEVICE0", O_RDONLY );
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

    return 0;
}
