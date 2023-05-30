#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <memory.h>
#include <iostream>
#include <chrono>
#include <unordered_map>
#include "image.h"
#include "socket.h"

#define PORT 3508
#define ADDRESS "127.0.0.1"
#define PC_NAME "machine_1"

enum class Month {
    JANUARY = 1,
    FEBRUARY = 2,
    MARCH = 3,
    APRIL = 4,
    MAY = 5,
    JUNE = 6,
    JULY = 7,
    AUGUST = 8,
    SEPTEMBER = 9,
    OCTOBER = 10,
    NOVEMBER = 11,
    DECEMBER = 12
};

const std::unordered_map<const Month, std::string, std::hash<Month>> months {
    {Month::JANUARY, "January"},
    {Month::FEBRUARY, "February"},
    {Month::MARCH, "March"},
    {Month::APRIL, "April"},
    {Month::MAY, "May"},
    {Month::JUNE, "June"},
    {Month::JULY, "July"},
    {Month::AUGUST, "August"},
    {Month::SEPTEMBER, "September"},
    {Month::OCTOBER, "October"},
    {Month::NOVEMBER, "November"},
    {Month::DECEMBER, "December"}
};

char* readFile(const std::string& filename) {
    FILE* file = fopen(filename.c_str(), "rb");
    if (!file) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return nullptr;
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(fileSize);
    if (!buffer) {
        std::cerr << "Error allocating memory for file contents" << std::endl;
        fclose(file);
        return nullptr;
    }

    size_t bytesRead = fread(buffer, 1, fileSize, file);
    if (bytesRead != fileSize) {
        std::cerr << "Error reading file contents" << std::endl;
        free(buffer);
        fclose(file);
        return nullptr;
    }

    fclose(file);

    return buffer;
}

const std::string monthToStr(const Month& month)
{
    return months.find(month)->second;
}

template<typename... Args>
std::string format(const std::string &format, Args... args)
{
    int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) + 1; // Extra space for '\0'

    if (size_s <= 0) {
        throw std::runtime_error("Error during formatting.");
    }

    auto size = static_cast<size_t>(size_s);
    char* buf = new char[size];
    std::snprintf(buf, size, format.c_str(), args...);
    std::string str(buf, buf + size - 1); // remove '\0'
    delete[] buf;
    return str;
}

std::string timestamp()
{
    std::string retval;
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    struct tm date;
    localtime_r(&now_c, &date);

    int hour  = date.tm_hour;
    int minute = date.tm_min;
    int second = date.tm_sec;
    int mon = date.tm_mon;
    int day = date.tm_mday;

    std::chrono::system_clock::time_point tp = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(tp);
    std::chrono::duration<double> fractional_seconds =
            (tp - std::chrono::system_clock::from_time_t(tt)) + std::chrono::seconds(second);

    std::string buffer("%s_%02d_%02d:%02d:%09.5f");
    std::string month = monthToStr(static_cast<Month>(mon + 1));
    retval = format(&buffer.front(), month.c_str(), day, hour, minute, fractional_seconds.count());

    return retval;
}

struct ClientData
{
    std::string machineName;
    char* imgData;
    int32_t imgDataLength;
    std::string timestamp;
};

bool sendData(int socket) {
    auto image = getImageBase();
    int bytesPerPixel = 0;
    auto fixedPixels = prepareImage(image, bytesPerPixel);

    std::string fileName = "screenshot.png";
    auto savedImage = saveImage(fileName.c_str(), image, fixedPixels, bytesPerPixel);

    struct stat fileInfo;
    fstat(fileno(savedImage), &fileInfo);

    int32_t dataSize = fileInfo.st_size;
    std::cout << dataSize << std::endl;

    auto imageData = readFile(fileName);

    if (!sendData<int32_t>(socket, dataSize)) {
        std::cerr << "failed to send data size" << std::endl;
        return false;
    }


    if (!sendDataS<char*>(socket, imageData, fileInfo.st_size)) {
        std::cerr << "failed to send image data" << std::endl;
        return false;
    }

    std::cout << "sent: " << dataSize << std::endl;
    dataSize = strlen(PC_NAME);

    if (!sendData<int32_t>(socket, dataSize)) {
        std::cerr << "failed to send pc name" << std::endl;
        return false;
    }

    if (!sendDataS<char*>(socket, PC_NAME, strlen(PC_NAME))) {
        std::cerr << "failed to send pc name" << std::endl;
        return false;
    }

    std::cout << "sent: " << dataSize << std::endl;
    std::string time = timestamp();
    dataSize = time.size();

    if (!sendData<int32_t>(socket, dataSize)) {
        std::cerr << "failed to send pc name" << std::endl;
        return false;
    }

    if (!sendDataS<char*>(socket, (char*)time.c_str(), time.size())) {
        std::cerr << "failed to send pc name" << std::endl;
        return false;
    }

    std::cout << "sent: " << dataSize << std::endl;

    fclose(savedImage);
    free(imageData);
    XDestroyImage(image);
    free(fixedPixels);
    return true;
}

int main() {
    int clientSocketDesc = socket(AF_INET, SOCK_STREAM, 0);

    if (clientSocketDesc < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return -1;
    }

    SocketGuard client(clientSocketDesc);

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(ADDRESS);
    serverAddress.sin_port = htons(PORT);

    if (connect(client.desc, (sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        std::cerr << "Error connecting to server" << std::endl;
        return -1;
    }

    int32_t buffer;
    int rc = recv(client.desc, &buffer, sizeof(int32_t), 0);

    if (rc <= 0) {
        std::cerr << "failed to initial handshake" << std::endl;
        return -1;
    }

    while (true) {
        int32_t buffer;
        int rc = recv(client.desc, &buffer, sizeof(int32_t), 0);

        if (rc <= 0) {
            std::cerr << "failed to read server" << std::endl;
            return -1;
        }

        std::cout << "readed server: " << buffer << std::endl;

        if (!sendData(client.desc)) {
            std::cerr << "failed to send data" << std::endl;
            return -1;
        }
    }
}
