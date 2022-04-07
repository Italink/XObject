#ifndef Symbols_h__
#define Symbols_h__

#include <string>
#include <vector>
#include <set>
#include <stack>
#include "Token.h"

struct Symbol
{
    inline Symbol() : lineNum(-1), token(NOTOKEN), from(0), len(-1) {}
    inline Symbol(int lineNum, Token token) :
        lineNum(lineNum), token(token), from(0), len(-1) {}
    inline Symbol(int lineNum, Token token, const std::string& lexem) :
        lineNum(lineNum), token(token), lex(lexem), from(0) {
        len = lex.size();
    }
    inline Symbol(int lineNum, Token token, const std::string& lexem, int from, int len) :
        lineNum(lineNum), token(token), lex(lexem), from(from), len(len) {}
    int lineNum;
    Token token;
    inline std::string lexem() const { return lex.substr(from, len); }
    inline std::string unquotedLexem() const { return lex.substr(from + 1, len - 2); }
    inline operator std::string() const { return lex.substr(from, len); }
    bool operator==(const Symbol& o) const
    {
        return lex.substr(from,len)==o.lex.substr(from,len);
    }
    std::string lex;
    int from, len;
};


typedef std::vector<Symbol> Symbols;

struct SafeSymbols {
    Symbols symbols;
    std::string expandedMacro;
    std::set<std::string> excludedSymbols;
    int index;
};

class SymbolStack : public std::vector<SafeSymbols>
{
public:
    inline bool hasNext() {
        while (!empty() && back().index >= back().symbols.size())
            this->pop_back();
        return !empty();
    }
    inline Token next() {
        while (!empty() && back().index >= back().symbols.size())
            this->pop_back();
        if (empty())
            return NOTOKEN;
        return back().symbols.at(back().index++).token;
    }
    bool test(Token);
    inline const Symbol& symbol() const { return back().symbols.at(back().index - 1); }
    inline Token token() { return symbol().token; }
    inline std::string lexem() const { return symbol().lexem(); }
    inline std::string unquotedLexem() { return symbol().unquotedLexem(); }

    bool dontReplaceSymbol(const std::string& name);
    std::set<std::string> excludeSymbols();
};

inline bool SymbolStack::test(Token token)
{
    int stackPos = size() - 1;
    while (stackPos >= 0 && at(stackPos).index >= at(stackPos).symbols.size())
        --stackPos;
    if (stackPos < 0)
        return false;
    if (at(stackPos).symbols.at(at(stackPos).index).token == token) {
        next();
        return true;
    }
    return false;
}

inline bool SymbolStack::dontReplaceSymbol(const std::string& name)
{
    for (int i = 0; i < size(); ++i) {
        if (name == at(i).expandedMacro || at(i).excludedSymbols.find(name)!= at(i).excludedSymbols.end())
            return true;
    }
    return false;
}

inline std::set<std::string> SymbolStack::excludeSymbols(){
    std::set<std::string> set;
    for (int i = 0; i < size(); ++i) {
        set.insert(this->at(i).expandedMacro);
        for(auto& it: at(i).excludedSymbols)
            set.insert(it);
    }
    return set;
}

#endif // Symbols_h__
