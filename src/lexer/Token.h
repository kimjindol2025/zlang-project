#ifndef TOKEN_H
#define TOKEN_H

#include <string>
#include <ostream>

namespace zlang {

/**
 * TokenType: Z-Lang의 모든 토큰 타입
 *
 * 【 어휘 분석(Lexing)의 산물 】
 * 소스 코드 → Lexer → Token 스트림 → Parser
 */
enum class TokenType {
    // 특수 토큰
    END_OF_FILE = 0,
    NEWLINE,

    // 리터럴
    INTEGER,       // 123, 456
    FLOAT,         // 3.14, 2.71
    BOOL_TRUE,     // true
    BOOL_FALSE,    // false
    STRING,        // "hello"
    IDENTIFIER,    // variable_name

    // 키워드
    KW_FN,         // fn
    KW_LET,        // let
    KW_RETURN,     // return
    KW_IF,         // if
    KW_ELSE,       // else
    KW_WHILE,      // while
    KW_TRUE,       // true (literal로도 사용)
    KW_FALSE,      // false (literal로도 사용)

    // 【 Step 3: Exception Handling Keywords 】
    KW_TRY,        // try
    KW_CATCH,      // catch
    KW_FINALLY,    // finally
    KW_THROW,      // throw

    // 타입 키워드
    KW_I32,        // i32
    KW_I64,        // i64
    KW_F32,        // f32
    KW_F64,        // f64
    KW_BOOL,       // bool
    KW_VOID,       // void
    KW_STRING,     // string

    // 연산자
    PLUS,          // +
    MINUS,         // -
    STAR,          // *
    SLASH,         // /
    PERCENT,       // %
    CARET,         // ^

    // 비교 연산자
    EQ,            // ==
    NE,            // !=
    LT,            // <
    GT,            // >
    LE,            // <=
    GE,            // >=

    // 논리 연산자
    AND,           // &&
    OR,            // ||
    NOT,           // !

    // 할당 연산자
    ASSIGN,        // =

    // 구두점
    LPAREN,        // (
    RPAREN,        // )
    LBRACE,        // {
    RBRACE,        // }
    LBRACKET,      // [
    RBRACKET,      // ]
    SEMICOLON,     // ;
    COMMA,         // ,
    COLON,         // :
    DOT,           // .
    ARROW,         // ->
    AMPERSAND,     // & (참조)

    // 속성 (Attribute)
    HASH_BRACKET,  // #[ (속성 시작)

    // 오류
    UNKNOWN,
    ERROR
};

/**
 * Token: 단일 토큰을 표현
 *
 * 예:
 *   source: "let x: i64 = 42;"
 *   tokens:
 *     Token(KW_LET, "let", line=1, col=1)
 *     Token(IDENTIFIER, "x", line=1, col=5)
 *     Token(COLON, ":", line=1, col=6)
 *     Token(KW_I64, "i64", line=1, col=8)
 *     Token(ASSIGN, "=", line=1, col=12)
 *     Token(INTEGER, "42", line=1, col=14)
 *     Token(SEMICOLON, ";", line=1, col=16)
 */
struct Token {
    TokenType type;
    std::string lexeme;    // 원본 텍스트 (예: "123", "variable_name")
    std::string literal;   // 파싱된 값 (예: integer 123, float 3.14)
    int line;
    int column;

    Token()
        : type(TokenType::END_OF_FILE), lexeme(""), literal(""),
          line(0), column(0) {}

    Token(TokenType t, const std::string& lex, const std::string& lit = "",
          int l = 0, int c = 0)
        : type(t), lexeme(lex), literal(lit), line(l), column(c) {}

    // 디버깅용 출력
    std::string toString() const {
        return std::string(
            "[" + std::to_string(line) + ":" + std::to_string(column) +
            "] " + lexeme + " (" + std::to_string(static_cast<int>(type)) + ")"
        );
    }
};

/**
 * TokenType → 문자열 변환 (디버깅용)
 */
inline std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::END_OF_FILE:    return "EOF";
        case TokenType::NEWLINE:        return "NEWLINE";
        case TokenType::INTEGER:        return "INTEGER";
        case TokenType::FLOAT:          return "FLOAT";
        case TokenType::BOOL_TRUE:      return "TRUE";
        case TokenType::BOOL_FALSE:     return "FALSE";
        case TokenType::STRING:         return "STRING";
        case TokenType::IDENTIFIER:     return "IDENTIFIER";
        case TokenType::KW_FN:          return "fn";
        case TokenType::KW_LET:         return "let";
        case TokenType::KW_RETURN:      return "return";
        case TokenType::KW_IF:          return "if";
        case TokenType::KW_ELSE:        return "else";
        case TokenType::KW_WHILE:       return "while";
        case TokenType::KW_I32:         return "i32";
        case TokenType::KW_I64:         return "i64";
        case TokenType::KW_F32:         return "f32";
        case TokenType::KW_F64:         return "f64";
        case TokenType::KW_BOOL:        return "bool";
        case TokenType::KW_VOID:        return "void";
        case TokenType::KW_STRING:      return "string";
        case TokenType::PLUS:           return "+";
        case TokenType::MINUS:          return "-";
        case TokenType::STAR:           return "*";
        case TokenType::SLASH:          return "/";
        case TokenType::PERCENT:        return "%";
        case TokenType::CARET:          return "^";
        case TokenType::EQ:             return "==";
        case TokenType::NE:             return "!=";
        case TokenType::LT:             return "<";
        case TokenType::GT:             return ">";
        case TokenType::LE:             return "<=";
        case TokenType::GE:             return ">=";
        case TokenType::AND:            return "&&";
        case TokenType::OR:             return "||";
        case TokenType::NOT:            return "!";
        case TokenType::ASSIGN:         return "=";
        case TokenType::LPAREN:         return "(";
        case TokenType::RPAREN:         return ")";
        case TokenType::LBRACE:         return "{";
        case TokenType::RBRACE:         return "}";
        case TokenType::LBRACKET:       return "[";
        case TokenType::RBRACKET:       return "]";
        case TokenType::SEMICOLON:      return ";";
        case TokenType::COMMA:          return ",";
        case TokenType::COLON:          return ":";
        case TokenType::DOT:            return ".";
        case TokenType::ARROW:          return "->";
        case TokenType::AMPERSAND:      return "&";
        case TokenType::HASH_BRACKET:   return "#[";
        default:                        return "UNKNOWN";
    }
}

} // namespace zlang

#endif // TOKEN_H
