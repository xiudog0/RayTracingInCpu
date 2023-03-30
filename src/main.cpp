#pragma once
#include "objLoader.h"

#include <math.h>

//#define _DEBUG_

/*
* DONE:
*	Ka, Kd, Ke, Ks, Ni
*	Tr: Transmittance
*   Ns: shininess
*   dissolve:
*   map_Kd: texture mapping in diffuse
* TODO:
*   illum:
*/
inline Vec sampleAllLight(const Ray& r, const BVHTree& bvh, const std::vector<Object*>& lightObjects) {
	Vec ret;
	for (auto obj : lightObjects)
	{
		float sampleLight = obj->sampleLight(r.origin + r.direction * 1e-3, bvh);
		auto objMat = obj->material;
		auto& objAmbient = objMat->ambient;
		auto& objEmission = objMat->emission;
		ret = ret + (Vec(objMat->ambient) + objMat->emission) * sampleLight;
	}
	return ret;
}

inline float caculatePdf(Intersection& intersection, Object* lightObject)
{
	if (lightObject->getType() != objectType::sph)
		return 0;

	float area = lightObject->getArea();
	float avergEmission = (lightObject->material->emission[0] +
		lightObject->material->emission[1] +
		lightObject->material->emission[2]
		) / 3.0f;
	float dis;
	float cos;
	Vec randPoint, normal;
	if (lightObject->getType() == objectType::sph) {
		Vec line = intersection.point - (static_cast<Sphere*>(lightObject)->center);
		dis = line.length();
		cos = std::abs((line * -1).normalized().dot(intersection.normal));
	}
	else
	{
		Vec line = intersection.point - (static_cast<Triangle*>(lightObject)->v1);
		dis = line.length();
		cos = std::abs((line * -1).normalized().dot(intersection.normal));
	}
	float tmp = area * dis * dis * cos * 1e4 * 2;
	tmp = (tmp == 0.0f) ? 1e-10 : tmp;
	tmp = avergEmission / tmp;

	return tmp < 1e-3 ? 0 : (tmp > 0.99999) ? 1.000001 : tmp;
}


