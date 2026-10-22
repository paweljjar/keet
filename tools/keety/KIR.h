//
// Created by PAWEL on 22/10/2026.
// KIR.h - KeetScript Intermediate Representation (K-IR) 🐱
//

#ifndef KEETY_KIR_H
#define KEETY_KIR_H

#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "AST.h"
#include "Token.h"

namespace Keety {

    // =========================================================================
    // K-IR Type System
    // =========================================================================

    enum class KIRBaseType {
        Void,
        Int32,
        Int64,
        Float32,
        Float64,
        Bool,
        String,
        Buffer,
        Auto,
        Custom
    };

    inline std::ostream& operator<<(std::ostream& os, KIRBaseType base) {
        switch (base) {
            case KIRBaseType::Void: return os << "Void";
            case KIRBaseType::Int32: return os << "Int32";
            case KIRBaseType::Int64: return os << "Int64";
            case KIRBaseType::Float32: return os << "Float32";
            case KIRBaseType::Float64: return os << "Float64";
            case KIRBaseType::Bool: return os << "Bool";
            case KIRBaseType::String: return os << "String";
            case KIRBaseType::Buffer: return os << "Buffer";
            case KIRBaseType::Auto: return os << "Auto";
            case KIRBaseType::Custom: return os << "Custom";
        }
        return os << "Unknown";
    }

    struct KIRType {
        KIRBaseType base = KIRBaseType::Void;
        std::string customName;
        ReferenceKind refKind = ReferenceKind::Value;
        std::string lifetime;

        static KIRType fromAnnotation(const TypeAnnotation& ann);
        static KIRType fromString(std::string_view raw);
        [[nodiscard]] std::string toString() const;
        [[nodiscard]] bool isReference() const { return refKind != ReferenceKind::Value; }
        [[nodiscard]] bool isMutableRef() const { return refKind == ReferenceKind::MutableRef; }
        [[nodiscard]] bool isImmutableRef() const { return refKind == ReferenceKind::ImmutableRef; }

        bool operator==(const KIRType& other) const {
            return base == other.base && customName == other.customName &&
                   refKind == other.refKind && lifetime == other.lifetime;
        }
    };

    // =========================================================================
    // K-IR Operands & Registers
    // =========================================================================

    enum class KIROperandKind {
        Variable,       // %varName
        Register,       // %0, %1, %2
        ConstantInt,    // 42
        ConstantFloat,  // 3.14
        ConstantString, // "hello"
        ConstantBool    // true / false
    };

    struct KIROperand {
        KIROperandKind kind = KIROperandKind::Variable;
        std::string name;
        int64_t intVal = 0;
        double floatVal = 0.0;
        std::string strVal;
        bool boolVal = false;
        KIRType type;

        static KIROperand reg(int id, KIRType t = {});
        static KIROperand var(std::string name, KIRType t = {});
        static KIROperand constInt(int64_t v);
        static KIROperand constFloat(double v);
        static KIROperand constStr(std::string s);
        static KIROperand constBool(bool b);

        [[nodiscard]] std::string toString() const;

        bool operator==(const KIROperand& o) const;
    };

    // =========================================================================
    // K-IR Instructions & Control Flow
    // =========================================================================

    enum class KIROpcode {
        Alloca,      // %dest = alloca <type>
        Store,       // store <val>, %dest
        Load,        // %dest = load %src
        BorrowRef,   // %dest = borrow_ref %src
        BorrowMut,   // %dest = borrow_mut %src
        Move,        // %dest = move %src
        Call,        // %dest = call @func(%arg1, ...)
        BinaryOp,    // %dest = <op> %lhs, %rhs
        Branch,      // branch <targetBlock>
        BranchCond,  // branch_cond %cond, <trueBlock>, <falseBlock>
        Return,      // return [%val]
        Drop,        // drop %var
        Zonk         // %dest = zonk %call, %fallback
    };

    inline std::string opcodeToString(KIROpcode op) {
        switch (op) {
            case KIROpcode::Alloca: return "alloca";
            case KIROpcode::Store: return "store";
            case KIROpcode::Load: return "load";
            case KIROpcode::BorrowRef: return "borrow_ref";
            case KIROpcode::BorrowMut: return "borrow_mut";
            case KIROpcode::Move: return "move";
            case KIROpcode::Call: return "call";
            case KIROpcode::BinaryOp: return "binop";
            case KIROpcode::Branch: return "branch";
            case KIROpcode::BranchCond: return "branch_cond";
            case KIROpcode::Return: return "return";
            case KIROpcode::Drop: return "drop";
            case KIROpcode::Zonk: return "zonk";
        }
        return "unknown";
    }

