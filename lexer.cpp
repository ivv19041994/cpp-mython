#include "lexer.h"

#include <algorithm>
#include <charconv>
#include <unordered_map>
#include <cassert>

#include <iostream>
using namespace std;

namespace parse {

const std::unordered_map<std::string_view, Token> Lexer::official_words{
    {"class", token_type::Class{} },
    {"return", token_type::Return{} },
    {"if", token_type::If{} },
    {"else", token_type::Else{} },
    {"def", token_type::Def{} },
    {"print", token_type::Print{} },
    {"and", token_type::And{} },
    {"or", token_type::Or{} },
    {"not", token_type::Not{} },
    {"==", token_type::Eq{} },
    {"!=", token_type::NotEq{} },
    {"<=", token_type::LessOrEq{} },
    {">=", token_type::GreaterOrEq{} },
    {"None", token_type::None{} },
    {"True", token_type::True{} },
    {"False", token_type::False{} }
};

bool operator==(const Token& lhs, const Token& rhs) {
    using namespace token_type;

    if (lhs.index() != rhs.index()) {
        return false;
    }
    if (lhs.Is<Char>()) {
        return lhs.As<Char>().value == rhs.As<Char>().value;
    }
    if (lhs.Is<Number>()) {
        return lhs.As<Number>().value == rhs.As<Number>().value;
    }
    if (lhs.Is<String>()) {
        return lhs.As<String>().value == rhs.As<String>().value;
    }
    if (lhs.Is<Id>()) {
        return lhs.As<Id>().value == rhs.As<Id>().value;
    }
    return true;
}

bool operator!=(const Token& lhs, const Token& rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const Token& rhs) {
    using namespace token_type;

#define VALUED_OUTPUT(type) \
    if (auto p = rhs.TryAs<type>()) return os << #type << '{' << p->value << '}';

    VALUED_OUTPUT(Number);
    VALUED_OUTPUT(Id);
    VALUED_OUTPUT(String);
    VALUED_OUTPUT(Char);

#undef VALUED_OUTPUT

#define UNVALUED_OUTPUT(type) \
    if (rhs.Is<type>()) return os << #type;

    UNVALUED_OUTPUT(Class);
    UNVALUED_OUTPUT(Return);
    UNVALUED_OUTPUT(If);
    UNVALUED_OUTPUT(Else);
    UNVALUED_OUTPUT(Def);
    UNVALUED_OUTPUT(Newline);
    UNVALUED_OUTPUT(Print);
    UNVALUED_OUTPUT(Indent);
    UNVALUED_OUTPUT(Dedent);
    UNVALUED_OUTPUT(And);
    UNVALUED_OUTPUT(Or);
    UNVALUED_OUTPUT(Not);
    UNVALUED_OUTPUT(Eq);
    UNVALUED_OUTPUT(NotEq);
    UNVALUED_OUTPUT(LessOrEq);
    UNVALUED_OUTPUT(GreaterOrEq);
    UNVALUED_OUTPUT(None);
    UNVALUED_OUTPUT(True);
    UNVALUED_OUTPUT(False);
    UNVALUED_OUTPUT(Eof);

#undef UNVALUED_OUTPUT

    return os << "Unknown token :("sv;
}

Lexer::Lexer(std::istream& input) : input_{input} {
    do {
        current_token_ = PullNextToken();
    } while(current_token_ == token_type::Newline{});
}

const Token& Lexer::CurrentToken() const {
    return current_token_;
}
    
