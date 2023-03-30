#define TINYOBJLOADER_IMPLEMENTATION
#include "third/tinyobjloader/tiny_obj_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "third/stb/stb_image.h"

#include "objLoader.h"

#include <fstream>
#include <map>
#include <iostream>
#include <algorithm>




bool xmlCameraAndCorrectMaterial(int modelSelect, int& width, int& height, float& fovy, Ray& cam, Vec& camUp, std::vector<tinyobj::material_t>& materials, int detailPrint) {


	const char* filename;
	if (modelSelect == 1) filename = "./scenes/cornell-box/cornell-box.xml";
	else if (modelSelect == 2)filename = "./scenes/veach-mis/veach-mis.xml";
	else if (modelSelect == 3)filename = "./scenes/staircase/staircase.xml";
	else if (modelSelect == 4)filename = "./scenes/test/test.xml";
	else  return false;

	if (filename == nullptr)
		return false;
	std::ifstream File(filename);
	// 读取 XML 文件的内容
	std::string xmlText;
	std::string line;
	while (getline(File, line))
	{
		xmlText += line;
	}
	Vec eye, target, up;
	File.close();

	size_t k = 0;

	k = xmlText.find("width", k + 10);
	sscanf_s(xmlText.substr(k).c_str(), "width=\"%d\" height=\"%d\" fovy=\"%f\"", &width, &height, &fovy);

	k = xmlText.find("eye", k + 10);
	sscanf_s(xmlText.substr(k).c_str(), "eye x=\"%f\" y=\"%f\" z=\"%f\"", &eye.x, &eye.y, &eye.z);

	k = xmlText.find("lookat", k + 10);
	sscanf_s(xmlText.substr(k).c_str(), "lookat x=\"%f\" y=\"%f\" z=\"%f\"", &target.x, &target.y, &target.z);

	k = xmlText.find("up", k + 10);
	sscanf_s(xmlText.substr(k).c_str(), "up x=\"%f\" y=\"%f\" z=\"%f\"", &up.x, &up.y, &up.z);

	cam = { eye,(target - eye).normalized() };
	camUp = up.normalized();

	while (true)
	{
		k = xmlText.find("light mtlname", k + 10);
		if (k == std::string::npos)
			break;
		std::string str = xmlText.substr(k + 15, 25);
		int k2 = str.find('"');
		str = str.substr(0, k2);
		Vec emission;
		sscanf_s(xmlText.substr(k + 15 + k2).c_str(), "\" radiance=\"%f, %f, %f\"", &emission.x, &emission.y, &emission.z);
		for (int i = 0; i < materials.size(); i++)
		{
			if (materials[i].name == str)
			{
				materials[i].emission[0] = emission.x;
				materials[i].emission[1] = emission.y;
				materials[i].emission[2] = emission.z;
				break;
			}
		}
	}

	for (size_t i = 0; i < materials.size(); i++) {
		printf("material[%zd].name = %s\n", i, materials[i].name.c_str());
		if (detailPrint)
		{
			printf("  material.ambient = \t(%f, %f ,%f)\n", materials[i].ambient[0], materials[i].ambient[1], materials[i].ambient[2]);
			printf("  material.diffuse = \t(%f, %f ,%f)\n", materials[i].diffuse[0], materials[i].diffuse[1], materials[i].diffuse[2]);
			printf("  material.specular = \t(%f, %f ,%f)\n", materials[i].specular[0], materials[i].specular[1], materials[i].specular[2]);
			printf("  material.transmittance = (%f, %f ,%f)\n", materials[i].transmittance[0], materials[i].transmittance[1], materials[i].transmittance[2]);
			printf("  material.emission = \t(%f, %f ,%f)\n", materials[i].emission[0], materials[i].emission[1], materials[i].emission[2]);
			printf("  material.shininess = \t%f\n", materials[i].shininess);
			printf("  material.ior = %f\n", materials[i].ior);
			printf("  material.dissolve = %f\n", materials[i].dissolve);
			printf("  material.illum = %d\n", materials[i].illum);
			printf("  material.map_Ka = %s\n", materials[i].ambient_texname.c_str());
			printf("  material.map_Kd = %s\n", materials[i].diffuse_texname.c_str());
			printf("  material.map_Ks = %s\n", materials[i].specular_texname.c_str());
			printf("  material.map_Ns = %s\n", materials[i].specular_highlight_texname.c_str());
			printf("  material.map_bump = %s\n", materials[i].bump_texname.c_str());
			printf("  material.map_d = %s\n", materials[i].alpha_texname.c_str());
			printf("  material.disp = %s\n", materials[i].displacement_texname.c_str());
			std::map<std::string, std::string>::const_iterator it(materials[i].unknown_parameter.begin());
			std::map<std::string, std::string>::const_iterator itEnd(materials[i].unknown_parameter.end());

			for (; it != itEnd; it++) {
				printf("  material.%s = %s\n", it->first.c_str(), it->second.c_str());
			}
			printf("\n");
		}
	}

	if (detailPrint)
	{
		printf("Image width: %d, height: %d\n", width, height);
		printf("Camera information :\n  position: (%6.2f, %6.2f, %6.2f) \n  direction: (%6.2f, %6.2f, %6.2f)\n  up(): (%6.2f, %6.2f, %6.2f)\n  fov: %f;\n",
			eye.x, eye.y, eye.z, cam.direction.x, cam.direction.y, cam.direction.z, up.x, up.y, up.z, fovy);
	}

	return true;
}



