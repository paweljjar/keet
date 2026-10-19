//
// Created by PAWEL on 19/10/2026.
//

#include "Codegen.h"
#include <algorithm>
#include <cctype>

namespace Keety {

    std::string Codegen::_mapType(const std::string& typeName) const {
        if (typeName == "string") return "std::string";
        if (typeName == "int64") return "long long";
        if (typeName == "int") return "int";
        if (typeName == "double") return "double";
        if (typeName == "float") return "float";
        if (typeName == "bool") return "bool";
        if (typeName == "void") return "void";
        if (typeName.empty()) return "void";
        return typeName;
    }

    void Codegen::_collectInternedStrings(const TemplateNode& node) {
        if (auto elem = dynamic_cast<const ElementNode*>(&node)) {
            _internedTags.insert(elem->tagName);
            for (const auto& attr : elem->attributes) {
                if (attr.kind == AttributeKind::StaticLiteral || attr.kind == AttributeKind::DynamicExpression) {
                    _internedProps.insert(attr.name);
                } else if (attr.kind == AttributeKind::TwoWayBinding) {
                    _internedProps.insert(attr.bindTargetProperty);
                    if (attr.bindTargetProperty == "value") {
                        _internedProps.insert("input");
                    } else if (attr.bindTargetProperty == "checked") {
                        _internedProps.insert("change");
                    }
                } else if (attr.kind == AttributeKind::EventCallback) {
                    std::string eventName = attr.name;
                    if (eventName.rfind("on", 0) == 0 && eventName.size() > 2) {
                        eventName = eventName.substr(2);
                        eventName[0] = static_cast<char>(std::tolower(static_cast<unsigned char>(eventName[0])));
                    }
                    _internedProps.insert(eventName);
                }
            }
            for (const auto& child : elem->children) {
                _collectInternedStrings(*child);
            }
        } else if (auto ifNode = dynamic_cast<const IfBlockNode*>(&node)) {
            for (const auto& branch : ifNode->branches) {
                for (const auto& child : branch.children) {
                    _collectInternedStrings(*child);
                }
            }
        } else if (auto forNode = dynamic_cast<const ForBlockNode*>(&node)) {
            for (const auto& child : forNode->children) {
                _collectInternedStrings(*child);
            }
        }
    }

    GeneratedCode Codegen::generate() {
        if (_headerFileName.empty()) {
            _headerFileName = _ast.componentName + ".h";
        }

        // Collect interned tags and props from template
        if (_ast.templateBlock) {
            for (const auto& root : _ast.templateBlock->roots) {
                _collectInternedStrings(*root);
            }
        }

        GeneratedCode result;
        result.headerCode = _generateHeader();
        result.sourceCode = _generateSource();
        return result;
    }

    std::string Codegen::_generateHeader() {
        std::ostringstream os;
        std::string comp = _ast.componentName;
        std::string guard = "KEET_GENERATED_" + comp + "_H";
        std::transform(guard.begin(), guard.end(), guard.begin(), ::toupper);

        os << "//\n// Automatically generated ahead-of-time by keety 🐱\n// Single-File Component: "
           << comp << "\n//\n\n";
        os << "#ifndef " << guard << "\n";
        os << "#define " << guard << "\n\n";
        os << "#include <keet/UIEngine.h>\n";
        os << "#include <string>\n";
        os << "#include <vector>\n";
        os << "#include <sstream>\n";

        // Phase 1.3: Emit native C++ bridge imports
        if (_ast.script) {
            for (const auto& imp : _ast.script->imports) {
                os << "#include \"" << imp->headerPath << "\"\n";
            }
        }
        os << "\n";

        os << "namespace Generated {\n\n";
        os << "    class " << comp << " : public UIEngine::Component {\n";
        os << "        public:\n";
        os << "            explicit " << comp << "(UIEngine::Props props = {});\n";
        os << "            virtual ~" << comp << "() override = default;\n\n";

        os << "            [[nodiscard]] virtual UIEngine::VNode render() const override;\n";
        os << "            [[nodiscard]] virtual UIEngine::StyleSheet styles() const override;\n";
        os << "            [[nodiscard]] virtual std::string getComponentName() const override { return \""
           << comp << "\"; }\n\n";

        // Methods declared via def in <script>
        if (_ast.script) {
            for (const auto& fn : _ast.script->functions) {
                os << "            " << _mapType(fn->returnType) << " " << fn->name << "(";
                for (size_t i = 0; i < fn->parameters.size(); ++i) {
                    if (i > 0) os << ", ";
                    os << _mapType(fn->parameters[i].typeName) << " " << fn->parameters[i].name;
                }
                os << ") const;\n";
            }
            if (!_ast.script->functions.empty()) os << "\n";
        }

        os << "        protected:\n";
        // Phase 1.1: State variables
        if (_ast.script) {
            for (const auto& st : _ast.script->stateDecls) {
                os << "            mutable UIEngine::State<" << _mapType(st->typeName)
                   << "> " << st->name << ";\n";
            }
            // Let constants / props
            for (const auto& lt : _ast.script->letDecls) {
                os << "            " << _mapType(lt->typeName) << " " << lt->name << ";\n";
            }
        }

        os << "    };\n\n";
        os << "} // namespace Generated\n\n";
        os << "#endif // " << guard << "\n";

        return os.str();
    }