 bool Lexer::IsStringBegin(char c) {
    return c == '\'' || c == '\"';
}

    
Token Lexer::NextToken() {
    current_token_ = PullNextToken();
    return current_token_;
}
    
std::string Lexer::ConvertStringToUser(std::string_view input) {
    std::stringstream ss;
    input = input.substr(1, input.size() - 2);
    for(size_t i = 0; i < input.size(); ++i) {
        if(input[i] == '\\') {
            switch(input[++i]) {
                case 't':
                    ss << static_cast<char>('\t');
                    continue;
                case 'n':
                    ss << static_cast<char>('\n');
                    continue;
            }
        }
        ss << static_cast<char>(input[i]);
    }
    return ss.str();
}

Token Lexer::LexToToken(std::string_view lex) {
    using namespace std::string_view_literals;
    using namespace std::string_literals;

    auto it = official_words.find(lex);
    if (it != official_words.end()) {
        return it->second;
    }

    if (isalpha(lex[0]) || lex[0] == UNDERSCORE) {
        return token_type::Id{ std::string(lex) };
    }

    if (isdigit(lex[0])) {
        return token_type::Number{ std::stoi(std::string(lex)) };
    }

    if (lex.size() == 1) {
        return token_type::Char{ lex[0] };
    }

    if (IsStringBegin(lex[0]) && lex.size() >= 2) {
        return token_type::String{ ConvertStringToUser(lex) };
    }

    throw std::logic_error("Unknown symbol: "s + std::string(lex));
}

std::string_view Lexer::GetString(std::string_view line) {
    char stop_symbol = line[0];
    size_t size = 1;

    for (; line[size] != stop_symbol; ++size) {
        if (line[size] == '\\') {
            ++size;
        }
    }
    return line.substr(0, size + 1);
}

std::pair<std::string_view, std::string_view> Lexer::GetNextLex(std::string_view line) {

    if (line.size() == 1) {
        return { line, "" };
    }

    if (line[0] == COMMENT_FRONT) {
        return { line, "" };
    }

    auto generate_result = [line](size_t lex_len) {
        std::string_view lex = line.substr(0, lex_len);
        std::string_view right_line = line.substr(lex_len);

        return std::pair{ lex, Lexer::TrimLeft(right_line) };
    };

    if (isalnum(line[0]) || line[0] == UNDERSCORE) {
        //протяженное слово
        size_t len = 1;
        for (; len < line.size() && (isalnum(line[len]) || line[len] == UNDERSCORE); ++len);
        return generate_result(len);
    }

    if (line[0] == '!' || line[0] == '<' || line[0] == '>' || line[0] == '=') {

        if (line[1] == '=') {
            return generate_result(2);
        }
        return generate_result(1);
    }

    if (IsStringBegin(line[0])) {
        return generate_result(GetString(line).size());
    }

    return generate_result(1);
}

std::list<Token> Lexer::SplitLineOnTokens(std::string_view line) {
    std::list<Token> retval{};
    while (line.size()) {
        auto [lex, right_line] = GetNextLex(line);
        line = right_line;
        if (lex[0] != COMMENT_FRONT) {
            retval.push_back(LexToToken(lex));
        }
    }
    return retval;
}

std::string_view Lexer::TrimLeft(std::string_view line) {
    auto beg = line.find_first_not_of(TRIM_SYMBOLS);
    if (beg == line.npos) {
        return "";
    }
    return line.substr(beg);
}

Lexer::Line Lexer::SplitLine(std::string_view line) {
    Line retval;
    size_t indent = line.find_first_not_of(SPACE);
    assert(indent % SPACE_ON_INDENT == 0);
    retval.indent = indent / SPACE_ON_INDENT;
    line = TrimLeft(line);
    assert(line.size());
    retval.tokens = SplitLineOnTokens(line);
    retval.tokens.push_back(token_type::Newline{});
    return retval;
} 

std::optional<Lexer::Line> Lexer::GetNextLine() {
    std::string line;

    while (std::getline(input_, line)) {
        std::string_view line_view{ line };
        if (line_view.size()) {//если от длины что то осталось, значит строка не пустая, бинго
            Line line = SplitLine(line_view);
            if (line.tokens.size() > 1) {//если там что то помимо Newline
                return line;
            }
        }
    }

    return {};
}

Token Lexer::PullNextToken() {

    if (indent_queue_) {
        --indent_queue_;
        return token_type::Indent{};
    }

    if (dedent_queue_) {
        --dedent_queue_;
        return token_type::Dedent{};
    }

    if (current_line_.tokens.size()) {
        Token token = current_line_.tokens.front();
        current_line_.tokens.pop_front();
        return token;
    }

    auto line = GetNextLine();
    if (!line) {
        if (indent_ == 0) {
            return token_type::Eof{};
        }
        dedent_queue_ = indent_ - 1;
        indent_ = 0;
        return token_type::Dedent{};
    }

    current_line_ = std::move(*line);

    assert(current_line_.tokens.size() > 0);

    if (current_line_.indent > indent_) {
        indent_queue_ = current_line_.indent - indent_;
        indent_ = current_line_.indent;
    }
    else if (current_line_.indent < indent_) {
        dedent_queue_ = indent_ - current_line_.indent;
        indent_ = current_line_.indent;
    }

    return PullNextToken();//не глубже 2, так как после чтения лайна в нем есть хотя бы один токен
}

}  // namespace parse