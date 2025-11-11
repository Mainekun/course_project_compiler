#ifndef PARSER_RULES_H
#define PARSER_RULES_H

#include <QMap>
#include <QString>
#include <QStack>

#include "lexer.h"

/*!
    1 - >.
    0 - =.
    -1 - <.
*/


static QMap<QString, QMap<QString, int>> parser_rules = {
    {
        "program",
        {
         {"^", -1},
         }
    },
    {
        "var",
        {
         {"a", -1},
         }
    },
    {
        "int",
        {
         {"var", -1},
         {",", 1},
         {"a", 1},
         }
    },
    {
        "integer",
        {
         {"var", -1},
         {",", 1},
         {"a", 1},
         }
    },
    {
        "begin",
        {
         {"a", -1},
         {"int", 1},
         {"integer", 1},
         {"begin", -1},
         {";", -1},
         {"else", -1},
         {"then", -1},
         {")", -1},
         }
    },
    {
        "end",
        {
         {";", 1},
         {"end", 1},
         {"begin", -1},
         {"a" , 1},
         {"*", 1},
         {"/", 1},
         {"-", 1},
         {"+", 1},
         {"=", 1},
         {")", 1},
         }
    },
    {
        "input",
        {
         {"begin", -1},
         {";", -1},
         {"else", -1},
         {"then", -1},
         }
    },
    {
        "output",
        {
         {"begin", -1},
         {";", -1},
         {"else", -1},
         {"then", -1},

         }
    },
    {
        "for",
        {
         {"begin", -1},
         {";", -1},
         {"else", -1},
         {"then", -1},

         }
    },
    {
        "while",
        {
         {"begin", -1},
         {";", -1},
         {"else", -1},
         {"then", -1},

         }
    },
    {
        "if",
        {
         {"begin", -1},
         {";", -1},
         {"else", -1},
         {"then", -1},

         }
    },
    {
        "let",
        {
         {"begin", -1},
         {";", -1},
         {"else", -1},
         {"then", -1},

         }
    },
    {
        ";",
        {
         {"a", 1},
         {"begin", -1},
         {"end", 1},
         {";", -1},
         {",", 1},
         {"*", 1},
         {"/", 1},
         {"-", 1},
         {"+", 1},
         {"=", 1},
         {"else", 1},
         {"then", 1},
         {"(", -1},
         {")", 1},
         }
    },
    {
        ",",
        {
         {"var", -1},
         {",", -1},
         {"*", 1},
         {"/", 1},
         {"-", 1},
         {"+", 1},
         {"a", 1},
         {"(", -1},
         {")", 1},
         }
    },
    {
        ".",
        {
         {"end", 0},
         }
    },
    {
        "*",
        {
         {"a", 1},
         {"end", 1},
         {";", -1},
         {",", -1},
         {"*", 1},
         {"/", 1},
         {"+", -1},
         {"-", -1},
         {"=", -1},
         {"(", -1},
         {")", 1},
         }
    },
    {
        "/",
        {
         {"a", 1},
         {"end", 1},
         {";", -1},
         {",", -1},
         {"*", 1},
         {"/", 1},
         {"+", -1},
         {"-", -1},
         {"=", -1},
         {"(", -1},
         {")", 1},
         }
    },
    {
        "+",
        {
         {"a", 1},
         {"end", 1},
         {";", -1},
         {",", -1},
         {"*", 1},
         {"/", 1},
         {"+", 1},
         {"-", 1},
         {"=", -1},
         {"(", -1},
         {")", 1},
         }
    },
    {
        "-",
        {
         {"a", 1},
         {"end", 1},
         {";", -1},
         {",", -1},
         {"*", 1},
         {"/", 1},
         {"+", 1},
         {"-", 1},
         {"=", -1},
         {"(", -1},
         {")", 1},
         }
    },
    {
        "=",
        {
         {"end", 1},
         {"a", 0},
         }
    },
    {
        "else",
        {
         {"end", 1},
         {"*", 1},
         {"/", 1},
         {"+", 1},
         {"-", 1},
         {"=", 1},
         {")", 1},
         {"else", 1},
         {"then", 1},
         }
    },
    {
        "then",
        {
            {"end", 1}
        }
    },
    {
        "a",
        {
         {"program", 0},
         {"var", -1},
         {"let", 0},
         {";", -1},
         {",", -1},
         {"*", -1},
         {"/", -1},
         {"+", -1},
         {"-", -1},
         {"=", -1},
         {"(", -1},
         }
    },
    {
        "(",
        {
         {"input", 0},
         {"output", 0},
         {"for", 0},
         {"while", 0},
         {"if", 0},
         {";", -1},
         {",", -1},
         {"*", -1},
         {"/", -1},
         {"+", -1},
         {"-", -1},
         {"=", -1},
         {"(", -1},
         }
    },
    {
        ")",
        {
         {"a", 1},
         {"end", 1},
         {"(", 0},
         {")", 1},
         {";", -1},
         }
    },
    {
        "^", //! Beginning
        {

        }
    },
    {
        "$", //! Ending
        {
         {".", 1},
         {"^", -1},
         }
    }
};

class Rule {
    QList<Lexema> __seq;
    QString __name;
    Lexema (*__return)();

public:
    Rule() : __name("???"), __return([](){return Lexema("???", TokenType::Error);}) {};
    Rule(QString name) : __name(name), __return([](){return Lexema("???", TokenType::Error);}) {}

    Rule& push_back(Lexema lex) {
        __seq.push_back(lex);
        return *this;
    }

    Rule& ret(Lexema (*ret)()) {
        __return = ret;
        return *this;
    }

    QString name() const { return __name; }

