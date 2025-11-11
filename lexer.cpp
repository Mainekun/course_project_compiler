#include "lexer.h"

/*!
    Returns true if file exists, false otherwise.
*/
bool Lexer::loadFile(QString filename) {
    QFile tmp = QFile(filename);

    if (tmp.exists())
    {
        is_reading_from_file = true;
        source_code = filename;
        return 1;
    }

    return 0;
}

/*!
    Returns true no matter what.
*/
bool Lexer::loadText(QString& text) {
    is_reading_from_file = false;
    source_code = text;
    return 1;
}

/*!
    Performs lexical analysis.
    Result are stored in token_tables and tokenized_code.
    Returns true if no errors occured, false otherwise.
*/
bool Lexer::analyze() {
    QTextStream in;
    QFile* code;
    if (is_reading_from_file) {
        code = new QFile(source_code);
        code->open(QIODeviceBase::ReadOnly | QIODeviceBase::Text);
        in.setDevice(code);
    }
    else {
        in.setString(&source_code, QIODeviceBase::ReadOnly);
    }

    QChar ch;
    QString word;
    Lexema cand;
    while (!in.atEnd()) {
        ch = in.read(1).at(0);

        switch ([&]()->int{
            if (ch == ' ' || ch == '\n' || ch == '\t' ||
                ch == '\0' || ch == '\r')
                return 0;
            else if (ch.isDigit() || ch.isLetter())
                return 1;
            else if (ch == '.' || ch == ',' || ch == ';' ||
                     ch == '+' || ch == '-' || ch =='/' ||
                     ch == '*' || ch == '(' || ch == ')' ||
                     ch == '=')
                return 2;
            else
                return 3;
        }()) {
        case 0:
            if (word.length() == 0)
                goto nextcycle;

            cand = Lexema(word);
            if (cand.type() == TokenType::Error)
                throw std::runtime_error("InvalidTokenError");

            register_lexema(cand);
            word = "";

            break;
        case 1:
            word += ch;

            break;
        case 2:
            if (word.length() == 0)
                goto onlydelimeter;

            cand = Lexema(word);
            if (cand.type() == TokenType::Error)
                throw std::runtime_error("InvalidTokenValue");

            register_lexema(cand);
            word = "";

            onlydelimeter:

            word += ch;
            cand = Lexema(word);
            if (cand.type() == TokenType::Error)
                throw std::runtime_error("InvalidTokenValue");

            register_lexema(cand);
            word = "";

            break;
        case 3:

                throw std::runtime_error("UnknownCharacterError");

            break;
        }
        nextcycle:
    }
    endcycle:


    if (code != 0) {
        code->close();
        delete code;
    }

    return 1;
}

Lexema::Lexema(QString value) : __value(value) {
    if (lexemas_associations.contains(value))
        __type = lexemas_associations[__value];
    else if (Lexema::is_id(value))
        __type = TokenType::Id;
    else if (Lexema::is_const(value))
        __type = TokenType::Const;
    else
        __type = TokenType::Error;
}
