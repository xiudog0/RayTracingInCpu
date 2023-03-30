#pragma once

#include "bvh.h"
#include "third/tinyobjloader/tiny_obj_loader.h"


inline float clamp(float x) { return x < 0 ? 0 : x>1 ? 1 : x; }
inline Vec clamp(Vec v) {
	return Vec(v.x < 0 ? 0 : v.x>1 ? 1 : v.x,
		v.y < 0 ? 0 : v.y>1 ? 1 : v.y,
		v.z < 0 ? 0 : v.z>1 ? 1 : v.z);
}
inline int toInt(float x) { return int(pow(clamp(x), 1 / 2.2) * 255 + .5); }

bool transferTinyobjToTriangle(std::vector<tinyobj::shape_t>& shapes,
	std::vector<Object*>& objects,
	std::vector<Object*>& lightObjects,
	std::vector<tinyobj::material_t>& materials,
	int detailPrint = 0);

bool printInfo(const std::vector<tinyobj::shape_t>& shapes, const std::vector<tinyobj::material_t>& materials, bool triangulate = true, int detail = false);

bool xmlCameraAndCorrectMaterial(int modelSelect, int& width, int& height, float& fovy, Ray& cam, Vec& camUp, std::vector<tinyobj::material_t>& materials, int detailPrint = 0);

bool loadObj(const char* fileName,
	std::vector<tinyobj::shape_t>& shapes,
	std::vector<tinyobj::material_t>& materials,
	const char* basePath = NULL,
	int detailPrint = true,
	unsigned int flags = 1);

bool objLoader(int modelSelect,
	std::vector<tinyobj::shape_t>& shapes,
	std::vector<tinyobj::material_t>& materials,
	int detailPrint);


#pragma pack(push, 1)
struct BitmapFileHeader {
	uint16_t type{ 0x4D42 };
	uint32_t size{ 0 };
	uint16_t reserved1{ 0 };
	uint16_t reserved2{ 0 };
	uint32_t offset{ 0 };
};

struct BitmapInfoHeader {
	uint32_t size{ 0 };
	int32_t width{ 0 };
	int32_t height{ 0 };
	uint16_t planes{ 1 };
	uint16_t bit_count{ 0 };
	uint32_t compression{ 0 };
	uint32_t size_image{ 0 };
	int32_t x_pixels_per_meter{ 0 };
	int32_t y_pixels_per_meter{ 0 };
	uint32_t colors_used{ 0 };
	uint32_t colors_important{ 0 };
};

struct Bitmap {
	BitmapFileHeader file_header;
	BitmapInfoHeader info_header;
	uint8_t* data{ nullptr };
};
#pragma pack(pop)

void writeArrToFile(const int modelSelect, const Vec c[],const int spp, const int smax, const int width, const int height,std::string fileName = "");

bool readArrFromFile(const int modelSelect, Vec c[], int& spp, int& smax,const int width, const int height, std::string fileName = "");

void save_bitmap(const int modelSelect, const Vec c[], const int width, const int height, float completePercent = 1.0, std::string fileName = "");

// save file according to completePercent;
void fixFile(const int modelSelect, int smax, float completePercent);

// merge two tmpData file and give a pitcutre "unclear.bmp"
bool mergeFile(const char* file1, const char* file2, const char* outputFile, const int width, const int height,int &smax,float completePercent = 1.0);
