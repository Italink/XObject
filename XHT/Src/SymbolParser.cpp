#include "SymbolParser.h"
#include "Utils.h"
#include <iterator>
#include <sstream>
#include <filesystem>
#include <regex>

bool SymbolParser::parse(FileDataDef& fileData, const Symbols& symbols) {
	if (symbols.empty())
		return false;
	this->mSymbols = symbols;
	bool templateClass = false;
	int includeStack = 0;
	while (hasNext()) {
		Token t = next();
		switch (t) {
		case NAMESPACE: {
			int rewind = mIndex;
			if (test(IDENTIFIER)) {
				std::string nsName = lexem();
				std::vector<std::string> nested;
				while (test(SCOPE)) {
					next(IDENTIFIER);
					nested.push_back(nsName);
					nsName = lexem();
				}
				if (test(EQ)) {
					// namespace Foo = Bar::Baz;
					until(SEMIC);
				}
				else if (test(LPAREN)) {
					// Ignore invalid code such as: 'namespace __identifier("x")' (QTBUG-56634)
					until(RPAREN);
				}
				else if (!test(SEMIC)) {
					NamespaceDef def;
					def.classname = nsName;
					def.doGenerate = mCurrentFilenames.size() <= 1;

					next(LBRACE);
					def.begin = mIndex - 1;
					until(RBRACE);
					def.end = mIndex;
					mIndex = def.begin + 1;

					for (auto& it = fileData.namespaceList.rbegin(); it != fileData.namespaceList.rend(); ++it) {
						if (inNamespace(&*it))
							def.qualified = it->classname + "::" + def.qualified;
					}

					for (const std::string& ns : nested) {
						NamespaceDef parentNs;
						parentNs.classname = ns;
						parentNs.qualified = def.qualified;
						def.qualified += std::string(ns) + "::";
						parentNs.begin = def.begin;
						parentNs.end = def.end;
						fileData.namespaceList.push_back(parentNs);
					}

					while (inNamespace(&def) && hasNext()) {
						switch (next()) {
						case NAMESPACE:
						if (test(IDENTIFIER)) {
							while (test(SCOPE))
								next(IDENTIFIER);
							if (test(EQ)) {
								// namespace Foo = Bar::Baz;
								until(SEMIC);
							}
							else if (!test(SEMIC)) {
								until(RBRACE);
							}
						}
						break;
						case ENUM: {
							EnumDef enumDef;
							if (parseEnum(&enumDef))
								def.enumList.push_back(enumDef);
						} break;
						case CLASS:
						case STRUCT: {
							ClassDef classdef;
							if (!parseClassHead(&classdef))
								continue;
							while (inClass(&classdef) && hasNext())
								next(); // consume all Q_XXXX macros from this class
						} break;
						default: break;
						}
					}
					fileData.namespaceList.push_back(def);
					mIndex = rewind;
				}
			}
			break;
		}
		case SEMIC:
		case RBRACE:
		templateClass = false;
		break;
		case TEMPLATE:
		templateClass = true;
		break;
		case AXON_INCLUDE_BEGIN:
		++includeStack;
		break;
		case AXON_INCLUDE_END:
		--includeStack;
		break;

		case USING:
		if (test(NAMESPACE)) {
			while (test(SCOPE) || test(IDENTIFIER))
				;
			if (test(LPAREN))
				until(RPAREN);
			next(SEMIC);
		}
		break;
		case XENUM_TOKEN:
		fileData.globaleEnumList.push_back(parseXEnum());
		break;
		case XFUNCTION_TOKEN:
		fileData.globalMethodList.push_back(parseXFunction());
		break;
		case CLASS:
		case STRUCT: {
			ClassDef def;
			if (parseClassHead(&def)) {
				FunctionDef::Access access = FunctionDef::Access::Private;
				for (auto& it = fileData.namespaceList.rbegin(); it != fileData.namespaceList.rend(); ++it) {
					if (inNamespace(&*it))
						def.qualified = it->classname + "::" + def.qualified;
				}
				fileData.superclass[def.classname] = def.superclassList;
				if (includeStack != 0)
					break;
				while (inClass(&def) && hasNext()) {
					switch (next()) {
					case PRIVATE:
					access = FunctionDef::Access::Private;
					break;
					case PROTECTED:
					access = FunctionDef::Access::Protected;
					break;
					case PUBLIC:
					access = FunctionDef::Access::Public;
					break;
					case CLASS: {
						ClassDef nestedDef;
						if (parseClassHead(&nestedDef)) {
							while (inClass(&nestedDef) && inClass(&def)) {
								next();
							}
						}
					} break;
					case XENTRY_TOKEN:
					def.hasXEntry = true;
					break;
					case XFUNCTION_TOKEN: {
						def.functionList.push_back(parseXFunction(&def));
						break;
					}
					case XPROPERTY_TOKEN:
					def.propertyList.push_back(parseXProperty());
					break;
					case XENUM_TOKEN:
					def.enumList.push_back(parseXEnum());
					break;
					case SEMIC:
					case COLON:
					break;
					default:
					FunctionDef funcDef;
					funcDef.access = access;
					int rewind = mIndex--;
					if (parseMaybeFunction(&def, &funcDef)) {
						if (access == FunctionDef::Access::Public) {
							def.functionList.push_back(funcDef);
							if (funcDef.isConstructor) {
								while (funcDef.arguments.size() > 0 && funcDef.arguments.back().isDefault) {
									funcDef.wasCloned = true;
									funcDef.arguments.pop_back();
									def.functionList.push_back(funcDef);
								}
							}
						}
					}
					else {
						mIndex = rewind;
					}
					}
				}
				next(RBRACE);
				if (def.hasXEntry)
					fileData.classList.push_back(def);
			}
		}
		default:;
		}
	}
	return true;
}

