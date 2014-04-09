#include "compiler/build_tables/conflict_manager.h"
#include <vector>
#include <map>
#include <string>

namespace tree_sitter {
    namespace build_tables {
        using rules::Symbol;
        using std::vector;
        using std::string;
        using std::map;

        string message_for_action(const ParseAction &action, const map<Symbol, string> &rule_names) {
            switch (action.type) {
                case ParseActionTypeShift:
                    return "shift";
                case ParseActionTypeReduce: {
                    auto pair = rule_names.find(action.symbol);
                    if (pair != rule_names.end())
                        return "reduce " + pair->second;
                    else
                        return "ERROR " + action.symbol.name;
                }
                case ParseActionTypeAccept:
                    return "accept";
                case ParseActionTypeError:
                    return "error";
                    break;
            }
        }

        void ConflictManager::record_conflict(const rules::Symbol &symbol, const ParseAction &left, const ParseAction &right) {
            conflicts_.push_back(Conflict(rule_names.find(symbol)->second + ": " + message_for_action(left, rule_names) + " / " + message_for_action(right, rule_names)));
        }

        ConflictManager::ConflictManager(const PreparedGrammar &parse_grammar,
                                         const PreparedGrammar &lex_grammar,
                                         const map<Symbol, string> &rule_names) :
            parse_grammar(parse_grammar),
            lex_grammar(lex_grammar),
            rule_names(rule_names)
            {}

        bool ConflictManager::resolve_parse_action(const rules::Symbol &symbol,
                                                   const ParseAction &old_action,
                                                   const ParseAction &new_action) {
            if (new_action.type < old_action.type)
                return !resolve_parse_action(symbol, new_action, old_action);
            
            switch (old_action.type) {
                case ParseActionTypeError:
                    return true;
                case ParseActionTypeShift:
                    switch (new_action.type) {
                        case ParseActionTypeShift:
                            record_conflict(symbol, old_action, new_action);
                            return false;
                        case ParseActionTypeReduce:
                            record_conflict(symbol, old_action, new_action);
                            return false;
                        default:
                            return false;
                    }
                case ParseActionTypeReduce:
                    switch (new_action.type) {
                        case ParseActionTypeReduce: {
                            record_conflict(symbol, old_action, new_action);
                            size_t old_index = parse_grammar.index_of(old_action.symbol);
                            size_t new_index = parse_grammar.index_of(new_action.symbol);
                            return new_index < old_index;
                        }
                        default:
                            return false;
                    }
                default:
                    return false;
            }
        }

        bool ConflictManager::resolve_lex_action(const LexAction &old_action,
                                                 const LexAction &new_action) {
            switch (old_action.type) {
                case LexActionTypeError:
                    return true;
                case LexActionTypeAccept:
                    if (new_action.type == LexActionTypeAccept) {
                        size_t old_index = lex_grammar.index_of(old_action.symbol);
                        size_t new_index = lex_grammar.index_of(new_action.symbol);
                        return (new_index < old_index);
                    }
                default:;
            }
            return false;
        }

        const vector<Conflict> & ConflictManager::conflicts() const {
            return conflicts_;
        }
    }
}