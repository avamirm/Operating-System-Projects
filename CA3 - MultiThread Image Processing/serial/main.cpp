#include <iostream>
#include <unistd.h>
#include <fstream>
#include <chrono>

using std::cout;
using std::endl;
using std::ifstream;
using std::ofstream;

#pragma pack(1)

typedef int LONG;
typedef unsigned short WORD;
typedef unsigned int DWORD;

#define OUTPUT_FILE "output.bmp"

struct RGB
{
  unsigned char r;
  unsigned char g;
  unsigned char b;
};

RGB **rgbImage;
char *fileBuffer;
int bufferSize;

static bool isFloatEqual(float x, float y) 
{
    const float epsilon = 0.00001f;
    return abs(x - y) <= epsilon ;
}
typedef struct tagBITMAPFILEHEADER
{
  WORD bfType;
  DWORD bfSize;
  WORD bfReserved1;
  WORD bfReserved2;
  DWORD bfOffBits;  //gives the number of bytes from the start of the file to the pixel values.
} BITMAPFILEHEADER, *PBITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER
{
  DWORD biSize;
  LONG biWidth;
  LONG biHeight;
  WORD biPlanes;
  WORD biBitCount;
  DWORD biCompression;
  DWORD biSizeImage;
  LONG biXPelsPerMeter;
  LONG biYPelsPerMeter;
  DWORD biClrUsed;
  DWORD biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;

int rows;
int cols;

bool fillAndAllocate(char *&buffer, const char *fileName, int &rows, int &cols, int &bufferSize)
{
  std::ifstream file(fileName);

  if (file)
  {
    file.seekg(0, std::ios::end);
    std::streampos length = file.tellg();
    file.seekg(0, std::ios::beg);

    buffer = new char[length];
    file.read(&buffer[0], length);

    PBITMAPFILEHEADER file_header;
    PBITMAPINFOHEADER info_header;

    file_header = (PBITMAPFILEHEADER)(&buffer[0]);
    info_header = (PBITMAPINFOHEADER)(&buffer[0] + sizeof(BITMAPFILEHEADER));
    rows = info_header->biHeight;
    cols = info_header->biWidth;
    bufferSize = file_header->bfSize;
    return 1;
  }
  else
  {
    cout << "Eroor: file doesn't exist!" << endl;
    return 0;
  }
}

void getPixlesFromBMP24()
{
  int count = 1;
  int extra = cols % 4;
  for (int i = 0; i < rows; i++)
  {
    count += extra;
    for (int j = cols - 1; j >= 0; j--)
      for (int k = 0; k < 3; k++)
      {
        switch (k)
        {
        case 0:
          rgbImage[i][j].r = fileBuffer[bufferSize - count];
          break;
        case 1:
          rgbImage[i][j].g = fileBuffer[bufferSize - count];
          break;
        case 2:
          rgbImage[i][j].b = fileBuffer[bufferSize - count];
          break;
        }
        count++;
      }
  }
}

void writeOutBmp24()
{
  std::ofstream write(OUTPUT_FILE);
  if (!write)
  {
    cout << "Error: failed to write " << OUTPUT_FILE << endl;
    return;
  }
  int count = 1;
  int extra = cols % 4;
  for (int i = 0; i < rows; i++)
  {
    count += extra;
    for (int j = cols - 1; j >= 0; j--)
      for (int k = 0; k < 3; k++)
      {
        switch (k)
        {
        case 0:
          fileBuffer[bufferSize - count] = rgbImage[i][j].r;
          break;
        case 1:
          fileBuffer[bufferSize - count] = rgbImage[i][j].g;
          break;
        case 2:
          fileBuffer[bufferSize - count] = rgbImage[i][j].b;
          break;
        }
        count++;
      }
  }
  write.write(fileBuffer, bufferSize);
}

RGB** allocateRGB()
{
  RGB **rgbImage = new RGB *[rows];
  for (int i = 0; i < rows; i++)
    rgbImage[i] = new RGB[cols];
  return rgbImage;
}

void mirrorFilter()
{
    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols/2; j++)
        {
          RGB temp = rgbImage[i][j];
          rgbImage[i][j] = rgbImage[i][cols - j - 1];
          rgbImage[i][cols - j - 1] = temp;
        }
    }  
}

