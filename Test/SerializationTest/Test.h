#ifndef Test_h__
#define Test_h__

#include "XObject.h"
#include <iostream>
#include <map>
#include <vector>
#include <string>

class Person : public XObject {
	XENTRY(XObject)
public:
	XFUNCTION() Person() { };

	XPROPERTY() std::string name;
};

struct ReportCard {
	enum Level {
		A = 0, B, C, D
	}level;
	int score;
	X_SERIALIZE_BIND(level, score);	//���ڷ�XObject��ʹ�ú� XSERIALIZE_BIND �� ���л�����
};

class Student : public Person {
	XENTRY(Person)
public:
	XFUNCTION() Student() { std::cout << "Construct Student  " << this << std::endl; };
	XPROPERTY() std::map<std::string, ReportCard> reportCards;
};

class Teacher : public Person {
	XENTRY(Person)
public:
	XFUNCTION() Teacher() { std::cout << "Construct Teacher  " << this << std::endl; };

	XPROPERTY() std::vector<Student*> students; //XObject���ɿ�������ΪPropertyʱֻ����ָ��
};

#endif // Test_h__ 