bool SymbolParser::parseClassHead(ClassDef* def)
{
	// figure out whether this is a class declaration, or only a
	// forward or variable declaration.
	int i = 0;
	Token token;
	do {
		token = lookup(i++);
		if (token == COLON || token == LBRACE)
			break;
		if (token == SEMIC || token == RANGLE)
			return false;
	} while (token);

	if (!test(IDENTIFIER)) // typedef struct { ... }
		return false;

	std::string name = lexem();

	// support "class IDENT name" and "class IDENT(IDENT) name"
	// also support "class IDENT name (final|sealed|Q_DECL_FINAL)"
	if (test(LPAREN)) {
		until(RPAREN);
		if (!test(IDENTIFIER))
			return false;
		name = lexem();
	}
	else  if (test(IDENTIFIER)) {
		const std::string lex = lexem();
		if (lex != "final" && lex != "sealed" && lex != "Q_DECL_FINAL")
			name = lex;
	}

	def->qualified += name;
	while (test(SCOPE)) {
		def->qualified += lexem();
		if (test(IDENTIFIER)) {
			name = lexem();
			def->qualified += name;
		}
	}
	def->classname = name;

	if (test(IDENTIFIER)) {
		const std::string lex = lexem();
		if (lex != "final" && lex != "sealed" && lex != "Q_DECL_FINAL")
			return false;
	}

	if (test(COLON)) {
		do {
			test(VIRTUAL);
			FunctionDef::Access access = FunctionDef::Access::Public;
			if (test(PRIVATE))
				access = FunctionDef::Access::Private;
			else if (test(PROTECTED))
				access = FunctionDef::Access::Protected;
			else
				test(PUBLIC);
			test(VIRTUAL);
			const std::string type = parseType().name;
			// ignore the 'class Foo : BAR(Baz)' case
			if (test(LPAREN)) {
				until(RPAREN);
			}
			else {
				def->superclassList.push_back(std::make_pair(type, access));
			}
		} while (test(COMMA));
	}
	if (!test(LBRACE))
		return false;
	def->begin = mIndex - 1;
	bool foundRBrace = until(RBRACE);
	def->end = mIndex;
	mIndex = def->begin + 1;
	return foundRBrace;
}