void diamondFilter()
{
  int midRow = rows / 2;
  int midCol = cols / 2;
  float a1 = (float)(midRow) / (float)(midCol);
  float a2 = (float)(-midRow) / (float)(midCol);  
  for (int i = 0; i < rows; i++)
  {
    for (int j = 0; j < cols; j++)
    {
      if (isFloatEqual(j*a1 + midRow, float(i)) || isFloatEqual(j*a2 + midRow, float(i)) || isFloatEqual(j*a1 - midRow, float(i)) || isFloatEqual(j*a2 + midRow + rows, float(i)))
      {
        rgbImage[i][j].r = 255;
        rgbImage[i][j].g = 255;
        rgbImage[i][j].b = 255;
      }
    }
  }
}

void checkeredFilter()
{
  RGB** temp = allocateRGB();
  for (int i = 0; i < rows; i++)
    for (int j = 0; j < cols; j++)
      temp[i][j] = rgbImage[i][j];
  
  int conv[3][3] = {{-2, -1, 0}, {-1, 1, 1}, {0, 1, 2}};

  for (int i = 0; i < rows; i++)
  {
    for (int j = 0; j < cols; j++)
    {
      int tempR = 0;
      int tempG = 0;
      int tempB = 0;
      for (int dx = -1; dx <= 1; dx++)
      {
        for (int dy = -1; dy <= 1; dy++)
        {
          int xNeighbor = i + dx;
          int yNeighbor = j + dy;
          if (xNeighbor >= 0 && xNeighbor < rows && yNeighbor >= 0 && yNeighbor < cols)
          {
            tempR += int(temp[xNeighbor][yNeighbor].r) * conv[dx + 1][dy + 1];
            tempG += int(temp[xNeighbor][yNeighbor].g) * conv[dx + 1][dy + 1];
            tempB += int(temp[xNeighbor][yNeighbor].b) * conv[dx + 1][dy + 1];
          }
        }
      }
      if (tempR > 255)
        rgbImage[i][j].r = 255;
      else if (tempR < 0)
        rgbImage[i][j].r = 0;
      else
        rgbImage[i][j].r = tempR;
      if (tempG > 255)
        rgbImage[i][j].g = 255;
      else if (tempG < 0)
        rgbImage[i][j].g = 0;
      else
        rgbImage[i][j].g = tempG;
      if (tempB > 255)
        rgbImage[i][j].b = 255;
      else if (tempB < 0)
        rgbImage[i][j].b = 0;
      else
        rgbImage[i][j].b = tempB;
    }
  }
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    cout << "Error: number of args" << endl;
    return 0;
  }

  auto start = std::chrono::high_resolution_clock::now();
  char *fileName = argv[1];
  if (!fillAndAllocate(fileBuffer, fileName, rows, cols, bufferSize))
  {
    cout << "File read error" << endl;
    return 1;
  }
  rgbImage = allocateRGB();
  auto startGetPixlesFromBMP24 = std::chrono::high_resolution_clock::now();
  getPixlesFromBMP24();
  auto finishGetPixlesFromBMP24 = std::chrono::high_resolution_clock::now();
  std::cout << "getPixlesFromBMP24 Execution Time: " << std::chrono::duration_cast< std::chrono::milliseconds>  (finishGetPixlesFromBMP24 - startGetPixlesFromBMP24).count() << " ms" << std::endl;
  mirrorFilter();
  auto finishMirrorFilter = std::chrono::high_resolution_clock::now();
  std::cout << "mirrorFilter Execution Time: " << std::chrono::duration_cast< std::chrono::milliseconds>  (finishMirrorFilter - finishGetPixlesFromBMP24).count() << " ms" << std::endl;
  checkeredFilter();
  auto finishCheckeredFilter = std::chrono::high_resolution_clock::now();
  std::cout << "checkeredFilter Execution Time: " << std::chrono::duration_cast< std::chrono::milliseconds>  (finishCheckeredFilter - finishMirrorFilter).count() << " ms" << std::endl;
  diamondFilter();
  auto finishDiamondFilter = std::chrono::high_resolution_clock::now();
  std::cout << "diamondFilter Execution Time: " << std::chrono::duration_cast< std::chrono::milliseconds>  (finishDiamondFilter - finishCheckeredFilter).count() << " ms" << std::endl;
  writeOutBmp24();
  auto finishWriteOutBmp24 = std::chrono::high_resolution_clock::now();
  std::cout << "writeOutBmp24 Execution Time: " << std::chrono::duration_cast< std::chrono::milliseconds>  (finishWriteOutBmp24 - finishDiamondFilter).count() << " ms" << std::endl;
  auto finish = std::chrono::high_resolution_clock::now();
  std::cout << "Execution Time: " << std::chrono::duration_cast< std::chrono::milliseconds>  (finish - start).count() << " ms" << std::endl;
  return 0;
}