bool objLoader(int modelSelect,
	std::vector<tinyobj::shape_t>& shapes,
	std::vector<tinyobj::material_t>& materials,
	int detailPrint)
{
	if (modelSelect == 1)
	{
		loadObj("./scenes/cornell-box/cornell-box.obj", shapes, materials, "./scenes/cornell-box/", detailPrint);
	}
	else if (modelSelect == 2)
	{
		loadObj("./scenes/veach-mis/veach-mis.obj", shapes, materials, "./scenes/veach-mis/", detailPrint);
	}
	else if (modelSelect == 3)
	{
		loadObj("./scenes/staircase/stairscase.obj", shapes, materials, "./scenes/staircase/", detailPrint);
	}
	else if (modelSelect == 4)
	{
		loadObj("./scenes/test/test.obj", shapes, materials, "./scenes/test/", detailPrint);
	}
	else
	{
		fprintf(stderr, "please input a right number!"); return 0;
	}
	return 1;
}

bool loadObj(
	const char* fileName,
	std::vector<tinyobj::shape_t>& shapes,
	std::vector<tinyobj::material_t>& materials,
	const char* basePath,
	int detailPrint,
	unsigned int flags)
{
	std::cout << "Loading " << fileName << std::endl;
	std::string err;
	bool ret = tinyobj::LoadObj(shapes, materials, err, fileName, basePath, flags);

	if (!err.empty()) {
		std::cerr << err << std::endl;
	}

	if (!ret) {
		std::cout << "failed to load " << fileName << std::endl;
		return false;
	}
	for (auto& material : materials)
	{
		if (material.diffuse_texname.size() != 0)
		{
			material.diffuse_texname = basePath + material.diffuse_texname;

		}
	}

	bool triangulate((flags & tinyobj::triangulation) == tinyobj::triangulation);


	return printInfo(shapes, materials, triangulate, detailPrint);
}


