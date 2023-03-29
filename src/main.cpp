#pragma once
#include "objLoader.h"

#include <math.h>
int DEBUG = 1;
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
	tmp = tmp == 0.0f ? 1e-10 : tmp;

	return clamp(avergEmission / tmp);
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
			return ret + radiance(Ray(intersection.point, refelDir), depth, bvh, lightObjects).mult(specular);
		}

		Vec transDir = (r.direction * nnt - n * ((into ? 1 : -1) * (ddn * nnt + sqrt(cos2t)))).normalized();
		float a = ior - 1, b = ior + 1, R0 = a * a / (b * b), c = 1 - (into ? -ddn : transDir.dot(n));
		float fresnel = R0 + (1 - R0) * c * c * c * c * c;
		float transmitProbability = 1 - fresnel;

		float TransmitProb = 1 - fresnel, P = .25 + .5 * fresnel, RP = fresnel / P, TP = TransmitProb / (1 - P);

		if (floatrand() < TransmitProb)
		{
			float recipTransProb = (transmitProbability != 0.0f ? 1. / transmitProbability : 1e-6);
			return ret + radiance(Ray(intersection.point, transDir), depth, bvh, lightObjects).mult(transmittance) * TP;
		}
		else
		{
			float recipReflectProb = (fresnel != 0.0f ? 1. / fresnel : 1e-6);
			return ret + radiance(Ray(intersection.point, refelDir), depth, bvh, lightObjects).mult(specular) * RP;
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
		ret = ret + radiance(Ray(intersection.point, randDir), depth, bvh, lightObjects).mult(diffuse) * recipDiffProb;

	}
	else
	{
		// specular
		int lightObj = rand() % lightObjects.size();
		// probality of Sample this light
		float pdf = 0;
		/*
		pdf = caculatePdf(intersection, lightObjects[lightObj]);
		ret = ret + sampleAllLight(Ray(intersection.point,intersection.direction), bvh, lightObjects)*pdf;
		*/
		// perpendicular to each other
		Vec refelRandDir;
		//Bsdf
		Vec u = ((fabs(refelDir.x) > .1 ? Vec(0, 1) : Vec(1)).cross(refelDir)).normalized();
		Vec v = refelDir.cross(u).normalized();
		float theta = floatrand(2) * PI;
		refelRandDir = (refelDir + (u * std::cos(theta) + v * std::sin(theta)) * floatrand(0.2)).normalized();
		float recipSpecProb = 1.0 / (1.0 - diffuseProbability);
		Vec radi = radiance(Ray(intersection.point, refelRandDir), depth, bvh, lightObjects) * recipSpecProb;
		// if(1.0f - pdf >1e-6)
		ret = ret + radi.mult(specular) * std::pow(refelRandDir.dot(refelDir), shininess);
	}
	return ret;
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
	samps = 1024;

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
	objects.resize(0);
	w /= 16;
	h /= 16;

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
	float spp = 1 * samps;
	float recipSpp = 1.0 / spp;

	int s = 1;

	//srand(0);
	if (0)
		if (readArrFromFile(modelSelect, c, s, w, h))
			printf("Rendering begin with read data file! now spp: %d \n", s);
		else
			printf("Rendering begin!!!! \n");

	DEBUG = 0;
	int xmin = 0, xmax = w, ymin = 0, ymax = h;
	if (DEBUG) {
		/*
		xmin = w / 2 - w / 16;
		xmax = w / 2 + w / 16;
		*/

		xmin = w / 2;
		xmax = w / 2 + w / 4;
		ymax = h / 4;
	}

	for (s; s <= samps; s++) {
		if (s % 4 == 0) {
			printf("Rendering (%.0f spp) %5.2f%%", spp, 100. * s / (samps));
			// save file  per 16 spp;
			if (s % 16 == 0) {
				writeArrToFile(modelSelect, c, s, w, h);
				save_bitmap(modelSelect, c, w, h,static_cast<float>(s)/ static_cast<float>(samps));
			}
			printf("\n");
		}
		//#pragma omp parallel for
		for (int y = 0; y < h; y++) {
			/*
			if (s != 0) {
				if (y > ymax)
					break;
			}
			*/
			for (int x = 0; x < w; x++) {
				/*
				if (s != 1) {
					if (x < xmin)
						x = xmin;
					else if (x > xmax)
						break;
				}
				
				
				if ((x != 205 || y != h - 145 - 1))
				{
					int a = 0;
				}
				if (s == 165)
				{
					int a = 0;
				}
				*/
				float r1 = floatrand(2);
				float r2 = floatrand(2);
				float dx = r1 < 1 ? sqrt(r1) - 1 : 1 - sqrt(2 - r1);
				float dy = r2 < 1 ? sqrt(r2) - 1 : 1 - sqrt(2 - r2);
				Vec d = cxIncure * ((dx + .5) / 2 + x - w / 2) +
					cyIncure * ((dy + .5) / 2 + y - h / 2) + czIncure;
				Vec r;
				r = r + radiance(Ray(cam.origin + d, d.normalized()), 0, bvh, lightObjects);

				if (std::fpclassify(r.x) > 0 || std::fpclassify(r.y) > 0 || std::fpclassify(r.z) > 0)
				{
					printf("go wrong spp:%d, %7f %7f %7f------\n", s, r.x, r.y, r.z);
				}

				c[y * w + x] = c[y * w + x]  + r*recipSpp;
			}
		}
	}

	/*
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
	*/
	save_bitmap(modelSelect, c, w, h);

	return 0;
}