    bool operator==(QList<Lexema>& seq) const {
        if (__seq.length() != seq.length())
            return false;

        for (int i = 0; i < __seq.length(); i++)
            if ((__seq.at(i).type() == TokenType::Id || __seq.at(i).type() == TokenType::Const) &&
                (seq.at(i).type() == TokenType::Id || seq.at(i).type() == TokenType::Const))
                    continue;
            else
                if (__seq.at(i) == seq.at(i))
                    continue;
                else
                    return false;

        return true;
    }

    bool check_stack(QStack<Lexema>* stack) const {
        if (stack->length() < len())
            return false;

        for (int i = 0; i < len(); i++) {
            qsizetype stack_index = stack->length() - len() + i;
            if ((__seq.at(i).type() == TokenType::Id || __seq.at(i).type() == TokenType::Const) &&
                (stack->at(stack_index).type() == TokenType::Id || stack->at(stack_index).type() == TokenType::Const))
                continue;
            else
                if (__seq.at(i) == stack->at(stack_index))
                    continue;
                else
                    return false;
        }

        return true;
    }

    bool empty() const { return __seq.empty(); }

    qsizetype len() const { return __seq.length(); }

    Lexema operator()() { return __return(); }
};

inline QList<Rule> rules = {
    Rule("program").push_back(Lexema::PROGRAM())
        .push_back(Lexema::A())
        .push_back(Lexema::E())
        .push_back(Lexema::BEGIN())
        .push_back(Lexema::E())
        .push_back(Lexema::END())
        .push_back(Lexema::DOT())
        .ret(Lexema::E),

    Rule("description").push_back(Lexema::VAR())
        .push_back(Lexema::E())
        .push_back(Lexema::INT())
        .ret(Lexema::E),

    Rule("ids").push_back(Lexema::E())
        .push_back(Lexema::COM())
        .push_back(Lexema::E())
        .ret(Lexema::E),

    Rule("block").push_back(Lexema::BEGIN())
        .push_back(Lexema::E())
        .push_back(Lexema::END())
        .ret(Lexema::E),

    Rule("input").push_back(Lexema::INPUT())
        .push_back(Lexema::LPAR())
        .push_back(Lexema::E())
        .push_back(Lexema::RPAR())
        .ret(Lexema::E),

    Rule("output").push_back(Lexema::OUTPUT())
        .push_back(Lexema::LPAR())
        .push_back(Lexema::E())
        .push_back(Lexema::RPAR())
        .ret(Lexema::E),

    Rule("for").push_back(Lexema::FOR())
        .push_back(Lexema::LPAR())
        .push_back(Lexema::E())
        .push_back(Lexema::SEMICOLON())
        .push_back(Lexema::E())
        .push_back(Lexema::SEMICOLON())
        .push_back(Lexema::E())
        .push_back(Lexema::RPAR())
        .push_back(Lexema::E())
        .ret(Lexema::E),

    Rule("while").push_back(Lexema::WHILE())
        .push_back(Lexema::LPAR())
        .push_back(Lexema::E())
        .push_back(Lexema::RPAR())
        .push_back(Lexema::E())
        .ret(Lexema::E),

    Rule("if").push_back(Lexema::IF())
        .push_back(Lexema::LPAR())
        .push_back(Lexema::E())
        .push_back(Lexema::RPAR())
        .push_back(Lexema::THEN())
        .push_back(Lexema::E())
        .ret(Lexema::E),

    Rule("if-else").push_back(Lexema::IF())
        .push_back(Lexema::LPAR())
        .push_back(Lexema::E())
        .push_back(Lexema::RPAR())
        .push_back(Lexema::THEN())
        .push_back(Lexema::E())
        .push_back(Lexema::ELSE())
        .push_back(Lexema::E())
        .ret(Lexema::E),

    Rule("let").push_back(Lexema::LET())
        .push_back(Lexema::A())
        .push_back(Lexema::EQU())
        .push_back(Lexema::E())
        .ret(Lexema::E),

    Rule("ops").push_back(Lexema::E())
        .push_back(Lexema::SEMICOLON())
        .push_back(Lexema::E())
        .ret(Lexema::E),

    //Rule("exprs").push_back(Lexema::E())
    //    .push_back(Lexema::COM())
    //    .push_back(Lexema::E())
    //    .ret(Lexema::E),

    Rule("term_sum").push_back(Lexema::E())
        .push_back(Lexema::SUM())
        .push_back(Lexema::E())
        .ret(Lexema::E),

    Rule("term_dif").push_back(Lexema::E())
        .push_back(Lexema::DIF())
        .push_back(Lexema::E())
        .ret(Lexema::E),

    //Rule("term").push_back(Lexema::E())
    //    .ret(Lexema::E),

    //Rule("factor").push_back(Lexema::E())
    //    .ret(Lexema::E),

    //Rule("atom").push_back(Lexema::A())
    //    .ret(Lexema::T),

    Rule("factor_mul").push_back(Lexema::E())
        .push_back(Lexema::MUL())
        .push_back(Lexema::E())
        .ret(Lexema::E),

    Rule("factor_div").push_back(Lexema::F())
        .push_back(Lexema::DIV())
        .push_back(Lexema::E())
        .ret(Lexema::F),

    Rule("atom_pars").push_back(Lexema::LPAR())
        .push_back(Lexema::E())
        .push_back(Lexema::RPAR())
        .ret(Lexema::E),

    //Rule("ids").push_back(Lexema::E())
    //    .ret(Lexema::E),

    Rule("id").push_back(Lexema::A())
        .ret(Lexema::E),
};

#endif // PARSER_RULES_H