Type SymbolParser::parseType()
{
	Type type;
	bool hasSignedOrUnsigned = false;
	bool isVoid = false;
	type.firstToken = lookup();
	for (;;) {
		skipCxxAttributes();
		switch (next()) {
		case SIGNED:
		case UNSIGNED:
		hasSignedOrUnsigned = true;
		FALLTHROUGH();
		case CONST:
		case VOLATILE:
		type.name += lexem();
		type.name += ' ';
		if (lookup(0) == VOLATILE)
			type.isVolatile = true;
		continue;
		case NOTOKEN:
		return type;
		default:
		prev();
		break;
		}
		break;
	}

	skipCxxAttributes();
	test(ENUM) || test(CLASS) || test(STRUCT);
	for (;;) {
		skipCxxAttributes();
		switch (next()) {
		case IDENTIFIER:
		// void mySlot(unsigned myArg)
		if (hasSignedOrUnsigned) {
			prev();
			break;
		}
		FALLTHROUGH();
		case CHAR:
		case SHORT:
		case INT:
		case LONG:
		type.name += lexem();
		// preserve '[unsigned] long long', 'short int', 'long int', 'long double'
		if (test(LONG) || test(INT) || test(DOUBLE)) {
			type.name += ' ';
			prev();
			continue;
		}
		break;
		case FLOAT:
		case DOUBLE:
		case VOID:
		case BOOL:
		case AUTO:
		type.name += lexem();
		isVoid |= (lookup(0) == VOID);
		break;
		case NOTOKEN:
		return type;
		default:
		prev();
		;
		}
		if (test(LANGLE)) {
			if (type.name.empty()) {
				// '<' cannot start a type
				return type;
			}
			type.name += lexemUntil(RANGLE);
		}
		if (test(SCOPE)) {
			type.name += lexem();
			type.isScoped = true;
		}
		else {
			break;
		}
	}
	while (test(CONST) || test(VOLATILE) || test(SIGNED) || test(UNSIGNED)
		|| test(STAR) || test(AND) || test(ANDAND)) {
		type.name += ' ';
		type.name += lexem();
		if (lookup(0) == AND)
			type.referenceType = Type::Reference;
		else if (lookup(0) == ANDAND)
			type.referenceType = Type::RValueReference;
		else if (lookup(0) == STAR)
			type.referenceType = Type::Pointer;
	}
	type.rawName = type.name;
	// transform stupid things like 'const void' or 'void const' into 'void'
	if (isVoid && type.referenceType == Type::NoReference) {
		type.name = "void";
	}
	return type;
}

enum class IncludeState {
	IncludeBegin,
	IncludeEnd,
	NoInclude,
};

bool SymbolParser::parseEnum(EnumDef* def)
{
	bool isTypdefEnum = false; // typedef enum { ... } Foo;

	if (test(CLASS) || test(STRUCT))
		def->isEnumClass = true;

	if (test(IDENTIFIER)) {
		def->name = lexem();
	}
	else {
		if (lookup(-1) != TYPEDEF)
			return false; // anonymous enum
		isTypdefEnum = true;
	}
	if (test(COLON)) { // C++11 strongly typed enum
		// enum Foo : unsigned long { ... };
		parseType(); //ignore the result
	}
	if (!test(LBRACE))
		return false;
	do {
		if (lookup() == RBRACE) // accept trailing comma
			break;
		next(IDENTIFIER);
		def->values.push_back(lexem());
		skipCxxAttributes();
	} while (test(EQ) ? until(COMMA) : test(COMMA));
	next(RBRACE);
	if (isTypdefEnum) {
		if (!test(IDENTIFIER))
			return false;
		def->name = lexem();
	}
	return true;
}

void SymbolParser::parseFunctionArguments(FunctionDef* def)
{
	while (hasNext()) {
		ArgumentDef  arg;
		arg.type = parseType();
		if (arg.type.name == "void")
			break;
		if (test(IDENTIFIER))
			arg.name = lexem();
		while (test(LBRACK)) {
			arg.rightType += lexemUntil(RBRACK);
		}
		if (test(CONST) || test(VOLATILE)) {
			arg.rightType += ' ';
			arg.rightType += lexem();
		}
		arg.normalizedType = std::string(arg.type.name + ' ' + arg.rightType);
		arg.typeNameForCast = std::string(noRef(arg.type.name) + "(*)" + arg.rightType);
		if (test(EQ)) {
			arg.isDefault = true;
			//int parenStack = 0;
			//while (true) {
			//	Token nextToken = next();
			//	if (nextToken == COMMA || (parenStack == 0 && nextToken == RPAREN)) {
			//		prev();
			//		break;
			//	}
			//	arg.defaultValueString += lexem();
			//}
		}
		def->arguments.push_back(arg);
		if (!until(COMMA))
			break;
	}
}

bool SymbolParser::skipCxxAttributes()
{
	auto rewind = mIndex;
	if (test(LBRACK) && test(LBRACK) && until(RBRACK) && test(RBRACK))
		return true;
	mIndex = rewind;
	return false;
}

