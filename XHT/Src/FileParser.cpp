#include "FileParser.h"
#include "Utils.h"
#include "Keywords.inl"
#include <regex>
#include <sstream>
#include <fstream>
#include <filesystem>

// transform \r\n into \n
// \r into \n (os9 style)
// backslash-newlines into newlines

static std::string cleaned(const std::string& input)
{
	const char* data = input.c_str();
	const char* end = input.c_str() + input.size();
	char* result = new char[input.size()];
	char* output = result;

	int newlines = 0;
	while (data != end) {
		while (data != end && is_space(*data))
			++data;
		bool takeLine = (*data == '#');
		if (*data == '%' && *(data + 1) == ':') {
			takeLine = true;
			++data;
		}
		if (takeLine) {
			*output = '#';
			++output;
			do ++data; while (data != end && is_space(*data));
		}
		while (data != end) {
			// handle \\\n, \\\r\n and \\\r
			if (*data == '\\') {
				if (*(data + 1) == '\r') {
					++data;
				}
				if (data != end && (*(data + 1) == '\n' || (*data) == '\r')) {
					++newlines;
					data += 1;
					if (data != end && *data != '\r')
						data += 1;
					continue;
				}
			}
			else if (*data == '\r' && *(data + 1) == '\n') { // reduce \r\n to \n
				++data;
			}
			if (data == end)
				break;

			char ch = *data;
			if (ch == '\r') // os9: replace \r with \n
				ch = '\n';
			*output = ch;
			++output;

			if (*data == '\n') {
				// output additional newlines to keep the correct line-numbering
				// for the lines following the backslash-newline sequence(s)
				while (newlines) {
					*output = '\n';
					++output;
					--newlines;
				}
				++data;
				break;
			}
			++data;
		}
	}
	std::string str(result, output - result);
	delete[] result;
	return str;
}

bool FileParser::preprocessOnly = false;

void FileParser::skipUntilEndif()
{
	while (mIndex < mSymbols.size() - 1 && mSymbols.at(mIndex).token != PP_ENDIF) {
		switch (mSymbols.at(mIndex).token) {
		case PP_IF:
		case PP_IFDEF:
		case PP_IFNDEF:
			++mIndex;
			skipUntilEndif();
			break;
		default:
			;
		}
		++mIndex;
	}
}

bool FileParser::skipBranch()
{
	while (mIndex < mSymbols.size() - 1
		&& (mSymbols.at(mIndex).token != PP_ENDIF
			&& mSymbols.at(mIndex).token != PP_ELIF
			&& mSymbols.at(mIndex).token != PP_ELSE)
		) {
		switch (mSymbols.at(mIndex).token) {
		case PP_IF:
		case PP_IFDEF:
		case PP_IFNDEF:
			++mIndex;
			skipUntilEndif();
			break;
		default:
			;
		}
		++mIndex;
	}
	return (mIndex < mSymbols.size() - 1);
}

