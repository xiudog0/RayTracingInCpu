#pragma once
#include "third/tinyobjloader/tiny_obj_loader.h"
#include <cmath>
#include <vector>

constexpr float PI = 3.1415926;
inline float floatrand(float max = 1) { return max * static_cast <float> (rand()) / static_cast <float> (RAND_MAX); }
inline float floatMax(const float f3[]) { return std::max(std::max(f3[0], f3[1]), f3[2]); }

struct Vec {
	float x, y, z;

	Vec(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
	Vec(float v[]) : x(v[0]), y(v[1]), z(v[2]) {}
	Vec(const float v[]) : x(v[0]), y(v[1]), z(v[2]) {}
	Vec operator+(const Vec& v) const { return Vec(x + v.x, y + v.y, z + v.z); }
	Vec operator+(const float v[]) const { return Vec(x + v[0], y + v[1], z + v[2]); }
	Vec operator-(const Vec& v) const { return Vec(x - v.x, y - v.y, z - v.z); }
	Vec operator*(const float s) const { return Vec(x * s, y * s, z * s); }
	float operator[](const int i) const { return i == 0 ? x : (i == 1 ? y : z); }

	float dot(const Vec& v) const { return x * v.x + y * v.y + z * v.z; }
	Vec cross(const Vec& v) const { return Vec(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x); }
	Vec mult(const float f[]) const { return Vec(x * f[0], y * f[1], z * f[2]); }
	Vec mult(const Vec f) const { return Vec(x * f[0], y * f[1], z * f[2]); }
	float length() const { return sqrt(x * x + y * y + z * z); }
	Vec normalized() const { float len = length(); return Vec(x / len, y / len, z / len); }
};

inline float vecMax(const Vec& v) { return std::max(std::max(v.x, v.y), v.z); }

struct Ray {
	Vec origin;
	Vec direction;
	float tMin, tMax;
	Ray(const Vec& origin = { 0,0,0 }, const Vec& direction = { 0,0,0 }, float tMin = 1e-6, float tMax = 1e10)
		: origin(origin), direction(direction), tMin(tMin), tMax(tMax) {}
	// 计算光线上距离原点为t的点
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
	float t; // 光线参数
	tinyobj::material_t* material;
	Object* object;
	Vec point; // 交点位置
	Vec normal; // 交点处的法向量

	// 默认构造函数
	Intersection() : t(INFINITY), material(nullptr), object(nullptr) {}

	// 交点比较函数，用于排序
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
	virtual float sampleLight(const Vec& point, const BVHTree& bvh) = 0;

	virtual float getArea() = 0;

	virtual objectType getType() = 0;

	virtual Vec getTextureByPoint(const Vec& point) = 0;

	tinyobj::material_t* material;
	Texture* texture;
};


class Sphere : public Object {
public:
	Vec center;
	float radius;

	Sphere(float f[3], float r, tinyobj::material_t* mat, bool comArea = false) :
		center(f), radius(r) {
		material = mat;
		texture = nullptr;
	}

	Sphere(const Vec v, float r, tinyobj::material_t* mat, bool comArea = false) :
		center(v), radius(r) {
		material = mat;
		texture = nullptr;
	}


	virtual AABB getBoundingBox()override;
	virtual bool intersect(const Ray& ray, Intersection& intersection) override;
	virtual float sampleLight(const Vec& point, const BVHTree& bvh) override;
	virtual float getArea() override;
	virtual Vec getTextureByPoint(const Vec& point) override;
	objectType getType() override { return objectType::sph; }
};

class Triangle : public Object {
public:
	Vec v0, v1, v2;
	Vec uv0, uv1, uv2;
	Triangle(float f0[3], float f1[3], float f2[3],Vec uvv0,Vec uvv1,Vec uvv2, tinyobj::material_t* mat, Texture* tex = nullptr) :
		v0(f0), v1(f1), v2(f2), uv0(uvv0), uv1(uvv1), uv2(uvv2) {
		material = mat;
		texture = tex;
	}

	Triangle(const Vec& v0, const Vec& v1, const Vec& v2,tinyobj::material_t* mat) :
		v0(v0), v1(v1), v2(v2){
		material = mat;
		texture = nullptr;
	}

	virtual AABB getBoundingBox()override;
	virtual bool intersect(const Ray& ray, Intersection& intersection) override;
	virtual float sampleLight(const Vec& point, const BVHTree& bvh) override;
	virtual float getArea()override;
	virtual Vec getTextureByPoint(const Vec& point) override;
	Vec getNormal() { return (v1 - v0).cross(v2 - v0).normalized(); }

	virtual objectType getType() override { return objectType::tri; }
};



class BVHNode {
public:
	AABB box; // AABB表示包围盒
	BVHNode* left; // 左子树
	BVHNode* right; // 右子树
	Object* object; // 对象指针
	bool isLeaf; // 是否为叶节点

	BVHNode(std::vector<Object*>& objects, int start, int end);
};

class BVHTree {
public:
	BVHNode* root;
	BVHTree(std::vector<Object*>& objects) { root = new BVHNode(objects, 0, objects.size()); }

	bool intersect(const Ray& ray, Intersection& intersection) const;
	void depthInfo(BVHNode* root, int depth, int& maxdepth, int& alldepth)const;
};