Vec radiance(const Ray& r, int depth, BVHTree& bvh, std::vector<Object*>& lightObjects) {

	Vec ret;
	Intersection intersection;

	if (++depth > 3)
	{
		if (depth > 5 || floatrand() > 0.5)
		{
			return ret + sampleAllLight(r, bvh, lightObjects);
		}
	}

	if (bvh.intersect(r, intersection) == 0)
	{
		// sampling the light
		if (depth != 1)
		{
			ret = ret + sampleAllLight(r, bvh, lightObjects);
		}
		return ret;
	}

	auto material = intersection.material;


	const auto& ambient = material->ambient;
	const auto& emission = material->emission;
	const auto& transmittance = material->transmittance;
	const auto dissolve = material->dissolve;
	const auto shininess = material->shininess;
	const auto& specular = material->specular;
	const float specularMax = floatMax(specular);
	const float ambientMax = floatMax(ambient);
	const float emissionMax = floatMax(emission);
	const float transmittanceMax = floatMax(transmittance);

	ret = Vec(emission) + ambient;

	// stop if hit the light
	if (emissionMax != 0 || ambientMax > 1.0)
		return ret;

	Vec n = intersection.normal;
	Vec nl = n.dot(r.direction) < 0 ? n : n * -1;

	// reflect probality reflectance
	Vec refelDir = (r.direction - n * 2 * n.dot(r.direction)).normalized();

	if (transmittanceMax != 0)
	{
		bool into = r.direction.dot(n) < 0 ? true : false;
		float ior = material->ior;
		float nnt = into ? 1.0f / ior : ior;
		float ddn = r.direction.dot(nl);
		float cos2t = 1.0f - nnt * nnt * (1.0f - ddn * ddn);

		if (cos2t < 0)
		{
			return ret + radiance(Ray(intersection.point, refelDir), depth, bvh, lightObjects).mult(transmittance);
		}

		Vec transDir = (r.direction * nnt - n * ((into ? 1 : -1) * (ddn * nnt + sqrt(cos2t)))).normalized();
		float a = ior - 1, b = ior + 1, R0 = a * a / (b * b), c = 1 - (into ? -ddn : transDir.dot(n));
		float fresnel = R0 + (1 - R0) * c * c * c * c * c;
		float transmitProbability = 1 - fresnel;

		// Probablity
		float  P = .25 + .5 * fresnel, RP = fresnel / P, TP = transmitProbability / (1 - P);

		if (floatrand() < transmitProbability)
		{
			float recipTransProb = (transmitProbability != 0.0f ? 1. / transmitProbability : 1e-6);
			return ret + radiance(Ray(intersection.point, transDir), depth, bvh, lightObjects).mult(transmittance) * TP;
		}
		else
		{
			return ret + radiance(Ray(intersection.point, refelDir), depth, bvh, lightObjects).mult(transmittance) * RP;
		}
	}

	auto diffuse = Vec(material->diffuse);
	float diffuseMax = vecMax(diffuse);
	if (intersection.object->texture != nullptr)
	{
		auto  texture = intersection.object->getTextureByPoint(intersection.point);
		diffuse = diffuse.mult(texture);
	}

	float diffuseProbability = diffuseMax / (diffuseMax + specularMax);
	if (specularMax == 0 || floatrand() < diffuseProbability)
	{

		// w,u,v is perpendicular to each other
		Vec w = nl;
		Vec u = ((fabs(w.x) > .1 ? Vec(0, 1) : Vec(1)).cross(w)).normalized();
		Vec v = w.cross(u);
		// r1:(0-2pi)~U, r2:(0-1)~U, r2s
		float theta = PI * floatrand(2);
		float phi = PI * floatrand(0.5);
		float sinphi = std::sin(phi);
		// a random reflection ray

		Vec randDir = u * std::cos(theta) * sinphi + v * std::sin(theta) * sinphi + w * std::cos(phi);
		float recipDiffProb = (diffuseProbability != 0.0f ? 1. / diffuseProbability : 0.);
		return ret + radiance(Ray(intersection.point, randDir), depth, bvh, lightObjects).mult(diffuse) * recipDiffProb;
	}
	else
	{
		// specular
		int lightObj = rand() % lightObjects.size();
		// probality of Sample this light

		float pdf = 0;
		//caculatePdf(intersection, lightObjects[lightObj]);
		if (pdf != 0 && floatrand(0.999) < pdf) {
			return  ret + sampleAllLight(Ray(intersection.point, intersection.normal), bvh, lightObjects) * (1.0f / pdf);
		}
		// perpendicular to each other
		Vec refelRandDir;
		//Bsdf
		Vec u = ((fabs(refelDir.x) > .1 ? Vec(0, 1) : Vec(1)).cross(refelDir)).normalized();
		Vec v = refelDir.cross(u).normalized();
		float theta = floatrand(2) * PI;
		refelRandDir = (refelDir + (u * std::cos(theta) + v * std::sin(theta)) * floatrand(0.2)).normalized();
		float recipSpecProb = 1.0f / ((1.0f - diffuseProbability) * (1 - pdf));
		Vec radi = radiance(Ray(intersection.point, refelRandDir), depth, bvh, lightObjects) * recipSpecProb;
		// if(1.0f - pdf >1e-6)
		return  ret + radi.mult(specular) * std::pow(refelRandDir.dot(refelDir), shininess);


	}
}



