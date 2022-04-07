#ifndef HeaderParser_h__
#define HeaderParser_h__

#include "ParserBase.h"
#include "DataDef.h"
#include <map>

class SymbolParser : public ParserBase
{
public:
	SymbolParser() {}
	bool parse(FileDataDef& fileData, const Symbols& symbols);
private:
	/**
	 *  解析自己定义的标记
	 */
	FunctionDef parseXFunction(ClassDef* cdef = nullptr);
	PropertyDef parseXProperty();
	EnumDef parseXEnum();


	/**
	 *  下方提供了一系列的便捷解析函数
	 */
	bool parseClassHead(ClassDef* def);

	Type parseType();

	bool parseEnum(EnumDef* def);

	bool parseFunction(FunctionDef* def, bool inMacro = false, ClassDef* cdef = nullptr);

	bool parseMaybeFunction(const ClassDef* cdef, FunctionDef* def);

	void parseFunctionArguments(FunctionDef* def);

	std::string lexemUntil(Token);

	bool until(Token);

	bool skipCxxAttributes();

	inline bool inClass(const ClassDef* def) const {
		return mIndex > def->begin && mIndex < def->end - 1;
	}

	inline bool inNamespace(const NamespaceDef* def) const {
		return mIndex > def->begin && mIndex < def->end - 1;
	}
};

#endif // HeaderParser_h__