    struct KIRInstruction {
        KIROpcode opcode = KIROpcode::Alloca;
        std::string opStr; // for BinaryOp ("+", "-", "*", "/", "==", ">=", etc.)
        KIROperand dest;
        std::vector<KIROperand> operands;
        std::string targetBlock; // For Branch and true-target for BranchCond
        std::string falseBlock;  // For BranchCond
        SourceLocation loc;

        [[nodiscard]] std::string toString() const;
    };

    // =========================================================================
    // K-IR Basic Block & Control Flow Graph (CFG)
    // =========================================================================

    struct KIRBasicBlock {
        std::string label;
        std::vector<KIRInstruction> instructions;
        std::vector<std::string> predecessors;
        std::vector<std::string> successors;

        explicit KIRBasicBlock(std::string lbl = "entry") : label(std::move(lbl)) {}

        void addInstruction(KIRInstruction inst) {
            instructions.push_back(std::move(inst));
        }

        [[nodiscard]] bool isTerminated() const {
            if (instructions.empty()) return false;
            auto op = instructions.back().opcode;
            return op == KIROpcode::Return || op == KIROpcode::Branch || op == KIROpcode::BranchCond;
        }
    };

    // =========================================================================
    // K-IR Function & Module
    // =========================================================================

    struct KIRParameter {
        std::string name;
        KIRType type;
        bool isMove = false;
    };

    struct KIRFunction {
        std::string name;
        std::vector<KIRParameter> parameters;
        KIRType returnType;
        std::vector<std::shared_ptr<KIRBasicBlock>> blocks;
        bool isExported = false;
        bool isMain = false;
        SourceLocation loc;

        std::shared_ptr<KIRBasicBlock> createBlock(const std::string& label) {
            auto bb = std::make_shared<KIRBasicBlock>(label);
            blocks.push_back(bb);
            return bb;
        }

        std::shared_ptr<KIRBasicBlock> entryBlock() {
            if (blocks.empty()) return createBlock("entry");
            return blocks.front();
        }

        [[nodiscard]] std::shared_ptr<KIRBasicBlock> findBlock(const std::string& label) const {
            for (const auto& bb : blocks) {
                if (bb->label == label) return bb;
            }
            return nullptr;
        }
    };

    struct KIRStructField {
        std::string name;
        KIRType type;
        std::string defaultExpr;
    };

    struct KIRStruct {
        std::string name;
        bool isExported = false;
        std::vector<KIRStructField> fields;
    };

    class KIRModule {
    public:
        std::string name;
        std::vector<std::string> imports;
        std::vector<KIRStruct> structs;
        std::vector<std::shared_ptr<KIRFunction>> functions;

        void addFunction(std::shared_ptr<KIRFunction> fn) {
            functions.push_back(std::move(fn));
        }

        [[nodiscard]] std::shared_ptr<KIRFunction> findFunction(const std::string& fnName) const {
            for (const auto& fn : functions) {
                if (fn->name == fnName) return fn;
            }
            return nullptr;
        }

        // Binary serialization for incremental cache (.kir)
        void serialize(std::ostream& os) const;
        static std::unique_ptr<KIRModule> deserialize(std::istream& is);

        [[nodiscard]] std::vector<uint8_t> toBinary() const;
        static std::unique_ptr<KIRModule> fromBinary(const std::vector<uint8_t>& bytes);

        [[nodiscard]] std::string print() const;
    };

    // =========================================================================
    // K-IR Builder & Printer
    // =========================================================================

    class KIRBuilder {
    public:
        KIRBuilder() = default;

        std::unique_ptr<KIRModule> build(const SourceFile& ast);

        std::shared_ptr<KIRFunction> buildFunction(
            const std::string& name,
            const std::vector<ParameterDecl>& params,
            const std::string& returnType,
            const std::vector<Token>& bodyTokens,
            const SourceLocation& loc,
            bool isExported = false,
            bool isMain = false
        );

    private:
        int _regCounter = 0;
        int _blockCounter = 0;

        KIROperand _nextReg(KIRType t = {});
        std::string _nextBlockLabel(const std::string& prefix = "bb");
    };

    class KIRPrinter {
    public:
        static std::string print(const KIRModule& mod);
        static std::string printFunction(const KIRFunction& fn);
        static std::string printBlock(const KIRBasicBlock& bb);
        static std::string printInstruction(const KIRInstruction& inst);
    };

} // namespace Keety

#endif // KEETY_KIR_H
