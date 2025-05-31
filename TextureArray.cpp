#define _CRT_SECURE_NO_WARNINGS 1
#include "TextureArray.h"
#include <fstream>
#include <Windows.h>
using namespace std;

size_t formatSizeTable[] = { 0, sizeof(int) };
TextureArray::TextureArray(size_t initial_size)
	:
	dataBuffer(new uint8_t[initial_size])
{

}

bool TextureArray::loadFromFile(std::string path)
{
	fstream file(path.c_str(), ios::in | ios::binary);
	if (!file.is_open())
	{
		return false;
	}

	if (dataBuffer)
	{
		delete[] dataBuffer;
	}
	images.clear();

	uint32_t imageCount;
	file.read((char*)&imageCount, sizeof(uint32_t));
	images.resize(imageCount);

	size_t requiredSize = 0;

	for (uint32_t i = 0; i < imageCount; i++)
	{
		Image imgDesc;
		file.read((char*)&imgDesc.width, sizeof(uint32_t));
		file.read((char*)&imgDesc.height, sizeof(uint32_t));
		file.read((char*)&imgDesc.format, sizeof(uint32_t));
		if (imgDesc.format != IMAGE_FORMAT_R8G8B8A8)
		{
			OutputDebugString(L"TextureArray: Unsupported image format format\n");
			exit(-1);
		}
		imgDesc.bufferOffset = requiredSize;
		requiredSize += imgDesc.width * imgDesc.height * formatSizeTable[IMAGE_FORMAT_R8G8B8A8];
		images[i] = imgDesc;
	}
	dataBuffer = new uint8_t[requiredSize];

	streampos start = file.tellg();
	file.seekg(0, ios::end);
	streampos end = file.tellg();
	if (end - start != requiredSize)
	{
		OutputDebugString(L"TextureArray: texture array cache file has error\n");
		exit(-1);
	}

	file.seekg(start);
	file.read((char*)dataBuffer, requiredSize);

	return true;
}

TextureArray::~TextureArray()
{
	delete[] dataBuffer;
}