// returns false if the function should be ignored
bool SymbolParser::parseFunction(FunctionDef* def, bool inMacro, ClassDef* cdef)
{
	def->isVirtual = false;
	def->isStatic = false;
	//skip modifiers and attributes
	while (test(INLINE) || (test(STATIC) && (def->isStatic = true) == true) ||
		(test(VIRTUAL) && (def->isVirtual = true) == true) //mark as virtual
		|| skipCxxAttributes()) {
	}
	bool templateFunction = (lookup() == TEMPLATE);
	bool tilde = test(TILDE);
	def->returnType = parseType();
	if (def->returnType.name.empty()) {
		if (templateFunction)
			error("Template function as signal or slot");
		else
			error();
	}
	bool scopedFunctionName = false;
	if (test(LPAREN)) {
		def->name = def->returnType.name;
		scopedFunctionName = def->returnType.isScoped;
		if (cdef && def->name == cdef->classname) {
			def->isDestructor = tilde;
			def->isConstructor = !tilde;
			def->returnType = Type();
		}
		else {
			def->returnType = Type("int");
		}
	}
	else {
		Type tempType = parseType();;
		while (!tempType.name.empty() && lookup() != LPAREN) {
			if (!def->tag.empty())
				def->tag += ' ';
			def->tag += def->returnType.name;

			def->returnType = tempType;
			tempType = parseType();
		}
		next(LPAREN, "Not a signal or slot declaration");
		def->name = tempType.name;
		scopedFunctionName = tempType.isScoped;
	}

	if (!test(RPAREN)) {
		parseFunctionArguments(def);
		next(RPAREN);
	}

	// support optional macros with compiler specific options
	while (test(IDENTIFIER));

	def->isConst = test(CONST);

	if (cdef)
		return true;

	while (test(IDENTIFIER))
		;

	if (inMacro) {
		next(RPAREN);
		prev();
	}
	else {
		if (test(THROW)) {
			next(LPAREN);
			until(RPAREN);
		}

		if (def->returnType.name == "auto" && test(ARROW))
			def->returnType = parseType(); // Parse trailing return-type

		if (test(SEMIC))
			;
		else if ((def->inlineCode = test(LBRACE)))
			until(RBRACE);
		else if ((def->isAbstract = test(EQ)))
			until(SEMIC);
		else if (skipCxxAttributes())
			until(SEMIC);
		else
			error();
	}
	if (scopedFunctionName) {
		const std::string msg = "Function declaration " + def->name
			+ " contains extra qualification. Ignoring as signal or slot.";
		warning(msg.c_str());
		return false;
	}

	std::istringstream iss(def->returnType.name);
	std::vector<std::string> typeNameParts{ std::istream_iterator<std::string>(iss),std::istream_iterator<std::string>{} };
	if (std::find(typeNameParts.begin(), typeNameParts.end(), "auto") != typeNameParts.end()) {
		// We expected a trailing return type but we haven't seen one
		error("Function declared with auto as return type but missing trailing return type. "
			"Return type deduction is not supported.");
	}
	// we don't support references as return types, it's too dangerous
	if (def->returnType.referenceType == Type::Reference) {
		std::string rawName = def->returnType.rawName;
		def->returnType = Type("void");
		def->returnType.rawName = rawName;
	}

	def->normalizedType = def->returnType.name;
	return true;
}

// like parseFunction, but never aborts with an error
bool SymbolParser::parseMaybeFunction(const ClassDef* cdef, FunctionDef* def)
{
	def->isVirtual = false;
	def->isStatic = false;
	//skip modifiers and attributes
	while (test(EXPLICIT) || test(INLINE) || (test(STATIC) && (def->isStatic = true) == true) ||
		(test(VIRTUAL) && (def->isVirtual = true) == true) //mark as virtual
		|| skipCxxAttributes()) {
	}
	bool tilde = test(TILDE);
	def->returnType = parseType();
	if (def->returnType.name.empty())
		return false;

	if (test(LPAREN)) {
		def->name = def->returnType.name;
		if (def->name == cdef->classname) {
			def->isDestructor = tilde;
			def->isConstructor = !tilde;
			def->returnType = Type();
		}
		else {
			def->returnType = Type("int");
		}
	}
	else {
		Type tempType = parseType();;
		while (!tempType.name.empty() && lookup() != LPAREN) {
			if (!def->tag.empty())
				def->tag += ' ';
			def->tag += def->returnType.name;
			def->returnType = tempType;
			tempType = parseType();
		}
		if (!test(LPAREN))
			return false;
		def->name = tempType.name;
	}

	// we don't support references as return types, it's too dangerous
	if (def->returnType.referenceType == Type::Reference) {
		std::string rawName = def->returnType.rawName;
		def->returnType = Type("void");
		def->returnType.rawName = rawName;
	}

	def->normalizedType = def->returnType.name;

	if (!test(RPAREN)) {
		parseFunctionArguments(def);
		if (!test(RPAREN))
			return false;
	}
	if (def->isConstructor && lookup(1) == COLON) {
		next(COLON);
		int counter = 0;
		do {
			Token next_ = next();
			if (next_ == RBRACE)
				counter--;
			else if (next_ == LBRACE)
				counter++;
		} while (counter);
	}

	def->isConst = test(CONST);
	return true;
}

