#pragma once

#include <iostream>
#include <unistd.h>

struct SocketGuard {
    SocketGuard(int d) {
        desc = d;
    }
    ~SocketGuard() {
        close(desc);
    }

    int desc;
};


template<class T>
bool sendData(int socket, T data) {
    int rc = write(socket, &data, sizeof(T));

    if (rc < 0) {
        std::cerr << "Error sending data size to server" << std::endl;
        return false;
    }

    return true;
}

template<class T, class Size>
bool sendDataS(int socket, T data, Size size) {
    int rc = write(socket, data, size);

    if (rc < 0) {
        std::cerr << "Error sending data size to server" << std::endl;
        return false;
    }

    return true;
}
