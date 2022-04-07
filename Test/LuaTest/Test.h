#ifndef Test_h__
#define Test_h__

#include "XObject.h"
#include <iostream>
#include <vector>
#include <unordered_map>

class Base : public XObject {
	XENTRY()
public:
	XFUNCTION() Base() {
		std::cout << "Construct Base 0 " << std::endl;
	};
	XFUNCTION() Base(int x) :var(x) {
		std::cout << "Construct Base 1 " << std::endl;
	};

	~Base() {
		std::cout << "Destruct Base" << std::endl;
	}

	XFUNCTION() void print() {
		std::cout << "print member variable: " << var << std::endl;
	}
	XFUNCTION() void print(int x) {
		std::cout << "print function parameter: " << x << std::endl;
	}
	size_t size() {								//ʵ�ָú���sol���Զ��󶨵�Lua��#
		return std_vector.size();
	}

	bool operator <(const Base& other) const {	//��������ػᱻsol�Զ��󶨵�Lua
		return var < other.var;
	}

	XPROPERTY() int var = -1;					//sol ���Զ���std���������ͣ��Զ���������Ҫ����һЩģ��
	XPROPERTY() std::vector<std::string> std_vector = { "one","two","three" };
	XPROPERTY() std::unordered_map<std::string, int> std_map = { {"a",1},{"b",2},{"c",3},{"d",4} };

	XENUM() enum class Alignment {				//�������Լ�������չ
		Left = 0x0001,							//��Base��Ԫ����ע����һ��map<string,Alignment>�ľ�ָ̬��
		Right = 0x0002,
		Top = 0x0003,
		Bottom = 0x0004,
	};
};

class Derive : public Base {
	XENTRY()
public:
	Derive() { std::cout << "Construct Derive " << std::endl; }
	~Derive() {
		delete local_base;
		std::cout << "Destruct Derive " << std::endl;
	}
	XPROPERTY() Base* local_base = new Base(-1111);	   //���
};

class Derive1 : public Derive {
	XENTRY()
public:
	Derive1() { std::cout << "Construct Derive1 " << std::endl; }
	~Derive1() { std::cout << "Destruct Derive1 " << std::endl; }
};

XFUNCTION() inline int add(int a, int b) {
	return a + b;
}

XENUM() enum class GlobalEnum { One, Two };

#endif // Test_h__