FunctionDef SymbolParser::parseXFunction(ClassDef* cdef)
{
	next(LPAREN);
	next(RPAREN);
	FunctionDef funDef;
	funDef.isAxonFunction = true;
	parseFunction(&funDef, false, cdef);
	return funDef;
}

PropertyDef SymbolParser::parseXProperty()
{
	PropertyDef axVarDef;
	next(LPAREN);
	while (test(IDENTIFIER)) {
		std::string type = lexem();
		next();
		if (type == "GET") {
			axVarDef.getter = lexem();
		}
		else if (type == "SET") {
			axVarDef.setter = lexem();
		}
	}
	next(RPAREN);
	axVarDef.type = parseType();
	next(IDENTIFIER);
	axVarDef.name = lexem();
	until(SEMIC);
	return axVarDef;
}

EnumDef SymbolParser::parseXEnum()
{
	EnumDef enumdef;
	until(ENUM);
	parseEnum(&enumdef);
	return enumdef;
}

std::string SymbolParser::lexemUntil(Token target)
{
	int from = mIndex;
	until(target);
	std::string s;
	while (from <= mIndex) {
		std::string n = mSymbols.at(from++ - 1).lexem();
		if (s.size() && n.size()) {
			char prev = s.at(s.size() - 1);
			char next = n.at(0);
			if ((is_ident_char(prev) && is_ident_char(next))
				|| (prev == '<' && next == ':')
				|| (prev == '>' && next == '>'))
				s += ' ';
		}
		s += n;
	}
	return s;
}

bool SymbolParser::until(Token target) {
	int braceCount = 0;
	int brackCount = 0;
	int parenCount = 0;
	int angleCount = 0;
	if (mIndex) {
		switch (mSymbols.at(mIndex - 1).token) {
		case LBRACE: ++braceCount; break;
		case LBRACK: ++brackCount; break;
		case LPAREN: ++parenCount; break;
		case LANGLE: ++angleCount; break;
		default: break;
		}
	}

	//when searching commas within the default argument, we should take care of template depth (anglecount)
	// unfortunatelly, we do not have enough semantic information to know if '<' is the operator< or
	// the beginning of a template type. so we just use heuristics.
	int possible = -1;

	while (mIndex < mSymbols.size()) {
		Token t = mSymbols.at(mIndex++).token;
		switch (t) {
		case LBRACE: ++braceCount; break;
		case RBRACE: --braceCount; break;
		case LBRACK: ++brackCount; break;
		case RBRACK: --brackCount; break;
		case LPAREN: ++parenCount; break;
		case RPAREN: --parenCount; break;
		case LANGLE:
		if (parenCount == 0 && braceCount == 0)
			++angleCount;
		break;
		case RANGLE:
		if (parenCount == 0 && braceCount == 0)
			--angleCount;
		break;
		case GTGT:
		if (parenCount == 0 && braceCount == 0) {
			angleCount -= 2;
			t = RANGLE;
		}
		break;
		default: break;
		}
		if (t == target
			&& braceCount <= 0
			&& brackCount <= 0
			&& parenCount <= 0
			&& (target != RANGLE || angleCount <= 0)) {
			if (target != COMMA || angleCount <= 0)
				return true;
			possible = mIndex;
		}

		if (target == COMMA && t == EQ && possible != -1) {
			mIndex = possible;
			return true;
		}

		if (braceCount < 0 || brackCount < 0 || parenCount < 0
			|| (target == RANGLE && angleCount < 0)) {
			--mIndex;
			break;
		}

		if (braceCount <= 0 && t == SEMIC) {
			// Abort on semicolon. Allow recovering bad template parsing (QTBUG-31218)
			break;
		}
	}

	if (target == COMMA && angleCount != 0 && possible != -1) {
		mIndex = possible;
		return true;
	}

	return false;
}