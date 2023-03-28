#pragma once
#include <cmath>
#include <vector>


constexpr float PI = 3.1415926;
inline float floatrand(float max = 1) { return max * static_cast <float> (rand()) / static_cast <float> (RAND_MAX); }

struct Vec {
	float x, y, z;

	Vec(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
	Vec(float v[]) : x(v[0]), y(v[1]), z(v[2]) {}
	Vec operator+(const Vec& v) const { return Vec(x + v.x, y + v.y, z + v.z); }
	Vec operator+(const float v[]) const { return Vec(x + v[0], y + v[1], z + v[2]); }
	Vec operator-(const Vec& v) const { return Vec(x - v.x, y - v.y, z - v.z); }
	Vec operator*(const float s) const { return Vec(x * s, y * s, z * s); }
	float operator[](const int i) const { return i == 0 ? x : (i == 1 ? y : z); }

	float dot(const Vec& v) const { return x * v.x + y * v.y + z * v.z; }
	Vec cross(const Vec& v) const { return Vec(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x); }
	Vec mult(const float f[]) const { return Vec(x * f[0], y * f[1], z * f[2]); }
	float length() const { return sqrt(x * x + y * y + z * z); }
	Vec normalized() const { float len = length(); return Vec(x / len, y / len, z / len); }
};

struct Ray {
	Vec origin;
	Vec direction;
	float tMin, tMax;
	Ray(const Vec& origin = { 0,0,0 }, const Vec& direction = { 0,0,0 }, float tMin = 0, float tMax = 1e10)
		: origin(origin), direction(direction), tMin(tMin), tMax(tMax) {}
	// ��������Ͼ���ԭ��Ϊt�ĵ�
	Vec at(float t) const { return origin + direction * t; }
};

class AABB {
public:
	Vec min, max;

	AABB() {
		float inf = 1e20;
		min = Vec(inf, inf, inf);
		max = Vec(-inf, -inf, -inf);
	}

	AABB(const Vec& min, const Vec& max) : min(min), max(max) {}
	bool intersect(const Ray& ray) const;
	static AABB merge(const AABB& a, const AABB& b);
};

enum objectType : int
{
	tri = 10,
	sph = 20
};

class BVHTree;
class Object;

struct Intersection {
	float t; // ���߲���
	int matId; // �������ڵĲ���id
	Object* object;
	Vec point; // ����λ��
	Vec normal; // ���㴦�ķ�����

	// Ĭ�Ϲ��캯��
	Intersection() : t(INFINITY), matId(-1), object(nullptr) {}

	// ����ȽϺ�������������
	bool operator<(const Intersection& other) const { return t < other.t; }
};

struct Texture
{
	int w, h, c;
	unsigned char* photo;
	Texture(int width, int height, int channels, unsigned char* photoData) :
		w(width), h(height), c(channels), photo(photoData) {}
};

class Object {
public:
	virtual AABB getBoundingBox() = 0;
	// computer the intersection
	virtual bool intersect(const Ray& ray, Intersection& intersection) = 0;
	// while it is a light object, sample it from a intersection
	virtual float sampleLight(Vec point, BVHTree& bvh) = 0;

	virtual float getArea() = 0;

	virtual objectType getType() = 0;

	virtual Vec getTextureByPoint(Vec point) = 0;

	int mat_id; // material id
	Texture* texture;
};


class Sphere : public Object {
public:
	Vec center;
	float radius;

	Sphere(float f[3], float r, int mid, bool comArea = false) :
		center(f), radius(r) {
		mat_id = mid;
		texture = nullptr;
	}

	Sphere(const Vec v, float r, int mid, bool comArea = false) :
		center(v), radius(r) {
		mat_id = mid;
		texture = nullptr;
	}


	virtual AABB getBoundingBox()override;
	virtual bool intersect(const Ray& ray, Intersection& intersection) override;
	virtual float sampleLight(Vec point, BVHTree& bvh) override;
	virtual float getArea() override;
	virtual Vec getTextureByPoint(Vec point) override;
	objectType getType() override { return objectType::sph; }
};

class Triangle : public Object {
public:
	Vec v0, v1, v2;
	Vec uv0, uv1, uv2;
	Triangle(float f0[3], float f1[3], float f2[3],Vec uvv0,Vec uvv1,Vec uvv2, int mid, Texture* tex = nullptr) :
		v0(f0), v1(f1), v2(f2), uv0(uvv0), uv1(uvv1), uv2(uvv2) {
		mat_id = mid;
		texture = tex;
	}

	Triangle(const Vec& v0, const Vec& v1, const Vec& v2, int mid) :
		v0(v0), v1(v1), v2(v2){
		mat_id = mid;
		texture = nullptr;
	}

	virtual AABB getBoundingBox()override;
	virtual bool intersect(const Ray& ray, Intersection& intersection) override;
	virtual float sampleLight(Vec point, BVHTree& bvh) override;
	virtual float getArea()override;
	virtual Vec getTextureByPoint(Vec point) override;

	virtual objectType getType() override { return objectType::tri; }
};



class BVHNode {
public:
	AABB box; // AABB��ʾ��Χ��
	BVHNode* left; // ������
	BVHNode* right; // ������
	Object* object; // ����ָ��
	bool isLeaf; // �Ƿ�ΪҶ�ڵ�

	BVHNode(std::vector<Object*>& objects, int start, int end);
};

class BVHTree {
public:
	BVHNode* root;
	BVHTree(std::vector<Object*>& objects) { root = new BVHNode(objects, 0, objects.size()); }

	bool intersect(const Ray& ray, Intersection& intersection) const;
	void depthInfo(BVHNode* root, int depth, int& maxdepth, int& alldepth)const;
};