bool transferTinyobjToTriangle(std::vector<tinyobj::shape_t>& shapes, std::vector<Object*>& objects,
	std::vector<Object*>& lightObjects, std::vector<tinyobj::material_t>& materials, int detailPrint) {

	std::map<int, Texture*> tex;
	for (auto& shape : shapes)
	{
		auto& mesh = shape.mesh;
		bool mlight = mesh.indices.size() == 6996 ? 1 : 0;

		for (size_t f = 0; f < mesh.indices.size() / 3; f++)
		{
			int matid = mesh.material_ids[f];
			if (mlight == false || matid < 5)
			{
				if (mesh.texcoords.size() / 2 == mesh.positions.size() / 3)
				{
					Texture* texData = nullptr;

					// texture
					if (materials[matid].diffuse_texname.size() != 0)
					{
						/*
						printf("  vv0 = (%f, %f,%f),\t", mesh.positions[3 * mesh.indices[3 * f + 0]], mesh.positions[3 * mesh.indices[3 * f + 0] + 1], mesh.positions[3 * mesh.indices[3 * f + 0] + 2]);
						printf("  vv1 = (%f, %f,%f),\t", mesh.positions[3 * mesh.indices[3 * f + 1]], mesh.positions[3 * mesh.indices[3 * f + 1] + 1], mesh.positions[3 * mesh.indices[3 * f + 1] + 2]);
						printf("  vv2 = (%f, %f,%f);\n", mesh.positions[3 * mesh.indices[3 * f + 2]], mesh.positions[3 * mesh.indices[3 * f + 2] + 1], mesh.positions[3 * mesh.indices[3 * f + 2] + 2]);

						printf("  v0 = (%10f, %10f)\t", mesh.texcoords[2 * mesh.indices[3 * f + 0] + 0], mesh.texcoords[2 * mesh.indices[3 * f + 0] + 1]);
						printf("  v1 = (%10f, %10f)\t", mesh.texcoords[2 * mesh.indices[3 * f + 1] + 0], mesh.texcoords[2 * mesh.indices[3 * f + 1] + 1]);
						printf("  v2 = (%10f, %10f)\n", mesh.texcoords[2 * mesh.indices[3 * f + 2] + 0], mesh.texcoords[2 * mesh.indices[3 * f + 2] + 1]);
						*/
						if (tex.find(matid) == tex.end())
						{
							int width, height, channels;
							unsigned char* data = stbi_load(materials[matid].diffuse_texname.c_str(), &width, &height, &channels, 0);
							texData = new Texture{
								width,height,channels,
								data
							};
							tex[matid] = texData;
						}
						else
						{
							texData = tex[matid];
						}
					}

					objects.push_back(new Triangle(
						&mesh.positions[3 * mesh.indices[3 * f + 0]],
						&mesh.positions[3 * mesh.indices[3 * f + 1]],
						&mesh.positions[3 * mesh.indices[3 * f + 2]],
						Vec(mesh.texcoords[2 * mesh.indices[3 * f + 0] + 0], mesh.texcoords[2 * mesh.indices[3 * f + 0] + 1], 0),
						Vec(mesh.texcoords[2 * mesh.indices[3 * f + 1] + 0], mesh.texcoords[2 * mesh.indices[3 * f + 1] + 1], 0),
						Vec(mesh.texcoords[2 * mesh.indices[3 * f + 2] + 0], mesh.texcoords[2 * mesh.indices[3 * f + 2] + 1], 0),
						&materials[matid], texData
					));

				}
				else
				{
					objects.push_back(new Triangle(
						&mesh.positions[3 * mesh.indices[3 * f + 0]],
						&mesh.positions[3 * mesh.indices[3 * f + 1]],
						&mesh.positions[3 * mesh.indices[3 * f + 2]],
						&materials[matid]
					));
				}

				// if obj is a light
				if (floatMax(materials[matid].emission) != 0 || floatMax(materials[matid].ambient) > 1)
				{
					// if obj is bright enough or big erough
					if (objects.back()->getArea() > 1 || floatMax(objects.back()->material->emission) > 20)
						lightObjects.push_back(objects.back());
				}
			}
		}

		if (mlight)
		{
			objects.push_back(new Sphere(Vec(0.0, 6.5, 2.7), 0.05, &materials[5]));
			objects.push_back(new Sphere(Vec(0.0, 6.5, 0.0), 0.5, &materials[6]));
			objects.push_back(new Sphere(Vec(0.0, 6.5, -2.8), 1, &materials[7]));

			lightObjects.push_back(objects[objects.size() - 1]);
			lightObjects.push_back(objects[objects.size() - 2]);
			lightObjects.push_back(objects[objects.size() - 3]);
		}
	}



	printf("Transfer over!\n  object num: %zd\n  light num: %zd\n", objects.size(), lightObjects.size());
	if (detailPrint == 2)
	{
		for (size_t i = 0; i < objects.size(); i++)
		{
			auto box = (objects[i])->getBoundingBox();
			if (objects[i]->getType() == objectType::tri) {
				Triangle* tri = static_cast<Triangle*> (objects[i]);
				Vec e1 = tri->v1 - tri->v0;
				Vec e2 = tri->v2 - tri->v0;
				Vec normal = e1.cross(e2).normalized();

				printf("normal: (%6.2f,%6.2f,%6.2f)",
					normal.x, normal.y, normal.z
				);

				printf("object[%zd]: (%6.2f,%6.2f,%6.2f), (%6.2f,%6.2f,%6.2f), (%6.2f,%6.2f,%6.2f);\tmat:%s ;bound_box: (%6.2f,%6.2f,%6.2f), (%6.2f,%6.2f,%6.2f)\n", i,
					tri->v0.x, tri->v0.y, tri->v0.z,
					tri->v1.x, tri->v1.y, tri->v1.z,
					tri->v2.x, tri->v2.y, tri->v2.z,
					tri->material->name.c_str(),
					box.min.x, box.min.y, box.min.z, box.max.x, box.max.y, box.max.z
				);
				if (static_cast<Triangle*>(objects[i])->texture != nullptr)
				{

					auto text = static_cast<Triangle*> (objects[i])->texture;
					printf("  texture:img width:%d, height:%d,channels:%d; uv:(%6.2f,%6.2f), (%6.2f,%6.2f), (%6.2f,%6.2f)\n",
						text->w, text->h, text->c,
						tri->uv0.x, tri->uv0.y,
						tri->uv1.x, tri->uv1.y,
						tri->uv2.x, tri->uv2.y
					);
				}
			}
			else {
				auto box = (objects[i])->getBoundingBox();
				printf("object[%zd]: ---sphere--- center (%6.2f,%6.2f,%6.2f) ,radius:%6.2f,\tmat:%s ;bound_box: (%6.2f,%6.2f,%6.2f), (%6.2f,%6.2f,%6.2f)\n", i,
					static_cast<Sphere*> (objects[i])->center.x,
					static_cast<Sphere*> (objects[i])->center.y,
					static_cast<Sphere*> (objects[i])->center.z,
					static_cast<Sphere*> (objects[i])->radius,
					static_cast<Sphere*> (objects[i])->material->name.c_str(),
					box.min.x, box.min.y, box.min.z, box.max.x, box.max.y, box.max.z
				);
			}
		}
	}
	shapes.resize(0);
	return true;
}


