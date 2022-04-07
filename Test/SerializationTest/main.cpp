#include "Test.h"
#include <iostream>
#include <vector>

int main() {
	Teacher teacher;
	teacher.name = "Teacher A";

	Student A;
	A.name = "Student A";
	A.reportCards["Card 0"] = { ReportCard::Level::A ,95 };
	A.reportCards["Card 1"] = { ReportCard::Level::B ,75 };

	Student B;
	B.name = "Student B";
	B.reportCards["Card 2"] = { ReportCard::Level::C ,65 };
	B.reportCards["Card 3"] = { ReportCard::Level::B ,80 };

	teacher.students.push_back(&A);
	teacher.students.push_back(&B);
	std::cout << std::setw(4) << teacher.dump() << std::endl << std::endl;

	std::vector<uint8_t> data = teacher.serialize();		//�������л�

	std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
	std::cout << "sizeof(Teacher):" << sizeof(Teacher) << std::endl;
	std::cout << "sizeof(Student):" << sizeof(Student) << std::endl;
	std::cout << "sizeof(ReportCard):" << sizeof(ReportCard) << std::endl;

	/*���л�ֻ�����˹ؼ����ݣ����õ��Ķ��������ݴ�С���������ͳߴ�������С */
	std::cout << "Teacher A byte size of serialize data : " << data.size() << std::endl;
	std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl << std::endl;

	Teacher* newTeacher = dynamic_cast<Teacher*>(XObject::createByData(data));	 //���л������µ�XObject����
	newTeacher->name = "Teacher B";
	std::cout << std::setw(4) << newTeacher->dump() << std::endl << std::endl;

	newTeacher->students.pop_back();			   // ���ﵯ��һ��XObject���������Զ��ͷš�
	newTeacher->students.push_back(new Student{}); // �����½���һ��Student

	teacher.deserialize(newTeacher->serialize());										//teacher���Ѿ���������student����ʱ���л�ʱ�����½�student
	std::cout << std::setw(4) << teacher.dump() << std::endl << std::endl;				//���Ƕ��Ѵ��ڵ�student�������л�����

	return 0;
}