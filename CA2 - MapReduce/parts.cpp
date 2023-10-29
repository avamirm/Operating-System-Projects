#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string>
#include <map>
#include "defs.h"

void error(const std::string &errorMsg)
{
    perror(errorMsg.c_str());
    exit(FAILURE);
}

std::vector <std::string> seperateGenres(std::string &genreNames)
{
    std::stringstream lineStream(genreNames);
    std::string genre;
    std::vector <std::string> genres;
    while(getline(lineStream, genre, COMMA))
        genres.push_back(genre);
    
    return genres;
}

std::string seperatePartsByComma(std::vector <std::string> &genres,std::string &partNum)
{
    std::string partFilePath;
    std::string line;
    std::cin >> line;
    std::stringstream lineStream(line);
    std::string linePart;
    std::string genreNames;
    int count = 0;
    while(getline(lineStream, linePart, DELIM_CHAR))
    {
        if (count == 0)
            partNum = linePart;
        else if (count == 1)
            partFilePath = linePart;
        else
            genreNames = linePart;
        count++;
    }

    genres = seperateGenres(genreNames);
    return partFilePath;
}

std::map <std::string, int> initializeGenresCount(const std::vector <std::string> &genres)
{
    std::map <std::string, int> genresCount;
    for (int i = 0; i < genres.size(); i++)
        genresCount.insert({ genres[i], 0 });
    
    return genresCount;
}

void findGenre(const std::string &linePart,std::map <std::string, int> &genresCount)
{
    std::stringstream lineStream(linePart);
    std::string genre;
    while(getline(lineStream, genre, COMMA))
    {
        for (auto itr = genresCount.begin(); itr != genresCount.end(); ++itr) 
        {
            if (itr->first == genre)
                itr->second += 1;
        }
    }

}

void addGenreCount(const std::string &line, std::map <std::string, int> &genresCount)
{
    std::stringstream lineStream(line);
    std::string linePart;
    int count = 0;
    while(getline(lineStream, linePart, COMMA))
    {
        if (count != 0)
        {
            findGenre(linePart, genresCount);
        }
        count++;
    }

}

void readFile(std::map <std::string, int> &genresCount,const std::string &partFilePath)
{
    std::ifstream file;
    file.open(partFilePath);
    std::string line;
    while(getline(file, line))
    {
        addGenreCount(line, genresCount);
    }
    file.close();
}

void sendToGenres(const std::map <std::string, int> &genresCount, const std::string &partNum)
{
    for (auto genreCount = genresCount.begin(); genreCount != genresCount.end(); ++genreCount)
    {
        std::string fifoName = "fifo/fifo_" + genreCount->first + partNum;
            
        int fifoFd = open(fifoName.c_str(), O_WRONLY);
        if (write(fifoFd, std::to_string(genreCount->second).c_str(), std::to_string(genreCount->second).size()) <= 0)
            error("Error: fail to write partsGenresCount to Genres");       
        close(fifoFd);
    }   
}

int main(int argc, char *argv[])
{
    std::vector <std::string> genres;
    std::string partNum;
    std::string partFilePath = seperatePartsByComma(genres, partNum);
    std::map <std::string, int> genresCount = initializeGenresCount(genres);
    readFile(genresCount, partFilePath);
    sendToGenres(genresCount, partNum);
}