Symbols FileParser::tokenize(const std::string& input, int lineNum, FileParser::TokenizeMode mode)
{
	Symbols symbols;
	// Preallocate some space to speed up the code below.
	// The magic divisor value was found by calculating the average ratio between
	// input size and the final size of symbols.
	// This yielded a value of 16.x when compiling Qt Base.
	symbols.reserve(input.size() / 16);
	const char* begin = input.c_str();
	const char* data = begin;
	while (*data) {
		if (mode == TokenizeCpp || mode == TokenizeDefine) {
			int column = 0;

			const char* lexem = data;
			int state = 0;
			Token token = NOTOKEN;
			for (;;) {
				if (static_cast<signed char>(*data) < 0) {
					++data;
					continue;
				}
				int nextindex = keywords[state].next;
				int next = 0;
				if (*data == keywords[state].defchar)
					next = keywords[state].defnext;
				else if (!state || nextindex)
					next = keyword_trans[nextindex][(int)*data];
				if (!next)
					break;
				state = next;
				token = keywords[state].token;
				++data;
			}

			if (keywords[state].ident && is_ident_char(*data))
				token = keywords[state].ident;

			if (token == NOTOKEN) {
				if (*data)
					++data;
				if (!FileParser::preprocessOnly)
					continue;
			}

			++column;

			if (token > SPECIAL_TREATMENT_MARK) {
				switch (token) {
				case QUOTE:
					data = skipQuote(data);
					token = STRING_LITERAL;
					// concatenate multi-line strings for easier
					// STRING_LITERAL handling in moc
					if (!FileParser::preprocessOnly
						&& !symbols.empty()
						&& symbols.back().token == STRING_LITERAL) {
						const std::string newString
							= '\"'
							+ std::string(symbols.back().unquotedLexem())
							+ input.substr(lexem - begin + 1, data - lexem - 2)
							+ '\"';
						symbols.back() = Symbol(symbols.back().lineNum,
							STRING_LITERAL,
							newString);
						continue;
					}
					break;
				case SINGLEQUOTE:
					while (*data && (*data != '\''
						|| (*(data - 1) == '\\'
							&& *(data - 2) != '\\')))
						++data;
					if (*data)
						++data;
					token = CHARACTER_LITERAL;
					break;
				case LANGLE_SCOPE:
					// split <:: into two tokens, < and ::
					token = LANGLE;
					data -= 2;
					break;
				case DIGIT:
					while (is_digit_char(*data) || *data == '\'')
						++data;
					if (!*data || *data != '.') {
						token = INTEGER_LITERAL;
						if (data - lexem == 1 &&
							(*data == 'x' || *data == 'X'
								|| *data == 'b' || *data == 'B')
							&& *lexem == '0') {
							++data;
							while (is_hex_char(*data) || *data == '\'')
								++data;
						}
						break;
					}
					token = FLOATING_LITERAL;
					++data;
					FALLTHROUGH();
				case FLOATING_LITERAL:
					while (is_digit_char(*data) || *data == '\'')
						++data;
					if (*data == '+' || *data == '-')
						++data;
					if (*data == 'e' || *data == 'E') {
						++data;
						while (is_digit_char(*data) || *data == '\'')
							++data;
					}
					if (*data == 'f' || *data == 'F'
						|| *data == 'l' || *data == 'L')
						++data;
					break;
				case HASH:
					if (column == 1 && mode == TokenizeCpp) {
						mode = PreparePreprocessorStatement;
						while (*data && (*data == ' ' || *data == '\t'))
							++data;
						if (is_ident_char(*data))
							mode = TokenizePreprocessorStatement;
						continue;
					}
					break;
				case PP_HASHHASH:
					if (mode == TokenizeCpp)
						continue;
					break;
				case NEWLINE:
					++lineNum;
					if (mode == TokenizeDefine) {
						mode = TokenizeCpp;
						// emit the newline token
						break;
					}
					continue;
				case BACKSLASH:
				{
					const char* rewind = data;
					while (*data && (*data == ' ' || *data == '\t'))
						++data;
					if (*data && *data == '\n') {
						++data;
						continue;
					}
					data = rewind;
				} break;
				case CHARACTER:
					while (is_ident_char(*data))
						++data;
					token = IDENTIFIER;
					break;
				case C_COMMENT:
					if (*data) {
						if (*data == '\n')
							++lineNum;
						++data;
						if (*data) {
							if (*data == '\n')
								++lineNum;
							++data;
						}
					}
					while (*data && (*(data - 1) != '/' || *(data - 2) != '*')) {
						if (*data == '\n')
							++lineNum;
						++data;
					}
					token = WHITESPACE; // one comment, one whitespace
					FALLTHROUGH();
				case WHITESPACE:
					if (column == 1)
						column = 0;
					while (*data && (*data == ' ' || *data == '\t'))
						++data;
					if (FileParser::preprocessOnly) // tokenize whitespace
						break;
					continue;
				case CPP_COMMENT:
					while (*data && *data != '\n')
						++data;
					continue; // ignore safely, the newline is a separator
				default:
					continue; //ignore
				}
			}
#ifdef USE_LEXEM_STORE
			if (!Preprocessor::preprocessOnly
				&& token != IDENTIFIER
				&& token != STRING_LITERAL
				&& token != FLOATING_LITERAL
				&& token != INTEGER_LITERAL)
				symbols.push_back(Symbol(lineNum, token);
			else
#endif
					symbols.push_back(Symbol(lineNum, token, input, lexem - begin, data - lexem));
		}
		else { //   Preprocessor
			const char* lexem = data;
			int state = 0;
			Token token = NOTOKEN;
			if (mode == TokenizePreprocessorStatement) {
				state = pp_keyword_trans[0][(int)'#'];
				mode = TokenizePreprocessor;
			}
			for (;;) {
				if (static_cast<signed char>(*data) < 0) {
					++data;
					continue;
				}
				int nextindex = pp_keywords[state].next;
				int next = 0;
				if (*data == pp_keywords[state].defchar)
					next = pp_keywords[state].defnext;
				else if (!state || nextindex)
					next = pp_keyword_trans[nextindex][(int)*data];
				if (!next)
					break;
				state = next;
				token = pp_keywords[state].token;
				++data;
			}
			// suboptimal, is_ident_char  should use a table
			if (pp_keywords[state].ident && is_ident_char(*data))
				token = pp_keywords[state].ident;

			switch (token) {
			case NOTOKEN:
				if (*data)
					++data;
				break;
			case PP_DEFINE:
				mode = PrepareDefine;
				break;
			case PP_IFDEF:
				symbols.push_back(Symbol(lineNum, PP_IF));
				symbols.push_back(Symbol(lineNum, PP_DEFINED));
				continue;
			case PP_IFNDEF:
				symbols.push_back(Symbol(lineNum, PP_IF));
				symbols.push_back(Symbol(lineNum, PP_NOT));
				symbols.push_back(Symbol(lineNum, PP_DEFINED));
				continue;
			case PP_INCLUDE:
				mode = TokenizeInclude;
				break;
			case PP_QUOTE:
				data = skipQuote(data);
				token = PP_STRING_LITERAL;
				break;
			case PP_SINGLEQUOTE:
				while (*data && (*data != '\''
					|| (*(data - 1) == '\\'
						&& *(data - 2) != '\\')))
					++data;
				if (*data)
					++data;
				token = PP_CHARACTER_LITERAL;
				break;

			case PP_DIGIT:
				while (is_digit_char(*data) || *data == '\'')
					++data;
				if (!*data || *data != '.') {
					token = PP_INTEGER_LITERAL;
					if (data - lexem == 1 &&
						(*data == 'x' || *data == 'X')
						&& *lexem == '0') {
						++data;
						while (is_hex_char(*data) || *data == '\'')
							++data;
					}
					break;
				}
				token = PP_FLOATING_LITERAL;
				++data;
				FALLTHROUGH();
			case PP_FLOATING_LITERAL:
				while (is_digit_char(*data) || *data == '\'')
					++data;
				if (*data == '+' || *data == '-')
					++data;
				if (*data == 'e' || *data == 'E') {
					++data;
					while (is_digit_char(*data) || *data == '\'')
						++data;
				}
				if (*data == 'f' || *data == 'F'
					|| *data == 'l' || *data == 'L')
					++data;
				break;
			case PP_CHARACTER:
				if (mode == PreparePreprocessorStatement) {
					data = lexem;
					mode = TokenizePreprocessorStatement;
					continue;
				}
				while (is_ident_char(*data))
					++data;
				token = PP_IDENTIFIER;

				if (mode == PrepareDefine) {
					symbols.push_back(Symbol(lineNum, token, input, lexem - begin, data - lexem));
					if (*data != '(')
						symbols.push_back(Symbol(lineNum, WHITESPACE));
					mode = TokenizeDefine;
					continue;
				}
				break;
			case PP_C_COMMENT:
				if (*data) {
					if (*data == '\n')
						++lineNum;
					++data;
					if (*data) {
						if (*data == '\n')
							++lineNum;
						++data;
					}
				}
				while (*data && (*(data - 1) != '/' || *(data - 2) != '*')) {
					if (*data == '\n')
						++lineNum;
					++data;
				}
				token = PP_WHITESPACE; // one comment, one whitespace
				FALLTHROUGH();
			case PP_WHITESPACE:
				while (*data && (*data == ' ' || *data == '\t'))
					++data;
				continue; // the preprocessor needs no whitespace
			case PP_CPP_COMMENT:
				while (*data && *data != '\n')
					++data;
				continue; // ignore safely, the newline is a separator
			case PP_NEWLINE:
				++lineNum;
				mode = TokenizeCpp;
				break;
			case PP_BACKSLASH:
			{
				const char* rewind = data;
				while (*data && (*data == ' ' || *data == '\t'))
					++data;
				if (*data && *data == '\n') {
					++data;
					continue;
				}
				data = rewind;
			} break;
			case PP_LANGLE:
				if (mode != TokenizeInclude)
					break;
				token = PP_STRING_LITERAL;
				while (*data && *data != '\n' && *(data - 1) != '>')
					++data;
				break;
			default:
				break;
			}
			if (mode == PreparePreprocessorStatement)
				continue;
			symbols.push_back(Symbol(lineNum, token, input, lexem - begin, data - lexem));
		}
	}
	symbols.push_back(Symbol()); // eof symbol
	return symbols;
}

void FileParser::macroExpand(Symbols* into, FileParser* that, const Symbols& toExpand, int& index,
	int lineNum, bool one, const std::set<std::string>& excludeSymbols)
{
	SymbolStack symbols;
	SafeSymbols sf;
	sf.symbols = toExpand;
	sf.index = index;
	sf.excludedSymbols = excludeSymbols;
	symbols.push_back(sf);

	if (toExpand.empty())
		return;

	for (;;) {
		std::string macro;
		Symbols newSyms = macroExpandIdentifier(that, symbols, lineNum, &macro);

		if (macro.empty()) {
			// not a macro
			Symbol s = symbols.symbol();
			s.lineNum = lineNum;
			into->push_back(s);
		}
		else {
			SafeSymbols sf;
			sf.symbols = newSyms;
			sf.index = 0;
			sf.expandedMacro = macro;
			symbols.push_back(sf);
		}
		if (!symbols.hasNext() || (one && symbols.size() == 1))
			break;
		symbols.next();
	}

	if (symbols.size())
		index = symbols.back().index;
	else
		index = toExpand.size();
}

Symbols FileParser::macroExpandIdentifier(FileParser* that, SymbolStack& symbols, int lineNum, std::string* macroName)
{
	Symbol s = symbols.symbol();

	// not a macro
	if (s.token != PP_IDENTIFIER || that->mMacros.find(s) == that->mMacros.end() || symbols.dontReplaceSymbol(s.lexem())) {
		return Symbols();
	}

	const Macro& macro = that->mMacros.at(s);
	*macroName = s.lexem();

	Symbols expansion;
	if (!macro.isFunction) {
		expansion = macro.symbols;
	}
	else {
		bool haveSpace = false;
		while (symbols.test(PP_WHITESPACE)) { haveSpace = true; }
		if (!symbols.test(PP_LPAREN)) {
			*macroName = std::string();
			Symbols syms;
			if (haveSpace)
				syms.push_back(Symbol(lineNum, PP_WHITESPACE));
			syms.push_back(s);
			syms.back().lineNum = lineNum;
			return syms;
		}

		std::vector<Symbols> arguments;
		while (symbols.hasNext()) {
			Symbols argument;
			// strip leading space
			while (symbols.test(PP_WHITESPACE)) {}
			int nesting = 0;
			bool vararg = macro.isVariadic && (arguments.size() == macro.arguments.size() - 1);
			while (symbols.hasNext()) {
				Token t = symbols.next();
				if (t == PP_LPAREN) {
					++nesting;
				}
				else if (t == PP_RPAREN) {
					--nesting;
					if (nesting < 0)
						break;
				}
				else if (t == PP_COMMA && nesting == 0) {
					if (!vararg)
						break;
				}
				argument.push_back(symbols.symbol());
			}
			arguments.push_back(argument);

			if (nesting < 0)
				break;
			else if (!symbols.hasNext())
				that->error("missing ')' in macro usage");
		}

		// empty VA_ARGS
		if (macro.isVariadic && arguments.size() == macro.arguments.size() - 1)
			arguments.push_back(Symbols());

		// now replace the macro arguments with the expanded arguments
		enum Mode {
			Normal,
			Hash,
			HashHash
		} mode = Normal;

		for (int i = 0; i < macro.symbols.size(); ++i) {
			const Symbol& s = macro.symbols.at(i);
			if (s.token == HASH || s.token == PP_HASHHASH) {
				mode = (s.token == HASH ? Hash : HashHash);
				continue;
			}
			auto result = find(macro.symbols.begin(), macro.symbols.end(), s);
			int index = distance(macro.symbols.begin(), result);
			if (mode == Normal) {
				if (index >= 0 && index < arguments.size()) {
					if (i == macro.symbols.size() - 1 || macro.symbols.at(i + 1).token != PP_HASHHASH) {
						Symbols arg = arguments.at(index);
						int idx = 1;
						macroExpand(&expansion, that, arg, idx, lineNum, false, symbols.excludeSymbols());
					}
					else {
						for (auto& it : arguments.at(index)) {
							expansion.push_back(it);
						}
					}
				}
				else {
					expansion.push_back(s);
				}
			}
			else if (mode == Hash) {
				if (index < 0) {
					that->error("'#' is not followed by a macro parameter");
					continue;
				}
				else if (index >= arguments.size()) {
					that->error("Macro invoked with too few parameters for a use of '#'");
					continue;
				}

				const Symbols& arg = arguments.at(index);
				std::string stringified;
				for (int i = 0; i < arg.size(); ++i) {
					stringified += arg.at(i).lexem();
				}
				stringified = std::regex_replace(stringified, std::regex("\""), "\\\"");
				stringified = '"' + stringified + '"';
				expansion.push_back(Symbol(lineNum, STRING_LITERAL, stringified));
			}
			else if (mode == HashHash) {
				if (s.token == WHITESPACE)
					continue;

				while (expansion.size() && expansion.back().token == PP_WHITESPACE)
					expansion.pop_back();

				Symbol next = s;
				if (index >= 0 && index < arguments.size()) {
					const Symbols& arg = arguments.at(index);
					if (arg.size() == 0) {
						mode = Normal;
						continue;
					}
					next = arg.at(0);
				}

				if (!expansion.empty() && expansion.back().token == s.token
					&& expansion.back().token != STRING_LITERAL) {
					Symbol last = expansion.back();
					expansion.pop_back();
					std::string lexem = std::string(last.lexem()) + std::string(next.lexem());
					expansion.push_back(Symbol(lineNum, last.token, lexem));
				}
				else {
					expansion.push_back(next);
				}

				if (index >= 0 && index < arguments.size()) {
					const Symbols& arg = arguments.at(index);
					for (int i = 1; i < arg.size(); ++i)
						expansion.push_back(arg.at(i));
				}
			}
			mode = Normal;
		}
		if (mode != Normal)
			that->error("'#' or '##' found at the end of a macro argument");
	}

	return expansion;
}

void FileParser::substituteUntilNewline(Symbols& substituted)
{
	while (hasNext()) {
		Token token = next();
		if (token == PP_IDENTIFIER) {
			macroExpand(&substituted, this, mSymbols, mIndex, symbol().lineNum, true);
		}
		else if (token == PP_DEFINED) {
			bool braces = test(PP_LPAREN);
			next(PP_IDENTIFIER);
			Symbol definedOrNotDefined = symbol();

			definedOrNotDefined.token = mMacros.find(definedOrNotDefined) != mMacros.end() ? PP_MOC_TRUE : PP_MOC_FALSE;
			substituted.push_back(definedOrNotDefined);
			if (braces)
				test(PP_RPAREN);
			continue;
		}
		else if (token == PP_NEWLINE) {
			substituted.push_back(symbol());
			break;
		}
		else {
			substituted.push_back(symbol());
		}
	}
}

class PP_Expression : public ParserBase
{
public:
	int value() { mIndex = 0; return unary_expression_lookup() ? conditional_expression() : 0; }

	int conditional_expression();
	int logical_OR_expression();
	int logical_AND_expression();
	int inclusive_OR_expression();
	int exclusive_OR_expression();
	int AND_expression();
	int equality_expression();
	int relational_expression();
	int shift_expression();
	int additive_expression();
	int multiplicative_expression();
	int unary_expression();
	bool unary_expression_lookup();
	int primary_expression();
	bool primary_expression_lookup();
};

int PP_Expression::conditional_expression()
{
	int value = logical_OR_expression();
	if (test(PP_QUESTION)) {
		int alt1 = conditional_expression();
		int alt2 = test(PP_COLON) ? conditional_expression() : 0;
		return value ? alt1 : alt2;
	}
	return value;
}

int PP_Expression::logical_OR_expression()
{
	int value = logical_AND_expression();
	if (test(PP_OROR))
		return logical_OR_expression() || value;
	return value;
}

int PP_Expression::logical_AND_expression()
{
	int value = inclusive_OR_expression();
	if (test(PP_ANDAND))
		return logical_AND_expression() && value;
	return value;
}

int PP_Expression::inclusive_OR_expression()
{
	int value = exclusive_OR_expression();
	if (test(PP_OR))
		return value | inclusive_OR_expression();
	return value;
}

int PP_Expression::exclusive_OR_expression()
{
	int value = AND_expression();
	if (test(PP_HAT))
		return value ^ exclusive_OR_expression();
	return value;
}

int PP_Expression::AND_expression()
{
	int value = equality_expression();
	if (test(PP_AND))
		return value & AND_expression();
	return value;
}

int PP_Expression::equality_expression()
{
	int value = relational_expression();
	switch (next()) {
	case PP_EQEQ:
		return value == equality_expression();
	case PP_NE:
		return value != equality_expression();
	default:
		prev();
		return value;
	}
}

int PP_Expression::relational_expression()
{
	int value = shift_expression();
	switch (next()) {
	case PP_LANGLE:
		return value < relational_expression();
	case PP_RANGLE:
		return value > relational_expression();
	case PP_LE:
		return value <= relational_expression();
	case PP_GE:
		return value >= relational_expression();
	default:
		prev();
		return value;
	}
}

int PP_Expression::shift_expression()
{
	int value = additive_expression();
	switch (next()) {
	case PP_LTLT:
		return value << shift_expression();
	case PP_GTGT:
		return value >> shift_expression();
	default:
		prev();
		return value;
	}
}

int PP_Expression::additive_expression()
{
	int value = multiplicative_expression();
	switch (next()) {
	case PP_PLUS:
		return value + additive_expression();
	case PP_MINUS:
		return value - additive_expression();
	default:
		prev();
		return value;
	}
}

int PP_Expression::multiplicative_expression()
{
	int value = unary_expression();
	switch (next()) {
	case PP_STAR:
	{
		// get well behaved overflow behavior by converting to long
		// and then back to int
		// NOTE: A conformant preprocessor would need to work intmax_t/
		// uintmax_t according to [cpp.cond], 19.1 ¡ì10
		// But we're not compliant anyway
		int64_t result = int64_t(value) * int64_t(multiplicative_expression());
		return int(result);
	}
	case PP_PERCENT:
	{
		int remainder = multiplicative_expression();
		return remainder ? value % remainder : 0;
	}
	case PP_SLASH:
	{
		int div = multiplicative_expression();
		return div ? value / div : 0;
	}
	default:
		prev();
		return value;
	};
}

int PP_Expression::unary_expression()
{
	switch (next()) {
	case PP_PLUS:
		return unary_expression();
	case PP_MINUS:
		return -unary_expression();
	case PP_NOT:
		return !unary_expression();
	case PP_TILDE:
		return ~unary_expression();
	case PP_MOC_TRUE:
		return 1;
	case PP_MOC_FALSE:
		return 0;
	default:
		prev();
		return primary_expression();
	}
}

bool PP_Expression::unary_expression_lookup()
{
	Token t = lookup();
	return (primary_expression_lookup()
		|| t == PP_PLUS
		|| t == PP_MINUS
		|| t == PP_NOT
		|| t == PP_TILDE
		|| t == PP_DEFINED);
}

int PP_Expression::primary_expression()
{
	int value;
	if (test(PP_LPAREN)) {
		value = conditional_expression();
		test(PP_RPAREN);
	}
	else {
		next();
		std::istringstream ss(std::string(lexem().data()));
		ss >> value;
	}
	return value;
}

bool PP_Expression::primary_expression_lookup()
{
	Token t = lookup();
	return (t == PP_IDENTIFIER
		|| t == PP_INTEGER_LITERAL
		|| t == PP_FLOATING_LITERAL
		|| t == PP_MOC_TRUE
		|| t == PP_MOC_FALSE
		|| t == PP_LPAREN);
}

int FileParser::evaluateCondition()
{
	PP_Expression expression;
	expression.mCurrentFilenames = mCurrentFilenames;

	substituteUntilNewline(expression.mSymbols);

	return expression.value();
}

static std::string readOrMapFile(std::string file_path)
{
	std::ifstream  file(file_path, std::ios::in);
	if (!file.is_open())
		return "";
	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

static void mergeStringLiterals(Symbols* _symbols)
{
	Symbols& symbols = *_symbols;
	for (Symbols::iterator i = symbols.begin(); i != symbols.end(); ++i) {
		if (i->token == STRING_LITERAL) {
			Symbols::iterator mergeSymbol = i;
			int literalsLength = mergeSymbol->len;
			while (++i != symbols.end() && i->token == STRING_LITERAL)
				literalsLength += i->len - 2; // no quotes

			if (literalsLength != mergeSymbol->len) {
				std::string mergeSymbolOriginalLexem = mergeSymbol->unquotedLexem();
				std::string& mergeSymbolLexem = mergeSymbol->lex;
				mergeSymbolLexem.resize(0);
				mergeSymbolLexem.reserve(literalsLength);
				mergeSymbolLexem.append("\"");
				mergeSymbolLexem.append(mergeSymbolOriginalLexem);
				for (Symbols::iterator j = mergeSymbol + 1; j != i; ++j)
					mergeSymbolLexem.append(j->lex.c_str() + j->from + 1, j->len - 2); // append j->unquotedLexem()
				mergeSymbolLexem.append("\"");
				mergeSymbol->len = mergeSymbol->lex.length();
				mergeSymbol->from = 0;
				i = symbols.erase(mergeSymbol + 1, i);
			}
			if (i == symbols.end())
				break;
		}
	}
}

static std::string searchIncludePaths(const std::vector<std::string>& includepaths,
	const std::string& include)
{
	std::filesystem::directory_entry fileInfo;
	for (int j = 0; j < includepaths.size() && !exists(fileInfo.path()); ++j) {
		std::filesystem::path path(includepaths.at(j));;
		fileInfo.assign(path.append(include));
		// try again, maybe there's a file later in the include paths with the same name
		// (186067)
		if (fileInfo.is_directory()) {
			fileInfo.assign("");
			continue;
		}
	}
	if (!exists(fileInfo.path()) || fileInfo.is_directory())
		return std::string();

	return absolute(fileInfo.path()).string();
}

std::string FileParser::resolveInclude(const std::string& include, const std::string& relativeTo)
{
	if (!relativeTo.empty()) {
		std::filesystem::directory_entry fileInfo(std::filesystem::path(relativeTo).parent_path().append(include).string());
		if (exists(fileInfo.path()) && !fileInfo.is_directory())
			return absolute(fileInfo.path()).string();
	}

	auto it = nonlocalIncludePathResolutionCache.find(include);
	if (it == nonlocalIncludePathResolutionCache.end()) {
		std::string path = searchIncludePaths(mIncludeDir, include);
		nonlocalIncludePathResolutionCache[include] = path;
		return path;
	}
	return it->second;
}

void FileParser::preprocess(const std::string& filename, Symbols& preprocessed)
{
	mCurrentFilenames.push(filename);
	preprocessed.reserve(preprocessed.size() + mSymbols.size());
	while (hasNext()) {
		Token token = next();
		switch (token) {
		case PP_INCLUDE:
		{
			std::string include;
			bool local = false;
			if (test(PP_STRING_LITERAL)) {
				local = (lexem()[0] == '\"');
				include = unquotedLexem();
			}
			else
				continue;
			until(PP_NEWLINE);

			include = resolveInclude(include, local ? filename : std::string());
			if (include.empty())
				continue;
			if (FileParser::preprocessedIncludes.find(include) != FileParser::preprocessedIncludes.end())
				continue;
			FileParser::preprocessedIncludes.insert(include);

			std::string input = readOrMapFile(include);

			if (input.empty())
				continue;

			Symbols saveSymbols = mSymbols;
			int saveIndex = mIndex;

			// phase 1: get rid of backslash-newlines
			input = cleaned(input);

			// phase 2: tokenize for the preprocessor
			mSymbols = tokenize(input);
			input.clear();

			mIndex = 0;

			// phase 3: preprocess conditions and substitute macros
			preprocessed.push_back(Symbol(0, AXON_INCLUDE_BEGIN, include));
			preprocess(include, preprocessed);
			preprocessed.push_back(Symbol(symbol().lineNum, AXON_INCLUDE_END, include));

			mSymbols = saveSymbols;
			mIndex = saveIndex;
			continue;
		}
		case PP_DEFINE:
		{
			next();
			std::string name = lexem();
			if (name.empty() || !is_ident_start(name[0]))
				error();
			Macro macro;
			macro.isVariadic = false;
			if (test(LPAREN)) {
				// we have a function macro
				macro.isFunction = true;
				parseDefineArguments(&macro);
			}
			else {
				macro.isFunction = false;
			}
			int start = mIndex;
			until(PP_NEWLINE);
			macro.symbols.reserve(mIndex - start - 1);

			// remove whitespace where there shouldn't be any:
			// Before and after the macro, after a # and around ##
			Token lastToken = HASH; // skip shitespace at the beginning
			for (int i = start; i < mIndex - 1; ++i) {
				Token token = mSymbols.at(i).token;
				if (token == WHITESPACE) {
					if (lastToken == PP_HASH || lastToken == HASH ||
						lastToken == PP_HASHHASH ||
						lastToken == WHITESPACE)
						continue;
				}
				else if (token == PP_HASHHASH) {
					if (!macro.symbols.empty() &&
						lastToken == WHITESPACE)
						macro.symbols.pop_back();
				}
				macro.symbols.push_back(mSymbols.at(i));
				lastToken = token;
			}
			// remove trailing whitespace
			while (!macro.symbols.empty() &&
				(macro.symbols.back().token == PP_WHITESPACE || macro.symbols.back().token == WHITESPACE))
				macro.symbols.pop_back();

			if (!macro.symbols.empty()) {
				if (macro.symbols.front().token == PP_HASHHASH ||
					macro.symbols.back().token == PP_HASHHASH) {
					error("'##' cannot appear at either end of a macro expansion");
				}
			}
			mMacros[name] = macro;
			continue;
		}
		case PP_UNDEF: {
			next();
			std::string name = lexem();
			until(PP_NEWLINE);
			mMacros.erase(name);
			continue;
		}
		case PP_IDENTIFIER: {
			// substitute macros
			macroExpand(&preprocessed, this, mSymbols, mIndex, symbol().lineNum, true);
			continue;
		}
		case PP_HASH:
			until(PP_NEWLINE);
			continue; // skip unknown preprocessor statement
		case PP_IFDEF:
		case PP_IFNDEF:
		case PP_IF:
			while (!evaluateCondition()) {
				if (!skipBranch())
					break;
				if (test(PP_ELIF)) {
				}
				else {
					until(PP_NEWLINE);
					break;
				}
			}
			continue;
		case PP_ELIF:
		case PP_ELSE:
			skipUntilEndif();
		case PP_ENDIF:
			until(PP_NEWLINE);
			continue;
		case PP_NEWLINE:
			continue;
		default:
			break;
		}
		preprocessed.push_back(symbol());
	}

	mCurrentFilenames.pop();
}

FileParser::FileParser()
{
	Macro dummyVariadicFunctionMacro;
	dummyVariadicFunctionMacro.isFunction = true;
	dummyVariadicFunctionMacro.isVariadic = true;
	dummyVariadicFunctionMacro.arguments.push_back(Symbol(0, PP_IDENTIFIER, "__VA_ARGS__"));
	mMacros["__attribute__"] = dummyVariadicFunctionMacro;
	mMacros["__declspec"] = dummyVariadicFunctionMacro;
}

Symbols FileParser::preprocessed(const FileDataDef& fileData)
{
	std::string input = readOrMapFile(fileData.inputFilePath);
	if (input.empty())
		return mSymbols;

	mIncludeDir = fileData.inputIncludeDir;
	// phase 1: get rid of backslash-newlines
	input = cleaned(input);

	// phase 2: tokenize for the preprocessor
	mIndex = 0;
	mSymbols = tokenize(input);

	// phase 3: preprocess conditions and substitute macros
	Symbols result;
	// Preallocate some space to speed up the code below.
	// The magic value was found by logging the final size
	// and calculating an average when running MOC over FOSS projects.
	std::filesystem::directory_entry fileInfo(fileData.inputFilePath);
	result.reserve(fileInfo.file_size() / 300000);
	preprocess(fileData.inputFilePath, result);
	mergeStringLiterals(&result);
	return result;
}

void FileParser::parseDefineArguments(Macro* m)
{
	Symbols arguments;
	while (hasNext()) {
		while (test(PP_WHITESPACE)) {}
		Token t = next();
		if (t == PP_RPAREN)
			break;
		if (t != PP_IDENTIFIER) {
			std::string l = lexem();
			if (l == "...") {
				m->isVariadic = true;
				arguments.push_back(Symbol(symbol().lineNum, PP_IDENTIFIER, "__VA_ARGS__"));
				while (test(PP_WHITESPACE)) {}
				if (!test(PP_RPAREN))
					error("missing ')' in macro argument list");
				break;
			}
			else if (!is_identifier(l.data(), l.length())) {
				error("Unexpected character in macro argument list.");
			}
		}
		Symbol arg = symbol();

		if (find(arguments.begin(), arguments.end(), arg) != arguments.end())
			error("Duplicate macro parameter.");
		arguments.push_back(symbol());

		while (test(PP_WHITESPACE)) {}
		t = next();
		if (t == PP_RPAREN)
			break;
		if (t == PP_COMMA)
			continue;
		if (lexem() == "...") {
			//GCC extension:    #define FOO(x, y...) x(y)
			// The last argument was already parsed. Just mark the macro as variadic.
			m->isVariadic = true;
			while (test(PP_WHITESPACE)) {}
			if (!test(PP_RPAREN))
				error("missing ')' in macro argument list");
			break;
		}
		error("Unexpected character in macro argument list.");
	}
	m->arguments = arguments;
	while (test(PP_WHITESPACE)) {}
}

void FileParser::until(Token t)
{
	while (hasNext() && next() != t);
}