bool printInfo(const std::vector<tinyobj::shape_t>& shapes, const std::vector<tinyobj::material_t>& materials, bool triangulate, int detail)
{
	std::cout << "# of shapes    : " << shapes.size() << std::endl;
	std::cout << "# of materials : " << materials.size() << std::endl;

	for (size_t i = 0; i < shapes.size(); i++) {
		printf("shape[%zd].name = %s\n", i, shapes[i].name.c_str());
		printf("Size of shape[%zd].indices: %zd\n", i, shapes[i].mesh.indices.size());

		if (triangulate)
		{
			printf("--------Already Triangulation-------\nSize of shape[%zd].material_ids: %zd\n", i, shapes[i].mesh.material_ids.size());
			if (detail == 2)
			{
				assert((shapes[i].mesh.indices.size() % 3) == 0);
				for (size_t f = 0; f < shapes[i].mesh.indices.size() / 3; f++) {
					printf("  idx[%zd] = %d, %d, %d. mat_id = %d\n", f,
						shapes[i].mesh.indices[3 * f + 0],
						shapes[i].mesh.indices[3 * f + 1],
						shapes[i].mesh.indices[3 * f + 2],
						shapes[i].mesh.material_ids[f]);
				}
			}
		}
		else {
			for (size_t f = 0; f < shapes[i].mesh.indices.size(); f++) {
				printf("  idx[%zd] = %d\n", f, shapes[i].mesh.indices[f]);
			}

			printf("---------Not Triangulation----------\nSize of shape[%zd].material_ids: %zd\n", i, shapes[i].mesh.material_ids.size());
			if (detail == 2)
			{
				assert(shapes[i].mesh.material_ids.size() == shapes[i].mesh.num_vertices.size());
				for (size_t m = 0; m < shapes[i].mesh.material_ids.size(); m++) {
					printf("  material_id[%ld] = %zd\n", m,
						shapes[i].mesh.material_ids[m]);
				}
			}
		}

		printf("shape[%zd].num_faces: %zd\n", i, shapes[i].mesh.num_vertices.size());
		if (detail == 2)
		{
			for (size_t v = 0; v < shapes[i].mesh.num_vertices.size(); v++) {
				printf("  num_vertices[%zd] = %ld\n", v,
					static_cast<long>(shapes[i].mesh.num_vertices[v]));
				if (shapes[i].mesh.num_vertices[v] != 3)
				{
					printf("\n------------can't deal non-triangulation object-------------\n");
					return false;
				}
			}
		}

		printf("shape[%zd].vertices: %zd\n", i, shapes[i].mesh.positions.size() / 3);
		if (detail == 2)
		{
			assert((shapes[i].mesh.positions.size() % 3) == 0);
			for (size_t v = 0; v < shapes[i].mesh.positions.size() / 3; v++) {
				printf("  v[%zd] = (%f, %f, %f)\n", v,
					shapes[i].mesh.positions[3 * v + 0],
					shapes[i].mesh.positions[3 * v + 1],
					shapes[i].mesh.positions[3 * v + 2]);
			}
		}

		printf("Size of shape[%zd].texcoords: %zd\n", i, shapes[i].mesh.texcoords.size() / 2);

		if (detail == 2)
		{
			for (size_t t = 0; t < shapes[i].mesh.texcoords.size() / 2; t++) {
				{
					printf("  texcoords[%zd] = (%f, %f)\n", t,
						shapes[i].mesh.texcoords[t * 2],
						shapes[i].mesh.texcoords[t * 2 + 1]);
				}
			}
		}


		printf("shape[%zd].num_tags: %zd\n", i, shapes[i].mesh.tags.size());
		if (detail == 2)
		{
			for (size_t t = 0; t < shapes[i].mesh.tags.size(); t++) {
				printf("  tag[%zd] = %s ", t, shapes[i].mesh.tags[t].name.c_str());
				printf(" ints: [");
				for (size_t j = 0; j < shapes[i].mesh.tags[t].intValues.size(); ++j)
				{
					printf("%ld", static_cast<long>(shapes[i].mesh.tags[t].intValues[j]));
					if (j < (shapes[i].mesh.tags[t].intValues.size() - 1))
					{
						printf(", ");
					}
				}
				printf("]");

				printf(" floats: [");
				for (size_t j = 0; j < shapes[i].mesh.tags[t].floatValues.size(); ++j)
				{
					printf("%f", shapes[i].mesh.tags[t].floatValues[j]);
					if (j < (shapes[i].mesh.tags[t].floatValues.size() - 1))
					{
						printf(", ");
					}
				}
				printf("]");

				printf(" strings: [");
				for (size_t j = 0; j < shapes[i].mesh.tags[t].stringValues.size(); ++j)
				{
					printf("%s", shapes[i].mesh.tags[t].stringValues[j].c_str());
					if (j < (shapes[i].mesh.tags[t].stringValues.size() - 1))
					{
						printf(", ");
					}
				}
				printf("]");
				printf("\n");
			}
		}


	}
	return true;
}