int main(int argc, char* argv[]) {
	int w, h;
	float fovy;
	Ray cam;
	Vec camUp;

	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t>  materials;

	// 1: cornell-box, 2: veach-mis, 3: staircase, 4: test
	int modelSelect = (argc == 3 ? atoi(argv[1]) : 1);
	// 0: no-detail, 1: only material detail,2: all-detail
	int detailPrint;
	int samps = (argc == 2 ? atoi(argv[1]) : 1);

	// --------------------------------debug setting-------------------------------------

	modelSelect = 3;
	detailPrint = 1;
	samps = 4096;

	if (!objLoader(modelSelect, shapes, materials, detailPrint) ||
		!xmlCameraAndCorrectMaterial(modelSelect, w, h, fovy, cam, camUp, materials, detailPrint))
		return 0;

	std::vector<Object*> objects;
	std::vector<Object*> lightObjects;
	transferTinyobjToTriangle(shapes, objects, lightObjects, materials, detailPrint);



	BVHTree bvh{ objects };
	if (detailPrint)
	{
		int maxdepth = 0, alldepth = 0;
		bvh.depthInfo(bvh.root, 0, maxdepth, alldepth);
		printf("dfs object num : %zd, max depth:%d, average depth:%f\n\n",
			objects.size(), maxdepth, static_cast<float>(alldepth) / static_cast<float>(objects.size()));
	}
#ifdef _DEBUG_
	w /= 4;
	h /= 4;
#endif // DEBUF_
	// xyz increment;
	Vec cyIncure, cxIncure, czIncure;
	{
		Vec cv = cam.direction;
		Vec cu = camUp - cv * (camUp.dot(cv));
		Vec _aaa = cu.dot(cv);
		Vec cw = cv.cross(cu).normalized();
		float near = 0.1f;
		float tanFovy = std::tan(fovy * PI / 180.0f);
		czIncure = cv * near;
		cyIncure = cu * (tanFovy * near / static_cast<float>(h));
		cxIncure = cw * cyIncure.length();
	}

	Vec* c = new Vec[w * h];
	int spp = 1 * samps;
	float recipSpp = 1.0 / spp;

	int s = 1;

#ifdef _DEBUG_
	srand(0);
	if (0)
#endif // _DEBUG_
		if (readArrFromFile(modelSelect, c, s, spp, w, h))
			printf("Rendering begin with read data file! now spp: %d \n", s);
		else
			printf("Rendering begin!!!! \n");

#define CUTPHOTO 1 // --------------------------------------------------------------------

#ifdef _DEBUG_
	int xmin = 0, xmax = w - 1, ymin = 0, ymax = h - 1;
	if (CUTPHOTO) {
		/*
		xmin = w / 2 - w / 16;
		xmax = w / 2 + w / 16;
		*/

		xmin = w / 4;
		xmax = w / 2;
		//ymin = 1;
		ymax = h / 2;

		int a = 1;
		for (int y = ymin; y < ymax; y++)
		{
			if (xmin > 0)
				c[y * w + xmin - 1].x = 10.0f;
			if (xmax < w - 1)
				c[y * w + xmax + 1].x = 10.0f;
		}
		for (int x = xmin; x < xmax; x++)
		{
			if (ymin > 0)
				c[(ymin - 1) * w + x].x = 10.0f;
			if (ymax < w - 1)
				c[(ymax + 1) * w + x].x = 10.0f;
		}
	}

	printf("DEBUG Rendring begin ,x:(%d-%d), y(%d-%d),w: %d,h: %d\n", xmin, xmax, ymin, ymax, w, h);
#endif //DEBUF_


	for (s; s <= samps; s++) {
		if (s % 4 == 0) {
			printf("Rendering (%d spp) %5.2f%%", spp, 100. * s / (samps));
			// save file  per 16 spp;
			if (s % 16 == 0) {
#ifdef _DEBUG_
				if (0)
#endif // _DEBUG_
					writeArrToFile(modelSelect, c, s, spp, w, h);
				save_bitmap(modelSelect, c, w, h, static_cast<float>(s) / static_cast<float>(samps));
			}
			printf("\n");
		}
#pragma omp parallel for
#ifdef _DEBUG_
		for (int y = ymin; y < h; y++) {

			if (y > ymax)
				continue;
#else
		for (int y = 0; y < h; y++) {
#endif //_DEBUG_
			for (int x = 0; x < w; x++) {
#ifdef _DEBUG_
				if (x < xmin)
					continue;
				else if (x > xmax)
					continue;
#endif //_DEBUG_
				float r1 = floatrand(1) - 0.5;
				float r2 = floatrand(1) - 0.5;
				Vec d = cxIncure * (r1 + x - w / 2) +
					cyIncure * (r2 + y - h / 2) + czIncure;
				Vec r = radiance(Ray(cam.origin + d, d.normalized()), 0, bvh, lightObjects);
				c[y * w + x] = c[y * w + x] + r * recipSpp;
#ifdef _DEBUG_
				if (std::fpclassify(r.x) > 0 || std::fpclassify(r.y) > 0 || std::fpclassify(r.z) > 0)
				{
					printf("computer wrong answer!x:%d, y%d, spp:%d, %7f %7f %7f------\n", x, y, s, r.x, r.y, r.z);
				}
#endif // _DEBUG_
			}
		}
		}

#ifdef _DEBUG_
	float cmax = -1e10;
	float cmin = 1e10;
	for (int y = 0; y < w; y++) {
		for (int x = 0; x < w; x++)
		{
			int i = y * w + x;
			if (vecMax(c[i]) > cmax)
				cmax = vecMax(c[i]);
			if (std::min(std::min(c[i].x, c[i].y), c[i].z) < cmin)
				cmin = std::min(std::min(c[i].x, c[i].y), c[i].z);

		}
	}
	printf("min:%7f------max:%7f \n", cmin, cmax);
#endif //_DEBUG_

	save_bitmap(modelSelect, c, w, h);
	return 0;
		}