    std::string Codegen::_generateSource() {
        std::ostringstream os;
        std::string comp = _ast.componentName;

        os << "//\n// Automatically generated ahead-of-time by keety 🐱\n// Single-File Component: "
           << comp << "\n//\n\n";
        os << "#include \"" << _headerFileName << "\"\n";
        os << "#include <utility>\n";
        os << "#include <memory>\n\n";

        os << "namespace Generated {\n\n";

        // Phase 2.2: Static String Interning
        os << "    namespace {\n";
        for (const auto& tag : _internedTags) {
            std::string ident = tag;
            std::replace(ident.begin(), ident.end(), '-', '_');
            os << "        static const std::string s_tag_" << ident << " = \"" << tag << "\";\n";
        }
        for (const auto& prop : _internedProps) {
            std::string ident = prop;
            std::replace(ident.begin(), ident.end(), '-', '_');
            std::replace(ident.begin(), ident.end(), ':', '_');
            os << "        static const std::string s_prop_" << ident << " = \"" << prop << "\";\n";
        }
        os << "    }\n\n";

        // Constructor
        os << "    " << comp << "::" << comp << "(UIEngine::Props props)\n";
        os << "        : UIEngine::Component(std::move(props))";

        if (_ast.script) {
            for (const auto& st : _ast.script->stateDecls) {
                std::string init = st->initialValueExpr.empty() ? "{}" : st->initialValueExpr;
                os << ",\n          " << st->name << "(this, \"" << st->name << "\", " << init << ")";
            }
        }
        os << " {\n";

        // Initialize let variables from props or defaults
        if (_ast.script) {
            for (const auto& lt : _ast.script->letDecls) {
                std::string defaultVal = lt->defaultValueExpr.empty() ? "{}" : lt->defaultValueExpr;
                if (lt->typeName == "string") {
                    os << "        " << lt->name << " = getProp(\"" << lt->name << "\", " << defaultVal << ");\n";
                } else if (lt->typeName == "bool") {
                    os << "        " << lt->name << " = getBoolProp(\"" << lt->name << "\", " << defaultVal << ");\n";
                } else {
                    os << "        " << lt->name << " = " << defaultVal << ";\n";
                }
            }
        }
        os << "    }\n\n";

        // Methods
        if (_ast.script) {
            for (const auto& fn : _ast.script->functions) {
                os << "    " << _mapType(fn->returnType) << " " << comp << "::" << fn->name << "(";
                for (size_t i = 0; i < fn->parameters.size(); ++i) {
                    if (i > 0) os << ", ";
                    os << _mapType(fn->parameters[i].typeName) << " " << fn->parameters[i].name;
                }
                os << ") const {\n";
                os << "        " << fn->body << "\n";
                os << "    }\n\n";
            }
        }

        // styles() method implementation
        os << "    UIEngine::StyleSheet " << comp << "::styles() const {\n";
        os << "        UIEngine::StyleSheet sheet;\n";
        if (_ast.styleBlock) {
            for (const auto& rule : _ast.styleBlock->rules) {
                os << "        sheet.rule(\"" << rule->selector << "\", {\n";
                for (size_t i = 0; i < rule->declarations.size(); ++i) {
                    const auto& decl = rule->declarations[i];
                    os << "            {\"" << decl.property << "\", \"" << decl.value << "\"}";
                    if (i + 1 < rule->declarations.size()) os << ",";
                    os << "\n";
                }
                os << "        });\n";
            }
        }
        os << "        return sheet;\n";
        os << "    }\n\n";

        // render() method implementation
        os << "    UIEngine::VNode " << comp << "::render() const {\n";
        os << "        UIEngine::Detail::ComponentScopeGuard _scopeGuard(getComponentKey());\n";
        if (_ast.templateBlock && !_ast.templateBlock->roots.empty()) {
            int nodeCounter = 0;
            // Generate root elements
            if (_ast.templateBlock->roots.size() == 1) {
                const auto& root = _ast.templateBlock->roots[0];
                if (auto rootElem = dynamic_cast<const ElementNode*>(root.get())) {
                    _generateElementEmission(os, *rootElem, "", nodeCounter);
                    os << "        return node0;\n";
                } else {
                    os << "        return UIEngine::VNode::textNode(\"\");\n";
                }
            } else {
                // Wrap in root container if multiple roots exist
                os << "        UIEngine::VNode rootNode = UIEngine::VNode::el(\"div\");\n";
                os << "        rootNode.children.reserve(" << _ast.templateBlock->roots.size() << ");\n";
                for (const auto& root : _ast.templateBlock->roots) {
                    _generateNodeEmission(os, *root, "rootNode.children", nodeCounter);
                }
                os << "        return rootNode;\n";
            }
        } else {
            os << "        return UIEngine::VNode::el(\"div\");\n";
        }
        os << "    }\n\n";

        os << "} // namespace Generated\n";
        return os.str();
    }

