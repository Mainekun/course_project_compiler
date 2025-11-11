#include "parser.h"
#include <string>

[[nodiscard]] bool Parser::analyze() {
    QList<Lexema> observe_list;

    foreach(auto i, __lexer->get_tokenized_code())
        __line.push_back(i);

    __line.push_back(Lexema("$", TokenType::Delimeter));

    __stack.push(Lexema("^", TokenType::Delimeter));

    while (!__line.isEmpty()) {

        Lexema stack_lex = __stack.top();
        if(stack_lex.type() == TokenType::Nonterminal)
            stack_lex = __stack.at(__stack.length()-2);

        Lexema line_lex = __line.front();

        if ((int)line_lex.type() & ((int)TokenType::Id | (int)TokenType::Const))
            line_lex = Lexema::A();

        if (!(parser_rules.contains(line_lex.value()) &&
              parser_rules[line_lex.value()].contains(stack_lex.value())))
            throw std::runtime_error(QString("No realtion specified for: (%1, %2)")
                                         .arg(line_lex.value())
                                         .arg(stack_lex.value())
                                         .toStdString());

        int rel = parser_rules[line_lex.value()][stack_lex.value()];

        if (rel <= 0) {
            __stack.push(line_lex);
            __line.pop_front();
        }
        else {
            while(__stack.length() > 1) {
                observe_list.push_front(__stack.pop());

                qsizetype seq_size = observe_list.length();
                foreach (auto rule, rules) {
                    if (rule.len() != seq_size)
                        continue;

                    if (!(rule == observe_list))
                        continue;

                    if (rule.name() == "atom_pars" && Lexema::is_op(__stack.top())
                        || rule.name() == "ids" && __stack.top().value() == "="
                        || rule.name() == "id" && Lexema::is_arithm(__stack.top())
                        //|| rule.name() == "ops1" && __stack.at(__stack.length() - 2).value() == "OS"
                        //|| rule.name() == "ids" && Lexema::is_arithm(__stack.top())
                        )
                        continue;

                    __conv_seq.push_back(rule.name());
                    observe_list.clear();
                    __stack.push(rule());
                    goto rulewasfound;
                }

            }

            throw std::runtime_error("NoRuleForSequanceException");

            rulewasfound:
        }
    }

    return 1;
}
