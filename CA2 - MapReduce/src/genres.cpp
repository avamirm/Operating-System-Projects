#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string>
#include "include/defs.hpp"
#include <strings.h>

void error(const std::string &errorMsg)
{
    perror(errorMsg.c_str());
    exit(FAILURE);
}

std::string seperateInput(int &partFilesCount)
{
    std::string line, genreName;
    std::cin >> line;
    std::stringstream lineStream(line);
    std::string linePart;
    int count = 0;
    while(getline(lineStream, linePart, DELIM_CHAR))
    {
        if (count == 0)
            partFilesCount = stoi(linePart);
        else
            genreName = linePart;
        count++;
    }
    return genreName;
}

void recieveGenreCount(const std::string &genreName, const int &partFilesCount)
{
    int count = 0;
    for (int i = 0; i < partFilesCount; i++)
    {
        std::string fifo_name = "fifo/fifo_" + genreName + std::to_string(i+1);            
        int fifoFd = open(fifo_name.c_str(), O_RDONLY);
        char buffer[LINE_LEN];
        bzero(buffer, LINE_LEN);
        if (read(fifoFd, buffer, LINE_LEN) <= 0)
            error("Error:read() in genres.cpp");
        close(fifoFd);
        count += atoi(buffer);
    }
    std::cout << genreName << ":" << count << std::endl;   
}

int main(int argc, char *argv[])
{
    int partFilesCount;
    std::string genreName = seperateInput(partFilesCount);
    recieveGenreCount(genreName, partFilesCount);
}