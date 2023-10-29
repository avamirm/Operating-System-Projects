
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <vector>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <string>
#include "defs.h"

void error(const std::string &errorMsg)
{
    perror(errorMsg.c_str());
    exit(FAILURE);
}

void makeGenresProcesses(const std::vector <std::string> &genreNames, const std::string &partsFilesSize, int &pid)
{
    for (int i = 0; i < genreNames.size(); i++)
    {
        int pipeFd[2];
        if (pipe(pipeFd) == -1)
            error("Error:pipe()");

        pid = fork();
        if (pid < 0)
            error("Error:fork()");
        else if (pid == 0)
        {
            close(pipeFd[WRITE]);
            dup2(pipeFd[READ], STDIN_FILENO);
            close(pipeFd[READ]);
            if (execl(GENRES_OUTPUT, GENRES_OUTPUT, NULL) == -1)
                error("Error:execl()");
            close(pipeFd[READ]);
        }
        else
        {
            std::string buffer = partsFilesSize + DELIMITER + genreNames[i];
            if (write(pipeFd[WRITE], buffer.c_str(), buffer.size()) <= 0)
                error("Error:write");
            close(pipeFd[WRITE]);
        }
    }
}

std::string getPartNum(const std::string &fileName)
{
    std::string partNum = "";
    for (int i = 0; i < fileName.size(); i++)
    {
        if (fileName[i] <= '9' && fileName[i] >= '0')
            partNum += fileName[i];
    }
    return partNum;
}

void makePartsProcesses(const std::vector <std::string> &fileNames, const std::string& genres, int &pid)
{
    for (int i = 1; i < fileNames.size(); i++)
    {
        int fd[2];
        if (pipe(fd) == -1)
            error("Error:pipe()");

        pid = fork();
        if (pid < 0)
            error("Error:fork()");
        else if (pid == 0)
        {
            close(fd[WRITE]);
            dup2(fd[READ], STDIN_FILENO);
            close(fd[READ]);

            if (execl(PARTS_OUTPUT, PARTS_OUTPUT, NULL) == -1)
                error("Error:execl()");
            close(fd[READ]);
        }    
        else
        {
            std::string partNum = getPartNum(fileNames[i]);
            std::string buf = partNum + DELIMITER + fileNames[i] + DELIMITER + genres;
            if (write(fd[WRITE], buf.c_str(), buf.size()) <= 0)
                error("Error:write");

            close(fd[WRITE]);
        }    
    }
}

std::vector <std::string> getFileNames(std::string &dirName)
{
    std::vector <std::string> fileNames;
    const std::filesystem::path directory{dirName};
    for (auto const& dir_entry : std::filesystem::directory_iterator{directory}) 
    {
        std::string filePath = dir_entry.path();
        fileNames.push_back(filePath);
    }
    sort(fileNames.begin(), fileNames.end());
    return fileNames;
}

std::vector <std::string> getGenres(std::string &genrefile, std::string &line)
{
    std::vector <std::string> genreNames;
    std::ifstream fin;
    fin.open(genrefile);
    getline(fin, line);
    std::stringstream lineStream(line);
    std::string genre;
    while(getline(lineStream, genre, COMMA))
        genreNames.push_back(genre);
    fin.close();
    return genreNames;
}

void makeGenreFifo(const std::vector <std::string> &genreNames, const int &partFilesCount)
{
    for (int i = 0; i < genreNames.size(); i++)
    {
        for (int j = 0; j < partFilesCount; j++)
        {
            std::string fifoName = "fifo/fifo_" + genreNames[i] + std::to_string(j + 1);
            if (mkfifo(fifoName.c_str(), 0666) == -1)
                error("Error:mkfifo() in main process");
        }
    }
    
}

int main(int argc, char *argv[])
{
    if (argc != 2)
        error("You have not enter folder's path");
    std::string dirName = argv[1];
    int pid = 1;
    std::vector <std::string> fileNames = getFileNames(dirName);
    std::string genres;
    std::vector <std::string> genreNames = getGenres(fileNames[0], genres);
    makeGenreFifo(genreNames, fileNames.size()-1);
    makePartsProcesses(fileNames, genres, pid);
    makeGenresProcesses(genreNames, std::to_string(fileNames.size()-1), pid);
    if (pid > 0)
        for (int i = 0; i < fileNames.size(); i++)
            wait(NULL);
}