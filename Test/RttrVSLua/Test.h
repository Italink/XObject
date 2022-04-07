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
		//std::cout << "Construct Base 0 " << std::endl;
	};

	~Base() {
		//std::cout << "Destruct Base" << std::endl;
	}

	XFUNCTION() void print(int x) {
		std::cout << "print function parameter: " << x << std::endl;
	}

	XFUNCTION() static int add(int a, int b) {
		return a + b;
	}

	XFUNCTION() void setVar(int new_var) {
		var = new_var;
	}

	XFUNCTION() int getVar() {
		return var;
	}

	XPROPERTY() int var;
};

#endif // Test_h__