#pragma once
/*
	Format in memory
	uint32_t NrOfImages
	array of ImageHeader descriptors
	ImageHeader
	{
		uint32_t width
		uint32_t height
		uint32_t format
	}
	uint8_t[ * ] <- bytes for each image stored in order of
	appearance in header in continous chunks

*/
#include <vector>
#include <string>
#define IMAGE_FORMAT_R8G8B8A8 1

struct Image 
{
	uint32_t width;
	uint32_t height;
	uint32_t format;
	size_t bufferOffset;
};
class TextureArray
{
public:
	TextureArray(size_t initial_size);
	bool loadFromFile(std::string path);
	~TextureArray();
public:
	std::vector<Image> images;
	uint8_t* dataBuffer;
};

