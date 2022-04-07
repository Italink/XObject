#ifndef DataDef_h__
#define DataDef_h__

#include <string>
#include <map>
#include <list>
#include <vector>
#include "Token.h"

struct Type
{
	enum ReferenceType { NoReference, Reference, RValueReference, Pointer };

	inline Type() : isVolatile(false), isScoped(false), firstToken(NOTOKEN), referenceType(NoReference) {}
	inline explicit Type(const std::string& _name)
		: name(_name), rawName(name), isVolatile(false), isScoped(false), firstToken(NOTOKEN), referenceType(NoReference) {}
	std::string name;
	//When used as a return type, the type name may be modified to remove the references.
	// rawName is the type as found in the function signature
	std::string rawName;
	unsigned int isVolatile : 1;
	unsigned int isScoped : 1;
	Token firstToken;
	ReferenceType referenceType;
};

struct ClassDef;

struct EnumDef {
	std::string name;
	std::string enumName;
	std::vector<std::string> values;
	bool isEnumClass; // c++11 enum class
	EnumDef() : isEnumClass(false) {}
};

struct ArgumentDef
{
	ArgumentDef() : isDefault(false) {}
	Type type;
	std::string rightType, normalizedType, name;
	std::string typeNameForCast; // type name to be used in cast from void * in metacall
	std::string defaultValueString;
	bool isDefault;
};

struct FunctionDef
{
	Type returnType;
	std::vector<ArgumentDef> arguments;
	std::string normalizedType;
	std::string tag;
	std::string name;
	std::string inPrivateClass;

	enum class Access {
		Private, Protected, Public
	}access = Access::Private;

	bool isConst = false;
	bool isVirtual = false;
	bool isStatic = false;
	bool inlineCode = false;
	bool wasCloned = false;
	bool isConstructor = false;
	bool isDestructor = false;
	bool isAbstract = false;
	bool isAxonFunction = false;
};

struct PropertyDef {
	Type type;
	std::string name;
	std::string getter;
	std::string setter;
};

struct BaseDef {
	std::string classname;
	std::string qualified;
	std::vector<EnumDef> enumList;
	std::map<std::string, std::string> flagAliases;
	int begin = 0;
	int end = 0;
};

struct ClassDef : BaseDef {
	bool hasXEntry = false;
	std::vector<std::pair<std::string, FunctionDef::Access>> superclassList;
	std::vector<FunctionDef> functionList;
	std::vector<PropertyDef> propertyList;
};

struct NamespaceDef : BaseDef {
	bool doGenerate = false;
};

struct FileDataDef {
	std::string inputFilePath;						//输入文件的路径
	std::vector<std::string> inputIncludeDir;		//输入文件的包含目录

	std::map<std::string, std::vector<std::pair<std::string, FunctionDef::Access>>> superclass;		
	std::list<ClassDef> classList;				    //当前文件中的class信息
	std::list<NamespaceDef> namespaceList;			
	std::vector<FunctionDef> globalMethodList;		//全局函数
	std::vector<EnumDef> globaleEnumList;			//全局枚举

	std::string outputPath;							//输出文件的路径
};

#endif // DataDef_h__