void writeArrToFile(const int modelSelect, const Vec c[], const int spp, const int smax, const int width, const int height, std::string fileName)
{

	std::string filename;
	if (modelSelect == 1) filename = "tmpData/cornell_box";
	else if (modelSelect == 2) filename = "tmpData/veach_mis";
	else if (modelSelect == 3) filename = "tmpData/staircase";
	else if (modelSelect == 4) filename = "tmpData/test";
	else filename = "unclear_image";
	filename += "W" + std::to_string(width);
	filename += "H" + std::to_string(height);
	filename += ".tmpData";

	if (fileName != "")
		filename = fileName;

	std::ofstream file(filename, std::ios::binary);
	if (!file) {
		std::cerr << "can't open" << filename << std::endl;
		return;
	}
	file.write(reinterpret_cast<const char*>(&spp), sizeof(spp));
	file.write(reinterpret_cast<const char*>(&smax), sizeof(smax));
	file.write(reinterpret_cast<const char*>(c), width * height * sizeof(Vec));
	file.close();
}



bool readArrFromFile(const int modelSelect, Vec c[], int& spp, int& smax, const int width, const int height, std::string fileName)
{
	std::string filename;
	if (modelSelect == 1) filename = "tmpData/cornell_box";
	else if (modelSelect == 2) filename = "tmpData/veach_mis";
	else if (modelSelect == 3) filename = "tmpData/staircase";
	else if (modelSelect == 4) filename = "tmpData/test";
	else filename = "tmpData/unclear_image";

	filename += "W" + std::to_string(width);
	filename += "H" + std::to_string(height);
	filename += ".tmpData";

	if (fileName != "")
		filename = fileName;

	std::ifstream file(filename, std::ios::binary);
	if (!file) {
		return false;
	}

	int premax = smax;

	file.read(reinterpret_cast<char*>(&spp), sizeof(spp));
	file.read(reinterpret_cast<char*>(&premax), sizeof(premax));
	file.read(reinterpret_cast<char*>(c), width * height * sizeof(Vec));
	file.close();


	if (spp > smax)
	{
		std::cout << "warning:finish read spp is" << spp << ", but spp max setting is " << smax
			<< ", modified to" << premax << std::endl;
		smax = premax;
	}

	float recipMax = static_cast<float>(premax) / static_cast<float>(smax);

	for (int i = 0; i < width * height; i++)
		c[i] = c[i] * recipMax;

	return true;
}



