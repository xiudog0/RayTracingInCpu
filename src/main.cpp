#pragma once
#include "objLoader.h"

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
inline Vec sampleAllLight(const Vec& Point, const BVHTree& bvh, const std::vector<Object*>& lightObjects, const std::vector<tinyobj::material_t>& materials) {
	Vec ret;
	for (auto obj : lightObjects)
	{
		float sampleLight = obj->sampleLight(Point, bvh);

		if (sampleLight > 1e-6)
		{
			float objMatid = obj->mat_id;
			auto& objAmbient = materials[objMatid].ambient;
			auto& objEmission = materials[objMatid].emission;
			ret = ret + (Vec(materials[objMatid].ambient) + materials[objMatid].emission) * sampleLight;
		}
	}
	return ret;
}

inline float caculatePdf(Intersection& intersection, Object* lightObject, std::vector<tinyobj::material_t>& materials)
{

	float area = lightObject->getArea();
	float avergEmission = (materials[lightObject->mat_id].emission[0] +
		materials[lightObject->mat_id].emission[1] +
		materials[lightObject->mat_id].emission[2]
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


Vec radiance(const Ray& r, int depth, BVHTree& bvh, std::vector<Object*>& lightObjects, std::vector<tinyobj::material_t>& materials) {

	Vec ret;
	Intersection intersection;

	if (++depth > 5)
	{
		if (depth > 7 || floatrand() > 0.8)
		{
			return ret + sampleAllLight(r.origin, bvh, lightObjects, materials);
		}
	}

	if (bvh.intersect(r, intersection) == 0)
	{
		// sampling the light
		if (depth != 1)
		{
			ret = ret + sampleAllLight(r.origin, bvh, lightObjects, materials);
		}
		return ret;
	}

	int matid = intersection.matId;
	auto& diffuse = materials[matid].diffuse;

	auto& ambient = materials[matid].ambient;
	auto& specular = materials[matid].specular;
	auto& emission = materials[matid].emission;
	auto& transmittance = materials[matid].transmittance;
	auto dissolve = materials[matid].dissolve;
	auto shininess = materials[matid].shininess;

	float ambientMax = floatMax(ambient);
	float diffuseMax = floatMax(diffuse);
	float specularMax = floatMax(specular);
	float emissionMax = floatMax(emission);
	float transmittanceMax = floatMax(transmittance);

	ret = Vec(emission) + ambient;


	// stop if hit the light
	if (emissionMax != 0 || ambientMax > 1.0)
	{
		return ret;
	}

	Vec n = intersection.normal;
	Vec nl = n.dot(r.direction) < 0 ? n : n * -1;

	// reflect probality reflectance
	Vec refelDir = (r.direction - n * 2 * n.dot(r.direction)).normalized();
	if (transmittanceMax != 0)
	{
		bool into = r.direction.dot(n) < 0 ? true : false;
		float ior = materials[matid].ior;
		float nnt = into ? 1 / ior : ior;
		float ddn = r.direction.dot(nl);
		float cos2t = 1 - ior * ior * (1 - ddn * ddn);	
		if (cos2t < 0)
		{
			return ret + radiance(Ray(intersection.point, refelDir), depth, bvh, lightObjects, materials).mult(specular);
		}

		Vec transDir = (r.direction * nnt - n * ((into ? 1 : -1) * (ddn * nnt + sqrt(cos2t)))).normalized();
		float a = ior - 1, b = ior + 1, R0 = a * a / (b * b), c = 1 - (into ? -ddn : transDir.dot(n));
		float fresnel = R0 + (1 - R0) * c * c * c * c * c;
		float transmitProbability = 1 - fresnel;

		float Tr = 1 - fresnel, P = .25 + .5 * fresnel, RP = fresnel / P, TP = Tr / (1 - P);
		if (floatrand() < transmitProbability)
		{	
			float recipTransProb = (transmitProbability != 0.0f ? 1. / transmitProbability : 1e-6);
			return ret + radiance(Ray(intersection.point, transDir), depth, bvh, lightObjects, materials).mult(transmittance) * TP;
		}
		else
		{
			float recipReflectProb = (fresnel != 0.0f ? 1. / fresnel : 1e-6);
			return ret + radiance(Ray(intersection.point, refelDir), depth, bvh, lightObjects, materials).mult(specular) * RP;
		}
	}

	float diffuseProbability = diffuseMax / (diffuseMax + specularMax);

	if (floatrand() < diffuseProbability)
	{
		//diffuse
		if (intersection.object->texture != nullptr)
		{
			auto  texture = intersection.object->getTextureByPoint(intersection.point);
			diffuse[0] = texture.x;
			diffuse[1] = texture.y;
			diffuse[2] = texture.z;
		}
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
		ret = ret + radiance(Ray(intersection.point, randDir), depth, bvh, lightObjects, materials).mult(diffuse) * recipDiffProb;

	}
	else
	{
		// specular
		int lightObj = rand() % lightObjects.size();
		// probality of Sample this light
		float pdf = 0;
		/*
		pdf = caculatePdf(intersection, lightObjects[lightObj], materials);
		ret = ret + sampleAllLight(intersection.point, bvh, lightObjects, materials)*pdf;
		*/
		// perpendicular to each other
		Vec refelDir = (r.direction - n * 2 * n.dot(r.direction)).normalized();
		Vec refelRandDir;

		//Bsdf
		Vec u = ((fabs(refelDir.x) > .1 ? Vec(0, 1) : Vec(1)).cross(refelDir)).normalized();
		Vec v = refelDir.cross(u).normalized();
		float theta = floatrand(2) * PI;
		refelRandDir = (refelDir + (u * std::cos(theta) + v * std::sin(theta)) * floatrand(0.2)).normalized();
		float recipSpecProb = 1.0 / (1.0 - diffuseProbability);
		Vec radi = radiance(Ray(intersection.point, refelRandDir), depth, bvh, lightObjects, materials) * recipSpecProb;

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
	samps = 128;

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


	w /= 2;
	h /= 2;

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
	float spp = 4 * samps;
	float recipSpp = 1.0 / spp;

	int s = 0;
	//readArrFromFile(modelSelect, c, s, w, h);

	int xmin = 0, xmax = w, ymin = 0, ymax = h;
	if (DEBUG) {
		xmin = w * 3 / 8 - w/16;
		xmax = w / 2 + w/16;
		ymax = h / 2 + h/8;
	}

	for (; s < samps; s++) {
		printf("\nRendering (%.0f spp) %5.2f%%", spp, 100. * s / (samps));
#pragma omp parallel for
		for (int y = 0; y < h; y++) {
			if (y > ymax && DEBUG)
				break;
			for (int x = 0; x < w; x++) {
				if (x < xmin && DEBUG)
					x = xmin;
				else if (x > xmax && DEBUG)
					break;

				Vec r;
				for (int sy = 0; sy < 2; sy++) {
					// 2x2 subpixel rows
					for (int sx = 0; sx < 2; sx++) {

						float r1 = 2 * floatrand();
						float r2 = 2 * floatrand();
						float dx = r1 < 1 ? sqrt(r1) - 1 : 1 - sqrt(2 - r1);
						float dy = r2 < 1 ? sqrt(r2) - 1 : 1 - sqrt(2 - r2);
						Vec d = cxIncure * ((sx + dx + .5) / 2 + x - w / 2) +
							cyIncure * ((sy + dy + .5) / 2 + y - h / 2) + czIncure;
						r = r + radiance(Ray(cam.origin + d, d.normalized()), 0, bvh, lightObjects, materials);
					}
				}

				int i = y * w + x;

				if (s == 0 && c[i].x == 0.0)
					c[i] = r * 0.25;
				else
					c[i] = (c[i] * (spp - 4.0f) + r) * recipSpp;
			}
		}

		// save file  per 16 spp;
		if (s % 4 == 0) {
			writeArrToFile(modelSelect, c, s, w, h);
			save_bitmap(modelSelect, c, w, h);
		}

	}


	save_bitmap(modelSelect, c, w, h);

	return 0;
}
