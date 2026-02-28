#ifndef AST_NODE_H
#define AST_NODE_H

#include <string>
#include <vector>
#include <memory>
#include "../codegen/Type.h"

namespace zlang {

// 모든 AST 노드의 기본 클래스
class ASTNode {
public:
    enum class NodeType {
        // 리터럴
        IntLiteral,
        FloatLiteral,
        BoolLiteral,
        StringLiteral,

        // 식 (Expression)
        BinaryOp,
        UnaryOp,
        Identifier,
        Call,
        Assignment,

        // 문 (Statement)
        VarDecl,
        Block,
        If,
        While,
        Return,

        // 【 Step 2: Result Type 】
        ResultOk,    // Result<T, E>::Ok(value)
        ResultErr,   // Result<T, E>::Err(error)
        Match,       // match expression (패턴 매칭)

        // 【 Step 3: Exception Handling (try-catch-finally) 】
        TryCatch,    // try-catch-finally 문

        // 프로그램 레벨
        Function,
        Program
    };

    NodeType type;
    Type inferred_type;  // 2.3 의미분석에서 채워짐

    explicit ASTNode(NodeType t) : type(t) {}
    virtual ~ASTNode() = default;

    virtual std::string toString() const = 0;
};

// ============================================================================
// 리터럴 노드들
// ============================================================================

class IntLiteralNode : public ASTNode {
public:
    int64_t value;

    explicit IntLiteralNode(int64_t val)
        : ASTNode(NodeType::IntLiteral), value(val) {}

    std::string toString() const override {
        return "IntLiteral(" + std::to_string(value) + ")";
    }
};

class FloatLiteralNode : public ASTNode {
public:
    double value;

    explicit FloatLiteralNode(double val)
        : ASTNode(NodeType::FloatLiteral), value(val) {}

    std::string toString() const override {
        return "FloatLiteral(" + std::to_string(value) + ")";
    }
};

class BoolLiteralNode : public ASTNode {
public:
    bool value;

    explicit BoolLiteralNode(bool val)
        : ASTNode(NodeType::BoolLiteral), value(val) {}

    std::string toString() const override {
        return "BoolLiteral(" + std::string(value ? "true" : "false") + ")";
    }
};

class StringLiteralNode : public ASTNode {
public:
    std::string value;

    explicit StringLiteralNode(const std::string& val)
        : ASTNode(NodeType::StringLiteral), value(val) {}

    std::string toString() const override {
        return "StringLiteral(\"" + value + "\")";
    }
};

// ============================================================================
// 연산 노드들
// ============================================================================

enum class BinaryOp {
    Add,    // +
    Sub,    // -
    Mul,    // *
    Div,    // /
    Mod,    // %
    Equal,  // ==
    NotEq,  // !=
    Less,   // <
    Greater,// >
    LessEq, // <=
    GreaterEq, // >=
    And,    // &&
    Or,     // ||
    Assign  // =
};

class BinaryOpNode : public ASTNode {
public:
    BinaryOp op;
    std::shared_ptr<ASTNode> left;
    std::shared_ptr<ASTNode> right;

    BinaryOpNode(BinaryOp o, std::shared_ptr<ASTNode> l, std::shared_ptr<ASTNode> r)
        : ASTNode(NodeType::BinaryOp), op(o), left(l), right(r) {}

    std::string toString() const override {
        return "BinaryOp(...)";
    }
};

enum class UnaryOp {
    Neg,    // -x (Negate)
    Not,    // !x
    Negate, // -x (동의어)
    Address // &x (주소 연산자)
};

class UnaryOpNode : public ASTNode {
public:
    UnaryOp op;
    std::shared_ptr<ASTNode> operand;

    UnaryOpNode(UnaryOp o, std::shared_ptr<ASTNode> opnd)
        : ASTNode(NodeType::UnaryOp), op(o), operand(opnd) {}