void save_bitmap(const int modelSelect, const Vec c[], const int width, const int height, float completePercent, std::string fileName) {


	std::string filename;
	if (modelSelect == 1) filename = "Acornell_box.bmp";
	else if (modelSelect == 2) filename = "Aveach_mis.bmp";
	else if (modelSelect == 3) filename = "Astaircase.bmp";
	else if (modelSelect == 4) filename = "Atest.bmp";
	else filename = "unclear_image.bmp";

	if (fileName != "")
		filename = fileName;

	std::ofstream file(filename, std::ios::binary);
	if (!file) {
		printf("bitmap created!");
		return;
	}

	Bitmap bitmap;
	bitmap.file_header.size = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader) + width * height * 3;
	bitmap.file_header.offset = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader);
	bitmap.info_header.size = sizeof(BitmapInfoHeader);
	bitmap.info_header.width = width;
	bitmap.info_header.height = height;
	bitmap.info_header.bit_count = 24;
	bitmap.data = new uint8_t[width * height * 3];

	float recipCP = 1 / completePercent;

	for (int i = 0; i < width * height; ++i) {
		bitmap.data[i * 3 + 0] = (toInt(c[i].z * recipCP)) & 0xFF;
		bitmap.data[i * 3 + 1] = (toInt(c[i].y * recipCP)) & 0xFF;
		bitmap.data[i * 3 + 2] = (toInt(c[i].x * recipCP)) & 0xFF;
	}



	file.write(reinterpret_cast<char*>(&bitmap.file_header), sizeof(BitmapFileHeader));
	file.write(reinterpret_cast<char*>(&bitmap.info_header), sizeof(BitmapInfoHeader));
	file.write(reinterpret_cast<char*>(bitmap.data), width * height * 3);

	std::cout << "  " << filename << " saved!";

	delete[] bitmap.data;

}

void fixFile(const int modelSelect, int smax, float completePercent)
{

	Vec* c = new Vec[1024 * 1024];
	int s;
	readArrFromFile(modelSelect, c, s, smax, 1024, 1024);

	save_bitmap(modelSelect, c, 1024, 1024, completePercent);
	delete[] c;
}

bool mergeFile(const char* file1, const char* file2, const char* outputFile, const int width, const  int height, int& smax, float completePercent)
{
	Vec* c1 = new Vec[width * height];
	int s1, s1premax;


	std::ifstream fileR(file1, std::ios::binary);
	if (!fileR) {
		return false;
	}
	int premax;

	fileR.read(reinterpret_cast<char*>(&s1), sizeof(s1));
	fileR.read(reinterpret_cast<char*>(&s1premax), sizeof(s1premax));
	fileR.read(reinterpret_cast<char*>(c1), width * height * sizeof(Vec));
	fileR.close();


	Vec* c2 = new Vec[width * height];
	int s2, s2premax;

	std::ifstream fileR2(file1, std::ios::binary);
	if (!fileR2) {
		std::cerr << "can't open" << file1 << std::endl;
		return false;
	}
	fileR2.read(reinterpret_cast<char*>(&s2), sizeof(s2));
	fileR2.read(reinterpret_cast<char*>(&s2premax), sizeof(s2premax));
	fileR2.read(reinterpret_cast<char*>(c2), width * height * sizeof(Vec));
	fileR2.close();


	int spp = s1 + s2;

	if (smax < spp)
	{
		smax = std::max(s1premax, s2premax);
		if (smax < spp)
			smax *= 2;
		printf("warning: smax is too small to contain two file, now modify to %d\n", smax);
	}

	if (smax == 0 || spp == 0)
	{
		printf("no data to merge\n");
		return false;
	}
	float recipS1Max = static_cast<float>(s1premax) / static_cast<float>(smax);
	float recipS2Max = static_cast<float>(s2premax) / static_cast<float>(smax);


	for (int i = 0; i < width + height; i++)
		c1[i] = c1[i] * recipS1Max + c2[i] * recipS2Max;

	save_bitmap(-1, c1, width, height, static_cast<float>(spp) / static_cast<float>(smax));

	std::ofstream fileW(outputFile, std::ios::binary);
	if (!fileW) {
		std::cerr << "can't open" << outputFile << std::endl;
		return false;
	}
	fileW.write(reinterpret_cast<const char*>(&spp), sizeof(spp));
	fileW.write(reinterpret_cast<const char*>(&smax), sizeof(smax));
	fileW.write(reinterpret_cast<const char*>(c1), width * height * sizeof(Vec));
	fileW.close();

	delete[] c1;
	delete[] c2;

}
