#pragma once
#include "objLoader.h"

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
Vec radiance(const Ray& r, int depth, BVHTree& bvh, std::vector<Object*>& lightObjects, std::vector<tinyobj::material_t>& materials) {

	Vec ret;
	Intersection intersection;

	if (++depth > 3)
	{
		if (depth > 7 || floatrand() > 0.8)
		{
			// sampling the light
			for (auto obj : lightObjects)
			{
				float sampleLight = obj->sampleLight(r.origin, bvh);

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
	}

	if (bvh.intersect(r, intersection) == 0)
	{
		// sampling the light
		if (depth != 0)
		{
			for (auto obj : lightObjects)
			{
				float sampleLight = obj->sampleLight(r.origin, bvh);
				if (sampleLight > 1e-6)
				{
					float objMatid = obj->mat_id;
					auto& objAmbient = materials[objMatid].ambient;
					auto& objEmission = materials[objMatid].emission;
					ret = ret + (Vec(materials[objMatid].ambient) + materials[objMatid].emission) * sampleLight;
				}
			}
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

	// frenel reflectance
	float fresnel = 1.0f;
	if (transmittanceMax != 0)
	{
		bool into = r.direction.dot(n) < 0 ? true : false;
		float ior = materials[matid].ior;

		float nnt = into ? 1 / ior : ior;
		float ddn = r.direction.dot(nl), cos2t = 1;
		Vec transDir = (r.direction * nnt - n * ((into ? 1 : -1) * (ddn * nnt + sqrt(cos2t)))).normalized();
		float a = ior - 1, b = ior + 1, R0 = a * a / (b * b), c = 1 - (into ? -ddn : transDir.dot(n));
		fresnel = R0 + (1 - R0) * c * c * c * c * c;
		float transmitProbability = 1 - fresnel;

		float recipTransProb = (transmitProbability != 0 ? 1. / transmitProbability : 0);

		if (floatrand() < transmitProbability)
			return ret + radiance(Ray(intersection.point, transDir), depth, bvh, lightObjects, materials).mult(transmittance) * recipTransProb;
	}
	float diffuseProbability = diffuseMax / (diffuseMax + specularMax);



	if (floatrand() < diffuseProbability)
	{
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
		float recipDiffProb = (diffuseProbability != 0 ? 1. / diffuseProbability : 0.) / fresnel;

		return ret + radiance(Ray(intersection.point, randDir), depth, bvh, lightObjects, materials).mult(diffuse) * recipDiffProb;
	}
	else
	{

		// perpendicular to each other
		Vec refelDir = (r.direction - n * 2 * n.dot(r.direction)).normalized();
		Vec refelRandDir;

		
		if (floatrand() > 0) {
			Vec u = ((fabs(refelDir.x) > .1 ? Vec(0, 1) : Vec(1)).cross(refelDir)).normalized();
			Vec v = refelDir.cross(u).normalized();
			float theta = floatrand(2) * PI;
			refelRandDir = (refelDir + (u * std::cos(theta) + v * std::sin(theta)) * floatrand(0.58)).normalized();
		}
		else
			refelRandDir = refelDir;

		float recipSpecProb = 1.0/(1.0 - diffuseProbability) / fresnel;
		Vec radi = radiance(Ray(intersection.point, refelRandDir), depth, bvh, lightObjects, materials) * recipSpecProb;

		return ret + radi.mult(specular) * std::pow(refelRandDir.dot(refelDir), shininess);
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

	w /= 8;
	h /= 8;

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
	// readArrFromFile(modelSelect, c, s, w, h);


	int x1 = (float)w / 2.0f;
	int y1 = (float)h / 2.0;
	int a = 1;



	for (; s < samps; s++) {
		printf("\nRendering (%.0f spp) %5.2f%%", spp, 100. * s / (samps));
#pragma omp parallel for
		for (int y = 0; y < h; y++) {
			for (int x = 0; x < w; x++) {
				// 4 times radiance;
				
				Vec r;
				/*
				x = x1;
				if (a == 1)
					y = y1 - 10;
				else
					y = y1 + 10;
				a = 1 - a;
				Vec d = cxIncure * ((float)x - (float)w / 2.0f) +
					cyIncure * ((float)y - (float)h / 2.0f) + czIncure;

				r = r + radiance(Ray(cam.origin + d, d.normalized()), 0, bvh, lightObjects, materials);
				 */
				
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

				if (s == 0 && c[0].x == 0.0)
					c[i] = r * 0.25;
				else
					c[i] = (c[i] * (spp - 4) + r) * recipSpp;
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
