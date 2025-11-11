#ifndef LEXER_H
#define LEXER_H

#include <QMap>
#include <QList>
#include <QString>
#include <QVariant>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>


enum class TokenType {
    Word = 0b1,
    Delimeter = 0b10,
    Id = 0b100,
    Const = 0b1000,
    Nonterminal = 0b10000,
    Error = 0x0,
};

inline QMap<TokenType, QString> token_type_to_qstring = {
    {TokenType::Word, QString("Word")},
    {TokenType::Delimeter, QString("Delimeter")},
    {TokenType::Id, QString("Id")},
    {TokenType::Const, QString("Const")},
    {TokenType::Error, QString("Error")}
};

inline QMap<QString, TokenType> lexemas_associations = {
    {"program", TokenType::Word},
    {"var", TokenType::Word},
    {"begin", TokenType::Word},
    {"end", TokenType::Word},
    {"int", TokenType::Word},
    {"integer", TokenType::Word},
    {"input", TokenType::Word},
    {"output", TokenType::Word},
    {"for", TokenType::Word},
    {"while", TokenType::Word},
    {"if", TokenType::Word},
    {"let", TokenType::Word},
    {"(", TokenType::Delimeter},
    {")", TokenType::Delimeter},
    {";", TokenType::Delimeter},
    {",", TokenType::Delimeter},
    {".", TokenType::Delimeter},
    {"+", TokenType::Delimeter},
    {"-", TokenType::Delimeter},
    {"/", TokenType::Delimeter},
    {"*", TokenType::Delimeter},
    {"=", TokenType::Delimeter}
};

inline QString id_pattern = "^\\w\\d*\\w$";
inline QString const_pattern = "^\\d+$";
inline QList<QString> spec_op_words = {
    "input",
    "output"
};

class Lexema {

    QString __value;
    TokenType __type;

public:
    Lexema() {}
    Lexema(QString);
    Lexema(QString v, TokenType t) : __value(v), __type(t) {};

    QString value() const { return __value; }
    TokenType type() const { return __type; }

    QString toQString() { return QString("(\"%1\", %2)\n")
                              .arg(__value, token_type_to_qstring[__type]) ; }

    static bool is_id(QString& candidate) {
        QRegularExpression re = QRegularExpression(id_pattern);

        QRegularExpressionMatchIterator matches = re.globalMatch(candidate);

        if (matches.hasNext() && matches.next().hasMatch() && !matches.hasNext())
            return 1;
        return 0;
    }
    static bool is_const(QString& candidate) {
        QRegularExpression re = QRegularExpression(const_pattern);

        QRegularExpressionMatch match = re.match(candidate);

        if (match.hasMatch())
            return 1;
        return 0;
    }
    static bool is_op(Lexema& lex) {
        if (lex.__type != TokenType::Word)
            return false;

        if (!spec_op_words.contains(lex.__value))
            return false;

        return true;
    }
    static bool is_arithm(Lexema& lex) {
        if (lex.__value == '+' || lex.__value == '-' ||
            lex.__value == '/' || lex.__value == '*')
            return true;
        return false;
    }

    bool operator==(const Lexema& lex) const {
        if (__type == lex.__type && __value == lex.__value)
            return true;
        return false;
    }

    static Lexema PROGRAM() { return Lexema("program"); }
    static Lexema VAR() { return Lexema("var"); }
    static Lexema INT() { return Lexema("int"); }
    static Lexema BEGIN() { return Lexema("begin"); }
    static Lexema END() { return Lexema("end"); }
    static Lexema INPUT() { return Lexema("input"); }
    static Lexema OUTPUT() { return Lexema("output"); }
    static Lexema FOR() { return Lexema("for"); }
    static Lexema WHILE() { return Lexema("while"); }
    static Lexema IF() { return Lexema("if"); }
    static Lexema ELSE() { return Lexema("else"); }
    static Lexema THEN() { return Lexema("then"); }
    static Lexema LET() { return Lexema("let"); }
    static Lexema LPAR() { return Lexema("("); }
    static Lexema RPAR() { return Lexema(")"); }
    static Lexema SUM() { return Lexema("+"); }
    static Lexema DIF() { return Lexema("-"); }
    static Lexema DIV() { return Lexema("/"); }
    static Lexema MUL() { return Lexema("*"); }
    static Lexema EQU() { return Lexema("="); }
    static Lexema DOT() { return Lexema("."); }
    static Lexema COM() { return Lexema(","); }
    static Lexema SEMICOLON() { return Lexema(";"); }
    static Lexema A() { return Lexema("a", TokenType::Id); }
    static Lexema AS() { return Lexema("as", TokenType::Nonterminal); }
    static Lexema C() { return Lexema("C", TokenType::Nonterminal); }
    static Lexema E() { return Lexema("E", TokenType::Nonterminal); }
    static Lexema F() { return Lexema("F", TokenType::Nonterminal); }
    static Lexema T() { return Lexema("T", TokenType::Nonterminal); }
    static Lexema ES() { return Lexema("ES", TokenType::Nonterminal); }
    static Lexema O() { return Lexema("O", TokenType::Nonterminal); }
    static Lexema OS() { return Lexema("OS", TokenType::Nonterminal); }
    static Lexema P() { return Lexema("P", TokenType::Nonterminal); }
    static Lexema D() { return Lexema("D", TokenType::Nonterminal); }
    static Lexema I() { return Lexema("I", TokenType::Nonterminal); }
};


class Lexer {
public:
    QList<QString> token_types = {"words", "ids", "consts", "delimeters"};
private:
    QMap<QString, QList<Lexema>> token_tables;
    QList<Lexema> tokenized_code;

    bool is_reading_from_file = false;
    QString source_code;

    void register_lexema(Lexema& lex) {
        switch (lex.type()) {
        case TokenType::Word:
            if (!token_tables["words"].contains(lex))
                token_tables["words"].push_back(lex);
            break;
        case TokenType::Const:
            if (!token_tables["consts"].contains(lex))
                token_tables["consts"].push_back(lex);
            break;
        case TokenType::Id:
            if (!token_tables["ids"].contains(lex))
                token_tables["ids"].push_back(lex);
            break;
        case TokenType::Delimeter:
            if (!token_tables["delimeters"].contains(lex))
                token_tables["delimeters"].push_back(lex);
            break;
        default:
            throw std::runtime_error("InvalidTokenTypeError");
        }

        tokenized_code.push_back(lex);
    }

public:
    Lexer() {
        foreach (auto i, token_types)
            token_tables[i] = {};
    };

    QList<Lexema>& get_tokenized_code() { return tokenized_code; }
    QList<Lexema>& get_words() { return token_tables["words"]; }
    QList<Lexema>& get_ids() { return token_tables["ids"]; }
    QList<Lexema>& get_consts() { return token_tables["consts"]; }
    QList<Lexema>& get_delimeters() { return token_tables["delimeters"]; }
    QMap<QString, QList<Lexema>>& get_tables() {return token_tables;}

    bool analyze();

    bool loadFile(QString);
    bool loadText(QString&);
};

#endif // LEXER_H
