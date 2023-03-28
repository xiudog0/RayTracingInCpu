#include "bvh.h"

#include <stack>
#include <algorithm>


bool AABB::intersect(const Ray& ray) const {

	float tmin = (min.x - ray.origin.x) / ray.direction.x;
	float tmax = (max.x - ray.origin.x) / ray.direction.x;
	if (tmin > tmax) std::swap(tmin, tmax);

	float tymin = (min.y - ray.origin.y) / ray.direction.y;
	float tymax = (max.y - ray.origin.y) / ray.direction.y;
	if (tymin > tymax) std::swap(tymin, tymax);

	if ((tmin > tymax) || (tymin > tmax)) {
		return false;
	}

	if (tymin > tmin) {
		tmin = tymin;
	}

	if (tymax < tmax) {
		tmax = tymax;
	}

	float tzmin = (min.z - ray.origin.z) / ray.direction.z;
	float tzmax = (max.z - ray.origin.z) / ray.direction.z;
	if (tzmin > tzmax) std::swap(tzmin, tzmax);

	if ((tmin > tzmax) || (tzmin > tmax)) {
		return false;
	}

	if (tzmin > tmin) {
		tmin = tzmin;
	}

	if (tzmax < tmax) {
		tmax = tzmax;
	}

	return (tmin < ray.tMax) && (tmax > 0);
}


AABB AABB::merge(const AABB& a, const AABB& b) {
	// 合并两个AABB包围盒
	Vec min(fmin(a.min.x, b.min.x), fmin(a.min.y, b.min.y), fmin(a.min.z, b.min.z));
	Vec max(fmax(a.max.x, b.max.x), fmax(a.max.y, b.max.y), fmax(a.max.z, b.max.z));
	return AABB(min, max);
}


BVHNode::BVHNode(std::vector<Object*>& objects, int start, int end) {
	// 构造函数，输入为对象数组和起始、结束位置
	int axis = rand() % 3; // 随机选择分割轴
	int numTriangles = end - start;

	if (numTriangles == 1) {
		// 如果只有一个对象，直接将该对象作为叶节点
		isLeaf = true;
		object = objects[start];
		box = object->getBoundingBox();
		left = right = nullptr;
	}
	else if (numTriangles == 2) {
		// 如果只有两个对象，将它们作为左右子树
		isLeaf = false;
		left = new BVHNode(objects, start, start + 1);
		right = new BVHNode(objects, start + 1, end);
		box = AABB::merge(left->box, right->box);
	}
	else {
		// 如果有多个对象，按照分割轴对它们进行排序
		// 第三个参数为lambda表达式
		std::sort(objects.begin() + start, objects.begin() + end, [axis](Object* a, Object* b) {
			return a->getBoundingBox().min[axis] < b->getBoundingBox().min[axis];
			});

		// 将对象分成两组，分别构造左右子树
		int mid = start + numTriangles / 2;
		left = new BVHNode(objects, start, mid);
		right = new BVHNode(objects, mid, end);
		box = AABB::merge(left->box, right->box);
		isLeaf = false;
	}
}


bool BVHTree::intersect(const Ray& ray, Intersection& intersection) const {
	// 判断光线与BVH树的交点
	if (!root->box.intersect(ray)) {
		return false;
	}

	bool hit = false;
	std::stack<BVHNode*> stack;
	stack.push(root);
	while (!stack.empty()) {
		BVHNode* node = stack.top();
		stack.pop();

		if (node->isLeaf) {
			// 如果是叶节点，检查光线是否与对象相交
			//printf("leaf\n");
			Intersection temp;
			if (node->object->intersect(ray, temp)) {
				if (temp < intersection && temp.t >1e-3) {
					intersection = temp;
					hit = true;
				}
			}
		}
		else {
			// 如果不是叶节点，将左右子树加入栈中
			if (node->left->box.intersect(ray)) {
				stack.push(node->left);
			}
			if (node->right->box.intersect(ray)) {
				stack.push(node->right);
			}
		}
	}
	return hit;
}

void BVHTree::depthInfo(BVHNode* root, int depth, int& maxdepth, int& alldepth) const
{
	if (root == nullptr)
		return;

	if (depth > maxdepth)
		maxdepth = depth;

	depthInfo(root->left, depth + 1, maxdepth, alldepth);
	depthInfo(root->right, depth + 1, maxdepth, alldepth);

	if(root->left == nullptr  && root->right == nullptr)
		alldepth += depth;
	return;
}


AABB Triangle::getBoundingBox()
{
	AABB box;
	box.min.x = std::min(std::min(this->v0[0], this->v1[0]), this->v2[0]);
	box.min.y = std::min(std::min(this->v0[1], this->v1[1]), this->v2[1]);
	box.min.z = std::min(std::min(this->v0[2], this->v1[2]), this->v2[2]);
	box.max.x = std::max(std::max(this->v0[0], this->v1[0]), this->v2[0]);
	box.max.y = std::max(std::max(this->v0[1], this->v1[1]), this->v2[1]);
	box.max.z = std::max(std::max(this->v0[2], this->v1[2]), this->v2[2]);
	return box;
}


