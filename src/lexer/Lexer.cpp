#include "Lexer.h"
#include <cctype>
#include <iostream>

namespace zlang {

// ============================================================================
// 키워드 맵 초기화
// ============================================================================

const std::unordered_map<std::string, TokenType> Lexer::keywords = {
    // 함수 및 제어 흐름
    {"fn",       TokenType::KW_FN},
    {"let",      TokenType::KW_LET},
    {"return",   TokenType::KW_RETURN},
    {"if",       TokenType::KW_IF},
    {"else",     TokenType::KW_ELSE},
    {"while",    TokenType::KW_WHILE},

    // 리터럴
    {"true",     TokenType::KW_TRUE},
    {"false",    TokenType::KW_FALSE},

    // 타입
    {"i32",      TokenType::KW_I32},
    {"i64",      TokenType::KW_I64},
    {"f32",      TokenType::KW_F32},
    {"f64",      TokenType::KW_F64},
    {"bool",     TokenType::KW_BOOL},
    {"void",     TokenType::KW_VOID},
    {"string",   TokenType::KW_STRING},

    // 【 Step 3: Exception Handling 】
    {"try",      TokenType::KW_TRY},
    {"catch",    TokenType::KW_CATCH},
    {"finally",  TokenType::KW_FINALLY},
    {"throw",    TokenType::KW_THROW},
};

// ============================================================================
// 생성자
// ============================================================================

Lexer::Lexer(const std::string& src)
    : source(src), position(0), line(1), column(1) {
    if (!source.empty()) {
        current_char = source[0];
    }
}

// ============================================================================
// 토큰화 (진입점)
// ============================================================================

std::vector<Token> Lexer::tokenize() {
    tokens.clear();
    errors.clear();

    while (position < source.length()) {
        skipWhitespaceAndComments();

        if (position >= source.length()) break;

        // 식별자 또는 키워드
        if (isIdentifierStart(current_char)) {
            tokens.push_back(readIdentifierOrKeyword());
        }
        // 숫자
        else if (isDigit(current_char)) {
            tokens.push_back(readNumber());
        }
        // 문자열
        else if (current_char == '"') {
            tokens.push_back(readString());
        }
        // 연산자 및 구두점
        else {
            tokens.push_back(readOperatorOrPunctuation());
        }
    }

    // EOF 토큰 추가
    tokens.push_back(Token(TokenType::END_OF_FILE, "", "", line, column));

    return tokens;
}

// ============================================================================
// 【 내부 헬퍼 메서드 】
// ============================================================================

void Lexer::advance() {
    if (position < source.length()) {
        if (current_char == '\n') {
            line++;
            column = 1;
        } else {
            column++;
        }
        position++;
        if (position < source.length()) {
            current_char = source[position];
        } else {
            current_char = '\0';
        }
    }
}

char Lexer::peek() const {
    if (position + 1 < source.length()) {
        return source[position + 1];
    }
    return '\0';
}

bool Lexer::match(char c) {
    if (current_char == c) {
        advance();
        return true;
    }
    return false;
}

void Lexer::skipWhitespaceAndComments() {
    while (position < source.length()) {
        // 공백 건너뛰기
        if (isWhitespace(current_char)) {
            advance();
            continue;
        }

        // 주석: // ... (라인 끝까지)
        if (current_char == '/' && peek() == '/') {
            advance();  // /
            advance();  // /
            while (position < source.length() && current_char != '\n') {
                advance();
            }
            if (current_char == '\n') {
                advance();
            }
            continue;
        }

        // 블록 주석: /* ... */
        if (current_char == '/' && peek() == '*') {
            advance();  // /
            advance();  // *
            while (position < source.length()) {
                if (current_char == '*' && peek() == '/') {
                    advance();  // *
                    advance();  // /
                    break;
                }
                advance();
            }
            continue;
        }

        break;
    }
}

Token Lexer::readIdentifierOrKeyword() {
    int start_line = line;
    int start_col = column;
    std::string lexeme;

    while (isIdentifierContinue(current_char)) {
        lexeme += current_char;
        advance();
    }

    // 키워드 확인
    auto it = keywords.find(lexeme);
    if (it != keywords.end()) {
        return Token(it->second, lexeme, lexeme, start_line, start_col);
    }

    // 식별자
    return Token(TokenType::IDENTIFIER, lexeme, lexeme, start_line, start_col);
}

Token Lexer::readNumber() {
    int start_line = line;
    int start_col = column;
    std::string lexeme;
    bool is_float = false;

    // 정수 부분 읽기
    while (isDigit(current_char)) {
        lexeme += current_char;
        advance();
    }

    // 소수점 확인
    if (current_char == '.' && isDigit(peek())) {
        is_float = true;
        lexeme += current_char;
        advance();  // .

        // 소수 부분 읽기
        while (isDigit(current_char)) {
            lexeme += current_char;
            advance();
        }
    }

    // 지수 표기법: 1.5e10, 3e-5
    if ((current_char == 'e' || current_char == 'E') &&
        (isDigit(peek()) || (peek() == '-' && position + 2 < source.length() &&
                             isDigit(source[position + 2])))) {
        is_float = true;
        lexeme += current_char;
        advance();  // e or E

        if (current_char == '-' || current_char == '+') {
            lexeme += current_char;
            advance();
        }

        while (isDigit(current_char)) {
            lexeme += current_char;
            advance();
        }
    }

    TokenType type = is_float ? TokenType::FLOAT : TokenType::INTEGER;
    return Token(type, lexeme, lexeme, start_line, start_col);
}

Token Lexer::readString() {
    int start_line = line;
    int start_col = column;
    std::string lexeme;

    advance();  // " (여는 따옴표 건너뛰기)

    while (position < source.length() && current_char != '"') {
        if (current_char == '\\') {
            // 이스케이프 시퀀스 처리
            advance();
            if (position < source.length()) {
                switch (current_char) {
                    case 'n':  lexeme += '\n'; break;
                    case 't':  lexeme += '\t'; break;
                    case 'r':  lexeme += '\r'; break;
                    case '\\': lexeme += '\\'; break;
                    case '"':  lexeme += '"'; break;
                    default:   lexeme += current_char; break;
                }
                advance();
            }
        } else {
            lexeme += current_char;
            advance();
        }
    }

    if (current_char == '"') {
        advance();  // " (닫는 따옴표 건너뛰기)
    } else {
        reportError("Unterminated string at line " + std::to_string(start_line));
    }

    return Token(TokenType::STRING, lexeme, lexeme, start_line, start_col);
}

Token Lexer::readOperatorOrPunctuation() {
    int start_line = line;
    int start_col = column;
    char ch = current_char;
    advance();

    // 2글자 연산자 확인
    if (ch == '=' && current_char == '=') {
        advance();
        return Token(TokenType::EQ, "==", "", start_line, start_col);
    }
    if (ch == '!' && current_char == '=') {
        advance();
        return Token(TokenType::NE, "!=", "", start_line, start_col);
    }
    if (ch == '<' && current_char == '=') {
        advance();
        return Token(TokenType::LE, "<=", "", start_line, start_col);
    }
    if (ch == '>' && current_char == '=') {
        advance();
        return Token(TokenType::GE, ">=", "", start_line, start_col);
    }
    if (ch == '&' && current_char == '&') {
        advance();
        return Token(TokenType::AND, "&&", "", start_line, start_col);
    }
    if (ch == '|' && current_char == '|') {
        advance();
        return Token(TokenType::OR, "||", "", start_line, start_col);
    }
    if (ch == '-' && current_char == '>') {
        advance();
        return Token(TokenType::ARROW, "->", "", start_line, start_col);
    }
    if (ch == '#' && current_char == '[') {
        advance();
        return Token(TokenType::HASH_BRACKET, "#[", "", start_line, start_col);
    }

    // 1글자 토큰
    switch (ch) {
        case '+': return Token(TokenType::PLUS, "+", "", start_line, start_col);
        case '-': return Token(TokenType::MINUS, "-", "", start_line, start_col);
        case '*': return Token(TokenType::STAR, "*", "", start_line, start_col);
        case '/': return Token(TokenType::SLASH, "/", "", start_line, start_col);
        case '%': return Token(TokenType::PERCENT, "%", "", start_line, start_col);
        case '^': return Token(TokenType::CARET, "^", "", start_line, start_col);
        case '!': return Token(TokenType::NOT, "!", "", start_line, start_col);
        case '=': return Token(TokenType::ASSIGN, "=", "", start_line, start_col);
        case '<': return Token(TokenType::LT, "<", "", start_line, start_col);
        case '>': return Token(TokenType::GT, ">", "", start_line, start_col);
        case '(': return Token(TokenType::LPAREN, "(", "", start_line, start_col);
        case ')': return Token(TokenType::RPAREN, ")", "", start_line, start_col);
        case '{': return Token(TokenType::LBRACE, "{", "", start_line, start_col);
        case '}': return Token(TokenType::RBRACE, "}", "", start_line, start_col);
        case '[': return Token(TokenType::LBRACKET, "[", "", start_line, start_col);
        case ']': return Token(TokenType::RBRACKET, "]", "", start_line, start_col);
        case ';': return Token(TokenType::SEMICOLON, ";", "", start_line, start_col);
        case ',': return Token(TokenType::COMMA, ",", "", start_line, start_col);
        case ':': return Token(TokenType::COLON, ":", "", start_line, start_col);
        case '.': return Token(TokenType::DOT, ".", "", start_line, start_col);
        case '&': return Token(TokenType::AMPERSAND, "&", "", start_line, start_col);
        default:
            reportError("Unexpected character '" + std::string(1, ch) +
                       "' at line " + std::to_string(start_line) +
                       " column " + std::to_string(start_col));
            return Token(TokenType::UNKNOWN, std::string(1, ch), "", start_line, start_col);
    }
}

void Lexer::reportError(const std::string& message) {
    errors.push_back(message);
}

// ============================================================================
// 【 유틸리티 메서드 】
// ============================================================================

bool Lexer::isIdentifierStart(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool Lexer::isIdentifierContinue(char c) {
    return isIdentifierStart(c) || isDigit(c);
}

bool Lexer::isDigit(char c) {
    return c >= '0' && c <= '9';
}

bool Lexer::isHexDigit(char c) {
    return isDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

bool Lexer::isWhitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

} // namespace zlang