    void Codegen::_generateElementEmission(std::ostream& os, const ElementNode& elem, const std::string& parentVecName, int& nodeCounter) {
        int currentId = nodeCounter++;
        std::string varName = "node" + std::to_string(currentId);

        std::string tagIdent = elem.tagName;
        std::replace(tagIdent.begin(), tagIdent.end(), '-', '_');

        os << "        UIEngine::VNode " << varName << " = UIEngine::VNode::el(s_tag_" << tagIdent << ");\n";

        // Attributes, events, and two-way bindings
        for (const auto& attr : elem.attributes) {
            std::string propIdent = attr.name;
            std::replace(propIdent.begin(), propIdent.end(), '-', '_');
            std::replace(propIdent.begin(), propIdent.end(), ':', '_');

            if (attr.kind == AttributeKind::StaticLiteral) {
                os << "        " << varName << ".props[s_prop_" << propIdent << "] = \"" << attr.rawValue << "\";\n";
            } else if (attr.kind == AttributeKind::DynamicExpression) {
                os << "        " << varName << ".props[s_prop_" << propIdent << "] = " << attr.rawValue << ";\n";
            } else if (attr.kind == AttributeKind::TwoWayBinding) {
                std::string targetIdent = attr.bindTargetProperty;
                std::replace(targetIdent.begin(), targetIdent.end(), '-', '_');
                std::string stateVar = attr.bindStateVariable;

                if (attr.bindTargetProperty == "value") {
                    os << "        " << varName << ".props[s_prop_" << targetIdent << "] = std::string(this->" << stateVar << ".get());\n";
                    os << "        " << varName << ".props[s_prop_input] = UIEngine::PropValue([" << stateVar << " = this->" << stateVar << "](const UIEngine::Event& e) {\n";
                    os << "            " << stateVar << ".set(e.value);\n";
                    os << "        });\n";
                } else if (attr.bindTargetProperty == "checked") {
                    os << "        " << varName << ".props[s_prop_" << targetIdent << "] = this->" << stateVar << ".get();\n";
                    os << "        " << varName << ".props[s_prop_change] = UIEngine::PropValue([" << stateVar << " = this->" << stateVar << "](const UIEngine::Event& e) {\n";
                    os << "            " << stateVar << ".set(e.checked);\n";
                    os << "        });\n";
                }
            } else if (attr.kind == AttributeKind::EventCallback) {
                std::string eventName = attr.name;
                if (eventName.rfind("on", 0) == 0 && eventName.size() > 2) {
                    eventName = eventName.substr(2);
                    eventName[0] = static_cast<char>(std::tolower(static_cast<unsigned char>(eventName[0])));
                }
                std::string eventIdent = eventName;
                std::replace(eventIdent.begin(), eventIdent.end(), '-', '_');

                // Direct state mutation or statement
                if (!attr.eventExpr.targetState.empty() && !attr.eventExpr.mutationOp.empty()) {
                    std::string st = attr.eventExpr.targetState;
                    std::string op = attr.eventExpr.mutationOp;
                    std::string val = attr.eventExpr.mutationValue;

                    os << "        " << varName << ".props[s_prop_" << eventIdent << "] = UIEngine::PropValue([" << st << " = this->" << st << "](";
                    if (attr.eventExpr.paramStyle == EventParamStyle::EventArg) {
                        os << "const UIEngine::Event& " << attr.eventExpr.eventArgName;
                    } else {
                        os << "const UIEngine::Event&";
                    }
                    os << ") {\n";
                    if (op == "+=") {
                        os << "            " << st << ".set(" << st << ".get() + (" << val << "));\n";
                    } else if (op == "-=") {
                        os << "            " << st << ".set(" << st << ".get() - (" << val << "));\n";
                    } else if (op == "=") {
                        os << "            " << st << ".set(" << val << ");\n";
                    }
                    os << "        });\n";
                } else if (attr.eventExpr.paramStyle == EventParamStyle::MethodReference) {
                    os << "        " << varName << ".props[s_prop_" << eventIdent << "] = UIEngine::PropValue([self = std::make_shared<" << _ast.componentName << ">(*this)](const UIEngine::Event&) {\n";
                    os << "            self->" << attr.rawValue << "();\n";
                    os << "        });\n";
                } else {
                    os << "        " << varName << ".props[s_prop_" << eventIdent << "] = UIEngine::PropValue([self = std::make_shared<" << _ast.componentName << ">(*this)](";
                    if (attr.eventExpr.paramStyle == EventParamStyle::EventArg) {
                        os << "const UIEngine::Event& " << attr.eventExpr.eventArgName;
                    } else {
                        os << "const UIEngine::Event&";
                    }
                    os << ") {\n";
                    os << "            self->" << attr.eventExpr.rawExpression << ";\n";
                    os << "        });\n";
                }
            }
        }

        // Phase 2.2: Pre-allocate child vector if static child count > 0
        if (!elem.children.empty()) {
            os << "        " << varName << ".children.reserve(" << elem.children.size() << ");\n";
        }

        // Generate child nodes
        for (const auto& child : elem.children) {
            _generateNodeEmission(os, *child, varName + ".children", nodeCounter);
        }

        if (!parentVecName.empty()) {
            os << "        " << parentVecName << ".push_back(std::move(" << varName << "));\n";
        }
    }