bool Triangle::intersect(const Ray& ray, Intersection& intersection)
{
	const Vec e1 = v1 - v0;
	const Vec e2 = v2 - v0;
	const Vec p = ray.direction.cross(e2);
	const float det = e1.dot(p);

	if (fabs(det) < 1e-6) {
		return false; // 光线与三角形共面，无交点
	}

	const float inv_det = 1.0f / det;
	const Vec t = ray.origin - v0;
	const float u = t.dot(p) * inv_det;

	if (u < 0.0f || u > 1.0f) {
		return false; // 光线与三角形不相交
	}

	const Vec q = t.cross(e1);
	const float v = ray.direction.dot(q) * inv_det;

	if (v < 0.0f || u + v > 1.0f) {
		return false; // 光线与三角形不相交
	}

	const float t_hit = e2.dot(q) * inv_det;

	if (t_hit < ray.tMin || t_hit > ray.tMax) {
		return false; // 光线与三角形不相交
	}

	// 更新Intersection对象的信息
	intersection.t = t_hit;
	intersection.matId = this->mat_id;
	intersection.point = ray.at(t_hit);
	intersection.normal = e1.cross(e2).normalized();
	intersection.object = this;

	return true;
}

float Triangle::sampleLight(Vec point, BVHTree& bvh)
{

	// get a random point
	Vec e01 = v1 - v0;
	Vec e02 = v2 - v0;

	float rand1 = floatrand();
	float rand2 = floatrand();
	if (rand1 + rand2 > 1) {
		rand1 = 1.0 - rand1;
		rand2 = 1.0 - rand2;
	}
	Vec randPoint = v0 + e01 * rand1 + e02 * rand2;

	// a line from object to interction
	Vec line = point - randPoint;

	Intersection inte;

	//not hit
	if (bvh.intersect(Ray(point, (line * -1.0).normalized()), inte) == false)
		return 0;
	float a = (inte.point - point).length();
	if(	(inte.point - randPoint).length() > 1e-6)
		return 0.0f;


	float distance = line.length();
	distance *= distance;
	double area = 0.5 * e01.cross(e02).length();
	float cos = std::abs(line.normalized().dot(inte.normal));

	return area * cos / distance;
}

float Triangle::getArea()
{
	Vec e01 = v1 - v0;
	Vec e02 = v2 - v0;
	Vec cross_product = e01.cross(e02);
	return 0.5f * cross_product.length();
}

Vec Triangle::getTextureByPoint(Vec point)
{
	if (texture == nullptr)
		return Vec(1, 1, 1);

	Vec e01 = v1 - v0, e02 = v2 - v0, e0p = point - v0;

	float d00 = e01.dot(e01);
	float d01 = e01.dot(e02);
	float d11 = e02.dot(e02);
	float d20 = e0p.dot(e01);
	float d21 = e0p.dot(e02);
	float denom = d00 * d11 - d01 * d01;

	float w0 = (d11 * d20 - d01 * d21) / denom;
	float w1 = (d00 * d21 - d01 * d20) / denom;
	float w2 = 1.0f - w0 - w1;

	// 计算纹理坐标
	float u = w0 * uv0.x + w1 * uv1.x + w2 * uv2.x;
	float v = w0 * uv0.y + w1 * uv1.y + w2 * uv2.y;



	int w = texture->w;
	int x = u * w;
	int y = v * texture->h;


	return Vec(
		texture->photo[texture->c * (x + y * w) + 0],
		texture->photo[texture->c * (x + y * w) + 1],
		texture->photo[texture->c * (x + y * w) + 2]
	) * (1.0 / 255.0);
}

AABB Sphere::getBoundingBox()
{
	AABB box;
	box.min.x = this->center[0] - radius;
	box.min.y = this->center[1] - radius;
	box.min.z = this->center[2] - radius;
	box.max.x = this->center[0] + radius;
	box.max.y = this->center[1] + radius;
	box.max.z = this->center[2] + radius;
	return box;
}


bool Sphere::intersect(const Ray& ray, Intersection& intersection) {

	Vec op = center - ray.origin;  // p-o
	// Solve t^2*d.d + 2*t*(o-p).d + (o-p).(o-p)-R^2 = 0
	// because of the d.d is 1,we can simplify the answer
	// answer: -b/2 - sqrt((b/2)*(b/2) - c)
	double t;
	double eps = 1e-4;
	double b = op.dot(ray.direction);// d*(p-o) ,actually, it is -b/2;
	// c = op . op - r*r
	double det = b * b - op.dot(op) + radius * radius;// actually, det / 4

	if (det < 0)
		return 0;
	else
		det = sqrt(det);

	if (b - det > eps) {
		intersection.t = b - det;
	}
	else if (b + det > eps) {
		intersection.t = b + det;
	}
	else {
		return false;
	}

	intersection.point = ray.origin + ray.direction * intersection.t;
	intersection.normal = (intersection.point - center).normalized();
	intersection.matId = this->mat_id;
	intersection.object = this;

	return true;
}

float Sphere::sampleLight(Vec point, BVHTree& bvh)
{
	// get a random point
	float phi = PI * floatrand();
	float sinPhi = std::sin(phi);
	float theta = 2 * PI * floatrand();
	Vec randPoint = center + Vec(sinPhi * std::cos(theta), sinPhi * std::sin(theta), std::cos(phi)) * radius;
	float _aaaa = (randPoint - center).length();
	Intersection inte;
	// a line from object to interction
	Vec line = point - randPoint;

	// not hit
	if (bvh.intersect(Ray(point, (line * -1.).normalized()), inte) == false)
		return 0;
	
	if (std::abs((inte.point - center).length() - radius) < 1e-3)
		return 0;

	float distance = line.length();
	distance *= distance;
	float area = PI * radius * radius;
	float cos = line.normalized().dot(inte.normal);

	return area * cos / (distance * 2 * PI);
}

float Sphere::getArea()
{
	return 105;
}

Vec Sphere::getTextureByPoint(Vec point)
{
	return Vec(1,1,1);
}

