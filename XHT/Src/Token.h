#ifndef Token_h__
#define Token_h__

#define FOR_ALL_TOKENS(F) \
    F(NOTOKEN) \
    F(IDENTIFIER) \
    F(INTEGER_LITERAL) \
    F(CHARACTER_LITERAL) \
    F(STRING_LITERAL) \
    F(BOOLEAN_LITERAL) \
    F(HEADER_NAME) \
    F(LANGLE) \
    F(RANGLE) \
    F(LPAREN) \
    F(RPAREN) \
    F(ELIPSIS) \
    F(LBRACK) \
    F(RBRACK) \
    F(LBRACE) \
    F(RBRACE) \
    F(EQ) \
    F(SCOPE) \
    F(SEMIC) \
    F(COLON) \
    F(DOTSTAR) \
    F(QUESTION) \
    F(DOT) \
    F(DYNAMIC_CAST) \
    F(STATIC_CAST) \
    F(REINTERPRET_CAST) \
    F(CONST_CAST) \
    F(TYPEID) \
    F(THIS) \
    F(TEMPLATE) \
    F(THROW) \
    F(TRY) \
    F(CATCH) \
    F(TYPEDEF) \
    F(FRIEND) \
    F(CLASS) \
    F(NAMESPACE) \
    F(ENUM) \
    F(STRUCT) \
    F(UNION) \
    F(VIRTUAL) \
    F(PRIVATE) \
    F(PROTECTED) \
    F(PUBLIC) \
    F(EXPORT) \
    F(AUTO) \
    F(REGISTER) \
    F(EXTERN) \
    F(MUTABLE) \
    F(ASM) \
    F(USING) \
    F(INLINE) \
    F(EXPLICIT) \
    F(STATIC) \
    F(CONST) \
    F(VOLATILE) \
    F(OPERATOR) \
    F(SIZEOF) \
    F(NEW) \
    F(DELETE) \
    F(PLUS) \
    F(MINUS) \
    F(STAR) \
    F(SLASH) \
    F(PERCENT) \
    F(HAT) \
    F(AND) \
    F(OR) \
    F(TILDE) \
    F(NOT) \
    F(PLUS_EQ) \
    F(MINUS_EQ) \
    F(STAR_EQ) \
    F(SLASH_EQ) \
    F(PERCENT_EQ) \
    F(HAT_EQ) \
    F(AND_EQ) \
    F(OR_EQ) \
    F(LTLT) \
    F(GTGT) \
    F(GTGT_EQ) \
    F(LTLT_EQ) \
    F(EQEQ) \
    F(NE) \
    F(LE) \
    F(GE) \
    F(ANDAND) \
    F(OROR) \
    F(INCR) \
    F(DECR) \
    F(COMMA) \
    F(ARROW_STAR) \
    F(ARROW) \
    F(CHAR) \
    F(WCHAR) \
    F(BOOL) \
    F(SHORT) \
    F(INT) \
    F(LONG) \
    F(SIGNED) \
    F(UNSIGNED) \
    F(FLOAT) \
    F(DOUBLE) \
    F(VOID) \
    F(CASE) \
    F(DEFAULT) \
    F(IF) \
    F(ELSE) \
    F(SWITCH) \
    F(WHILE) \
    F(DO) \
    F(FOR) \
    F(BREAK) \
    F(CONTINUE) \
    F(GOTO) \
    F(RETURN) \
    F(AXON_INCLUDE_BEGIN) \
    F(AXON_INCLUDE_END) \
    F(XENTRY_TOKEN) \
    F(XFUNCTION_TOKEN) \
    F(XPROPERTY_TOKEN) \
    F(XENUM_TOKEN) \
    F(SPECIAL_TREATMENT_MARK) \
    F(CPP_COMMENT) \
    F(C_COMMENT) \
    F(FLOATING_LITERAL) \
    F(HASH) \
    F(QUOTE) \
    F(SINGLEQUOTE) \
    F(LANGLE_SCOPE) \
    F(DIGIT) \
    F(CHARACTER) \
    F(NEWLINE) \
    F(WHITESPACE) \
    F(BACKSLASH) \
    F(INCOMPLETE) \
    F(PP_DEFINE) \
    F(PP_UNDEF) \
    F(PP_IF) \
    F(PP_IFDEF) \
    F(PP_IFNDEF) \
    F(PP_ELIF) \
    F(PP_ELSE) \
    F(PP_ENDIF) \
    F(PP_INCLUDE) \
    F(PP_HASHHASH) \
    F(PP_HASH) \
    F(PP_DEFINED) \
    F(PP_INCOMPLETE) \
    F(PP_MOC_TRUE) \
    F(PP_MOC_FALSE)

enum Token {
#define CREATE_ENUM_VALUE(Name) Name,
	FOR_ALL_TOKENS(CREATE_ENUM_VALUE)
#undef CREATE_ENUM_VALUE

	// aliases
	PP_AND = AND,
	PP_ANDAND = ANDAND,
	PP_BACKSLASH = BACKSLASH,
	PP_CHARACTER = CHARACTER,
	PP_CHARACTER_LITERAL = CHARACTER_LITERAL,
	PP_COLON = COLON,
	PP_COMMA = COMMA,
	PP_CPP_COMMENT = CPP_COMMENT,
	PP_C_COMMENT = C_COMMENT,
	PP_DIGIT = DIGIT,
	PP_EQEQ = EQEQ,
	PP_FLOATING_LITERAL = FLOATING_LITERAL,
	PP_GE = GE,
	PP_GTGT = GTGT,
	PP_HAT = HAT,
	PP_IDENTIFIER = IDENTIFIER,
	PP_INTEGER_LITERAL = INTEGER_LITERAL,
	PP_LANGLE = LANGLE,
	PP_LE = LE,
	PP_LPAREN = LPAREN,
	PP_LTLT = LTLT,
	PP_MINUS = MINUS,
	PP_NE = NE,
	PP_NEWLINE = NEWLINE,
	PP_NOTOKEN = NOTOKEN,
	PP_NOT = NOT,
	PP_OR = OR,
	PP_OROR = OROR,
	PP_PERCENT = PERCENT,
	PP_PLUS = PLUS,
	PP_QUESTION = QUESTION,
	PP_QUOTE = QUOTE,
	PP_RANGLE = RANGLE,
	PP_RPAREN = RPAREN,
	PP_SINGLEQUOTE = SINGLEQUOTE,
	PP_SLASH = SLASH,
	PP_STAR = STAR,
	PP_STRING_LITERAL = STRING_LITERAL,
	PP_TILDE = TILDE,
	PP_WHITESPACE = WHITESPACE,
};

// for debugging only
#if defined(DEBUG_MOC)
const char* tokenTypeName(Token t);
#endif

typedef Token PP_Token;

#endif // Token_h__