    void Codegen::_generateNodeEmission(std::ostream& os, const TemplateNode& node, const std::string& parentVecName, int& nodeCounter) {
        if (auto elem = dynamic_cast<const ElementNode*>(&node)) {
            _generateElementEmission(os, *elem, parentVecName, nodeCounter);
        } else if (auto text = dynamic_cast<const TextNode*>(&node)) {
            // Escape quotes in static text
            std::string escaped = text->text;
            size_t pos = 0;
            while ((pos = escaped.find('"', pos)) != std::string::npos) {
                escaped.replace(pos, 1, "\\\"");
                pos += 2;
            }
            os << "        " << parentVecName << ".push_back(UIEngine::VNode::textNode(\"" << escaped << "\"));\n";
        } else if (auto interp = dynamic_cast<const InterpolationNode*>(&node)) {
            os << "        {\n";
            os << "            std::ostringstream _interpOss;\n";
            os << "            _interpOss << (" << interp->expression << ");\n";
            os << "            " << parentVecName << ".push_back(UIEngine::VNode::textNode(_interpOss.str()));\n";
            os << "        }\n";
        } else if (auto ifNode = dynamic_cast<const IfBlockNode*>(&node)) {
            for (size_t i = 0; i < ifNode->branches.size(); ++i) {
                const auto& branch = ifNode->branches[i];
                if (i == 0) {
                    os << "        if (" << branch.conditionExpr << ") {\n";
                } else if (!branch.conditionExpr.empty()) {
                    os << "        } else if (" << branch.conditionExpr << ") {\n";
                } else {
                    os << "        } else {\n";
                }
                for (const auto& child : branch.children) {
                    _generateNodeEmission(os, *child, parentVecName, nodeCounter);
                }
            }
            os << "        }\n";
        } else if (auto forNode = dynamic_cast<const ForBlockNode*>(&node)) {
            os << "        for (const auto& " << forNode->itemIdentifier << " : " << forNode->collectionExpr << ") {\n";
            for (const auto& child : forNode->children) {
                _generateNodeEmission(os, *child, parentVecName, nodeCounter);
            }
            os << "        }\n";
        }
    }

} // namespace Keety
