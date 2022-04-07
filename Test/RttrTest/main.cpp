#include "Test.h"
#include <iostream>
#include <vector>

int main() {
	Base base;

	/*�����޲γ�Ա����*/
	base.invoke("print");

	/*�����вγ�Ա����*/
	rttr::variant var = base.invoke("add", { 1,7 });
	std::cout << var.to_int() << std::endl;

	/*ͨ��ʵ����ȡ����*/
	std::vector<std::string> dataByInstance = base.getProperty("std_vector").get_value<std::vector<std::string>>();

	/*ͨ��Ԫ�����ȡ����*/
	rttr::property prop = Base::staticMetaObject()->getProperty("std_vector");
	std::vector<std::string> dataByMetaObject = prop.get_value(base).get_value<std::vector<std::string>>();
	assert(!dataByInstance.empty() && std::equal(dataByInstance.begin(), dataByInstance.end(), dataByMetaObject.begin()));

	Derive derive;

	derive.invoke("print");

	Base* local_base = derive.getProperty("local_base").get_value<Base*>();
	local_base->print();

	return 0;
}