    std::string toString() const override {
        return "UnaryOp(...)";
    }
};

// ============================================================================
// 변수/함수 관련 노드들
// ============================================================================

class IdentifierNode : public ASTNode {
public:
    std::string name;

    explicit IdentifierNode(const std::string& n)
        : ASTNode(NodeType::Identifier), name(n) {}

    std::string toString() const override {
        return "Identifier(" + name + ")";
    }
};

class CallNode : public ASTNode {
public:
    std::string func_name;
    std::vector<std::shared_ptr<ASTNode>> arguments;

    CallNode(const std::string& fname, std::vector<std::shared_ptr<ASTNode>> args)
        : ASTNode(NodeType::Call), func_name(fname), arguments(args) {}

    std::string toString() const override {
        return "Call(" + func_name + ")";
    }
};

class AssignmentNode : public ASTNode {
public:
    std::string var_name;
    std::shared_ptr<ASTNode> value;

    AssignmentNode(const std::string& name, std::shared_ptr<ASTNode> val)
        : ASTNode(NodeType::Assignment), var_name(name), value(val) {}

    std::string toString() const override {
        return "Assignment(" + var_name + ")";
    }
};

// ============================================================================
// 문 노드들
// ============================================================================

class VarDeclNode : public ASTNode {
public:
    std::string var_name;
    Type declared_type;
    std::shared_ptr<ASTNode> init_expr;  // 초기값 (optional)

    VarDeclNode(const std::string& name, const Type& type, std::shared_ptr<ASTNode> init = nullptr)
        : ASTNode(NodeType::VarDecl), var_name(name), declared_type(type), init_expr(init) {}

    std::string toString() const override {
        return "VarDecl(" + var_name + ")";
    }
};

class BlockNode : public ASTNode {
public:
    std::vector<std::shared_ptr<ASTNode>> statements;

    explicit BlockNode(std::vector<std::shared_ptr<ASTNode>> stmts = {})
        : ASTNode(NodeType::Block), statements(stmts) {}

    std::string toString() const override {
        return "Block(...)";
    }
};

class IfNode : public ASTNode {
public:
    std::shared_ptr<ASTNode> condition;
    std::shared_ptr<BlockNode> then_block;
    std::shared_ptr<BlockNode> else_block;  // optional

    IfNode(std::shared_ptr<ASTNode> cond, std::shared_ptr<BlockNode> then_b,
           std::shared_ptr<BlockNode> else_b = nullptr)
        : ASTNode(NodeType::If), condition(cond), then_block(then_b), else_block(else_b) {}

    std::string toString() const override {
        return "If(...)";
    }
};

class WhileNode : public ASTNode {
public:
    std::shared_ptr<ASTNode> condition;
    std::shared_ptr<BlockNode> body;

    WhileNode(std::shared_ptr<ASTNode> cond, std::shared_ptr<BlockNode> b)
        : ASTNode(NodeType::While), condition(cond), body(b) {}

    std::string toString() const override {
        return "While(...)";
    }
};

class ReturnNode : public ASTNode {
public:
    std::shared_ptr<ASTNode> value;  // optional

    explicit ReturnNode(std::shared_ptr<ASTNode> val = nullptr)
        : ASTNode(NodeType::Return), value(val) {}

    std::string toString() const override {
        return "Return(...)";
    }
};

// ============================================================================
// 함수 정의 노드
// ============================================================================

struct Parameter {
    std::string name;
    Type type;
};

class FunctionNode : public ASTNode {
public:
    std::string func_name;
    std::vector<Parameter> parameters;
    Type return_type;
    std::shared_ptr<BlockNode> body;

    FunctionNode(const std::string& name, std::vector<Parameter> params,
                 const Type& ret_type, std::shared_ptr<BlockNode> b)
        : ASTNode(NodeType::Function), func_name(name), parameters(params),
          return_type(ret_type), body(b) {}

