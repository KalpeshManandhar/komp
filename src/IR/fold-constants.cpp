#include <optional>
#include <cmath> // for std::floor, std::ceil, etc.
#include "ir.h"
#include "number.h"



Number foldConstants(Subexpr* s) {
    switch (s->subtag) {
        case Subexpr::SUBEXPR_LEAF: {
            // Convert numeric token to Number
            TokenType VALID_LITERALS[] = {
                TOKEN_NUMERIC_BIN, TOKEN_NUMERIC_DEC, TOKEN_NUMERIC_OCT,
                TOKEN_NUMERIC_HEX, TOKEN_NUMERIC_FLOAT, TOKEN_NUMERIC_DOUBLE,
                TOKEN_CHAR,
            };

            if (_matchv(s->leaf, VALID_LITERALS, ARRAY_COUNT(VALID_LITERALS))) {
                if (s->leaf.type == TOKEN_NUMERIC_FLOAT) {
                    return f32FromString(s->leaf.string);
                } else if (s->leaf.type == TOKEN_NUMERIC_DOUBLE) {
                    return f64FromString(s->leaf.string);
                } else {
                    return Number{.type = MIR_Datatypes::_i64, .i64 = {std::stoll(s->leaf.string)}};
                }
            }
            return std::nullopt;
        }

        case Subexpr::SUBEXPR_BINARY_OP: {
            auto leftVal = foldConstants(s->binary.left);
            auto rightVal = foldConstants(s->binary.right);
            
            if (!leftVal || !rightVal) return std::nullopt; // Non-constant subexpr
            
            Number result;
            switch (s->binary.op.type) {
                case TOKEN_PLUS:
                    result.i64[0] = leftVal->i64[0] + rightVal->i64[0];
                    break;
                case TOKEN_MINUS:
                    result.i64[0] = leftVal->i64[0] - rightVal->i64[0];
                    break;
                case TOKEN_STAR:
                    result.i64[0] = leftVal->i64[0] * rightVal->i64[0];
                    break;
                case TOKEN_SLASH:
                    if (rightVal->i64[0] == 0) return std::nullopt; // Avoid division by zero
                    result.i64[0] = leftVal->i64[0] / rightVal->i64[0];
                    break;
                default:
                    return std::nullopt; // Non-foldable operator
            }
            
            // Replace current subtree with constant value
            s->subtag = Subexpr::SUBEXPR_LEAF;
            s->leaf.type = TOKEN_NUMERIC_DEC;
            s->leaf.string = std::to_string(result.i64[0]); // Convert back to string

            return result;
        }

        case Subexpr::SUBEXPR_UNARY: {
            auto val = foldConstants(s->unary.expr);
            if (!val) return std::nullopt;

            Number result;
            switch (s->unary.op.type) {
                case TOKEN_MINUS:
                    result.i64[0] = -val->i64[0];
                    break;
                case TOKEN_PLUS:
                    result.i64[0] = val->i64[0]; // No change
                    break;
                default:
                    return std::nullopt; // Non-foldable unary op
            }
            
            s->subtag = Subexpr::SUBEXPR_LEAF;
            s->leaf.type = TOKEN_NUMERIC_DEC;
            s->leaf.string = std::to_string(result.i64[0]);

            return result;
        }

        case Subexpr::SUBEXPR_RECURSE_PARENTHESIS: {
            return foldConstants(s->inside);
        }

        case Subexpr::SUBEXPR_CAST: {
            auto val = foldConstants(s->cast.expr);
            if (!val) return std::nullopt;

            // Perform type casting
            Number result = *val;
            if (s->cast.to == MIR_Datatypes::_f32) {
                result.type = MIR_Datatypes::_f32;
                result.f32[0] = static_cast<float>(val->i64[0]);
            } else if (s->cast.to == MIR_Datatypes::_f64) {
                result.type = MIR_Datatypes::_f64;
                result.f64[0] = static_cast<double>(val->i64[0]);
            } else if (s->cast.to == MIR_Datatypes::_i64) {
                result.type = MIR_Datatypes::_i64;
                result.i64[0] = static_cast<int64_t>(val->f64[0]);
            }

            s->subtag = Subexpr::SUBEXPR_LEAF;
            s->leaf.type = TOKEN_NUMERIC_DEC;
            s->leaf.string = std::to_string(result.i64[0]);

            return result;
        }

        default:
            return std::nullopt;
    }
}
