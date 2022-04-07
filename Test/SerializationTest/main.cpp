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

	std::vector<uint8_t> data = teacher.serialize();		//对象序列化

	std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
	std::cout << "sizeof(Teacher):" << sizeof(Teacher) << std::endl;
	std::cout << "sizeof(Student):" << sizeof(Student) << std::endl;
	std::cout << "sizeof(ReportCard):" << sizeof(ReportCard) << std::endl;

	/*序列化只保留了关键数据，最后得到的二进制数据大小甚至比类型尺寸总量还小 */
	std::cout << "Teacher A byte size of serialize data : " << data.size() << std::endl;
	std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl << std::endl;

	Teacher* newTeacher = dynamic_cast<Teacher*>(XObject::createByData(data));	 //序列化生成新的XObject对象
	newTeacher->name = "Teacher B";
	std::cout << std::setw(4) << newTeacher->dump() << std::endl << std::endl;

	newTeacher->students.pop_back();			   // 这里弹出一个XObject，并不会自动释放。
	newTeacher->students.push_back(new Student{}); // 这里新建了一个Student

	teacher.deserialize(newTeacher->serialize());										//teacher中已经存在两个student，此时序列化时不会新建student
	std::cout << std::setw(4) << teacher.dump() << std::endl << std::endl;				//而是对已存在的student进行序列化操作

	return 0;
}