    std::string toString() const override {
        return "Function(" + func_name + ")";
    }
};

// ============================================================================
// 【 Step 3: try-catch-finally 노드 】
// ============================================================================

/**
 * CatchClause: catch 블록 정의
 * catch (e: ExceptionType) { ... }
 */
struct CatchClause {
    std::string exception_var;              // 예외 변수명 (e)
    std::string exception_type;             // 예외 타입 (ExceptionType)
    std::shared_ptr<BlockNode> body;        // catch 블록
};

/**
 * TryCatchNode: try-catch-finally 문
 * try { ... }
 * catch (e: Exception1) { ... }
 * catch (e: Exception2) { ... }
 * finally { ... }
 */
class TryCatchNode : public ASTNode {
public:
    std::shared_ptr<BlockNode> try_block;           // try 블록
    std::vector<CatchClause> catch_clauses;         // catch 블록들
    std::shared_ptr<BlockNode> finally_block;       // finally 블록 (선택사항)

    TryCatchNode(std::shared_ptr<BlockNode> try_b,
                 std::vector<CatchClause> catches = {},
                 std::shared_ptr<BlockNode> finally_b = nullptr)
        : ASTNode(NodeType::TryCatch), try_block(try_b),
          catch_clauses(catches), finally_block(finally_b) {}

    std::string toString() const override {
        return "TryCatch(" + std::to_string(catch_clauses.size()) + " catches" +
               (finally_block ? ", finally" : "") + ")";
    }
};

// ============================================================================
// 【 Step 2: Result 타입 노드들 】
// ============================================================================

/**
 * ResultOkNode: Result<T, E>::Ok(value)
 * 성공 값을 wrapping하는 노드
 */
class ResultOkNode : public ASTNode {
public:
    std::shared_ptr<ASTNode> value;

    explicit ResultOkNode(std::shared_ptr<ASTNode> val)
        : ASTNode(NodeType::ResultOk), value(val) {}

    std::string toString() const override {
        return "Ok(" + (value ? value->toString() : "?") + ")";
    }
};

/**
 * ResultErrNode: Result<T, E>::Err(error)
 * 오류 값을 wrapping하는 노드
 */
class ResultErrNode : public ASTNode {
public:
    std::shared_ptr<ASTNode> error;

    explicit ResultErrNode(std::shared_ptr<ASTNode> err)
        : ASTNode(NodeType::ResultErr), error(err) {}

    std::string toString() const override {
        return "Err(" + (error ? error->toString() : "?") + ")";
    }
};

/**
 * MatchArmNode: match의 한 가지 패턴
 * match x {
 *     Ok(v) => { ... },
 *     Err(e) => { ... },
 * }
 */
struct MatchArm {
    std::string pattern;                        // "Ok" 또는 "Err"
    std::shared_ptr<ASTNode> pattern_expr;      // Ok(v) 또는 Err(e)
    std::shared_ptr<BlockNode> body;            // => 다음의 블록
};

/**
 * MatchNode: match 표현식
 * Result<T, E> 타입의 값을 패턴 매칭으로 처리
 */
class MatchNode : public ASTNode {
public:
    std::shared_ptr<ASTNode> expr;              // match 대상 (보통 Result)
    std::vector<MatchArm> arms;                 // match 패턴들

    MatchNode(std::shared_ptr<ASTNode> e, std::vector<MatchArm> a)
        : ASTNode(NodeType::Match), expr(e), arms(a) {}

    std::string toString() const override {
        return "Match(" + (expr ? expr->toString() : "?") + ", " +
               std::to_string(arms.size()) + " arms)";
    }
};

// ============================================================================
// 프로그램 노드 (최상위)
// ============================================================================

class ProgramNode : public ASTNode {
public:
    std::vector<std::shared_ptr<FunctionNode>> functions;

    explicit ProgramNode(std::vector<std::shared_ptr<FunctionNode>> funcs = {})
        : ASTNode(NodeType::Program), functions(funcs) {}

    std::string toString() const override {
        return "Program(...)";
    }
};

} // namespace zlang

#endif // AST_NODE_H
