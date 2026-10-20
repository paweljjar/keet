//
// Created by PAWEL on 20/10/2026.
//

#include <keet/TestHarness.h>
#include <keet/Testing.h>

#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>

namespace keet::Testing {

    namespace {
        size_t nextTestSessionNumber = 0;

        std::string trim(std::string_view s) {
            while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
                s.remove_prefix(1);
            }
            while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
                s.remove_suffix(1);
            }
            return std::string(s);
        }

        std::string toLower(std::string_view s) {
            std::string out;
            out.reserve(s.size());
            for (char c : s) {
                out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
            }
            return out;
        }

        std::string extractMergedClass(const UIEngine::VNode& node) {
            std::vector<std::string> classes;
            std::set<std::string> seen;

            auto addTokens = [&](const std::string& str) {
                std::istringstream iss(str);
                std::string token;
                while (iss >> token) {
                    if (!seen.contains(token)) {
                        seen.insert(token);
                        classes.push_back(std::move(token));
                    }
                }
            };

            if (auto it = node.props.find("class"); it != node.props.end() && it->second.isString()) {
                addTokens(it->second.asString());
            }
            if (auto it = node.props.find("className"); it != node.props.end() && it->second.isString()) {
                addTokens(it->second.asString());
            }

            for (const auto& [k, v] : node.props) {
                if (k.starts_with("class:")) {
                    std::string cls = k.substr(6);
                    bool isTruthy = false;
                    if (v.isBool()) isTruthy = v.asBool();
                    else if (v.isString()) {
                        const auto& s = v.asString();
                        isTruthy = !s.empty() && s != "false" && s != "0";
                    }
                    if (isTruthy && !seen.contains(cls)) {
                        seen.insert(cls);
                        classes.push_back(std::move(cls));
                    }
                }
            }

            std::string result;
            for (size_t i = 0; i < classes.size(); ++i) {
                if (i > 0) result += " ";
                result += classes[i];
            }
            return result;
        }

        bool nodeHasClass(const UIEngine::VNode& node, const std::string& className) {
            auto checkString = [&](const std::string& classStr) {
                std::istringstream iss(classStr);
                std::string token;
                while (iss >> token) {
                    if (token == className) return true;
                }
                return false;
            };

            if (auto it = node.props.find("class"); it != node.props.end() && it->second.isString()) {
                if (checkString(it->second.asString())) return true;
            }
            if (auto it = node.props.find("className"); it != node.props.end() && it->second.isString()) {
                if (checkString(it->second.asString())) return true;
            }
            if (auto it = node.props.find("class:" + className); it != node.props.end()) {
                if (it->second.isBool() && it->second.asBool()) return true;
                if (it->second.isString()) {
                    const auto& s = it->second.asString();
                    if (!s.empty() && s != "false" && s != "0") return true;
                }
            }
            return false;
        }

        std::string nodeAttr(const UIEngine::VNode& node, const std::string& name) {
            if (name == "class" || name == "className") {
                return extractMergedClass(node);
            }
            auto it = node.props.find(name);
            if (it != node.props.end()) {
                if (it->second.isString()) return it->second.asString();
                if (it->second.isBool()) return it->second.asBool() ? "true" : "false";
            }
            return "";
        }

        bool nodeHasAttr(const UIEngine::VNode& node, const std::string& name) {
            if (name == "class" || name == "className") {
                return !extractMergedClass(node).empty();
            }
            auto it = node.props.find(name);
            if (it != node.props.end()) {
                if (it->second.isBool()) return it->second.asBool();
                return true;
            }
            return false;
        }

        std::string extractText(const UIEngine::VNode& node) {
            if (node.tag == "#text") {
                return node.text;
            }
            std::string result;
            for (const auto& child : node.children) {
                result += extractText(child);
            }
            return result;
        }

        std::string extractHtml(const UIEngine::VNode& node) {
            if (node.tag == "#text") {
                return node.text;
            }
            std::string html = "<" + node.tag;

            // id attribute
            if (auto it = node.props.find("id"); it != node.props.end() && it->second.isString()) {
                html += " id=\"" + it->second.asString() + "\"";
            }

            // class attribute
            std::string cls = extractMergedClass(node);
            if (!cls.empty()) {
                html += " class=\"" + cls + "\"";
            }

            // other attributes
            for (const auto& [k, v] : node.props) {
                if (k == "id" || k == "class" || k == "className" || k.starts_with("class:") || k == "key") {
                    continue;
                }
                if (v.isString()) {
                    html += " " + k + "=\"" + v.asString() + "\"";
                } else if (v.isBool()) {
                    if (v.asBool()) html += " " + k;
                } else if (v.isHandler()) {
                    html += " data-on-" + k;
                }
            }

            static const std::set<std::string> voidTags = {
                "area", "base", "br", "col", "embed", "hr", "img", "input",
                "link", "meta", "param", "source", "track", "wbr"
            };

            if (voidTags.contains(node.tag) && node.children.empty()) {
                html += " />";
                return html;
            }

            html += ">";
            for (const auto& child : node.children) {
                html += extractHtml(child);
            }
            html += "</" + node.tag + ">";
            return html;
        }

        // --- CSS Selector Grammar and Matching Engine ---
        struct AttrCondition {
            std::string name;
            std::string op; // "=", "!=", "^=", "$=", "*=", or "" (presence)
            std::string value;
        };

        struct CompoundSelector {
            std::string tag;
            std::string id;
            std::vector<std::string> classes;
            std::vector<AttrCondition> attributes;
            std::vector<std::string> pseudos; // ":first-child", ":last-child", ":disabled"
        };

        enum class Combinator {
            None,
            Descendant, // ' '
            Child       // '>'
        };

        struct SelectorBranch {
            std::vector<CompoundSelector> compounds;
            std::vector<Combinator> combinators; // combinators[i] links compounds[i] to compounds[i+1]
        };

        struct MatchContextNode {
            const UIEngine::VNode* node = nullptr;
            const MatchContextNode* parent = nullptr;
            size_t childIndex = 0;
            size_t totalSiblings = 0;
        };

        std::string unquote(const std::string& s) {
            if (s.size() >= 2 && ((s.front() == '"' && s.back() == '"') || (s.front() == '\'' && s.back() == '\''))) {
                return s.substr(1, s.size() - 2);
            }
            return s;
        }

        CompoundSelector parseCompound(std::string_view raw) {
            CompoundSelector comp;
            size_t i = 0;
            const size_t n = raw.size();

            // Tag name (if present at the start)
            if (i < n && (std::isalnum(static_cast<unsigned char>(raw[i])) || raw[i] == '*')) {
                size_t tagStart = i;
                while (i < n && (std::isalnum(static_cast<unsigned char>(raw[i])) || raw[i] == '-' || raw[i] == '_')) {
                    ++i;
                }
                comp.tag = std::string(raw.substr(tagStart, i - tagStart));
            }

            while (i < n) {
                if (raw[i] == '#') {
                    ++i;
                    size_t start = i;
                    while (i < n && (std::isalnum(static_cast<unsigned char>(raw[i])) || raw[i] == '-' || raw[i] == '_')) {
                        ++i;
                    }
                    comp.id = std::string(raw.substr(start, i - start));
                } else if (raw[i] == '.') {
                    ++i;
                    size_t start = i;
                    while (i < n && (std::isalnum(static_cast<unsigned char>(raw[i])) || raw[i] == '-' || raw[i] == '_')) {
                        ++i;
                    }
                    comp.classes.push_back(std::string(raw.substr(start, i - start)));
                } else if (raw[i] == '[') {
                    ++i;
                    size_t closePos = raw.find(']', i);
                    if (closePos == std::string_view::npos) break;
                    std::string attrExpr = std::string(raw.substr(i, closePos - i));
                    i = closePos + 1;

                    AttrCondition cond;
                    static const std::vector<std::string> ops = {"!=", "^=", "$=", "*=", "="};
                    bool foundOp = false;
                    for (const auto& op : ops) {
                        size_t pos = attrExpr.find(op);
                        if (pos != std::string::npos) {
                            cond.name = trim(attrExpr.substr(0, pos));
                            cond.op = op;
                            cond.value = unquote(trim(attrExpr.substr(pos + op.size())));
                            foundOp = true;
                            break;
                        }
                    }
                    if (!foundOp) {
                        cond.name = trim(attrExpr);
                        cond.op = "";
                        cond.value = "";
                    }
                    comp.attributes.push_back(std::move(cond));
                } else if (raw[i] == ':') {
                    ++i;
                    size_t start = i;
                    while (i < n && (std::isalnum(static_cast<unsigned char>(raw[i])) || raw[i] == '-')) {
                        ++i;
                    }
                    comp.pseudos.push_back(":" + std::string(raw.substr(start, i - start)));
                } else {
                    ++i;
                }
            }

            return comp;
        }

        std::vector<SelectorBranch> parseSelector(const std::string& selectorStr) {
            std::vector<SelectorBranch> branches;
            std::stringstream ss(selectorStr);
            std::string branchStr;

            while (std::getline(ss, branchStr, ',')) {
                branchStr = trim(branchStr);
                if (branchStr.empty()) continue;

                SelectorBranch branch;
                size_t i = 0;
                const size_t n = branchStr.size();

                while (i < n) {
                    while (i < n && std::isspace(static_cast<unsigned char>(branchStr[i]))) {
                        ++i;
                    }
                    if (i >= n) break;

                    if (branchStr[i] == '>') {
                        branch.combinators.push_back(Combinator::Child);
                        ++i;
                        continue;
                    }

                    // Parse compound token
                    size_t tokenStart = i;
                    bool inBracket = false;
                    while (i < n) {
                        if (branchStr[i] == '[') inBracket = true;
                        else if (branchStr[i] == ']') inBracket = false;
                        else if (!inBracket && (std::isspace(static_cast<unsigned char>(branchStr[i])) || branchStr[i] == '>')) {
                            break;
                        }
                        ++i;
                    }

                    std::string compoundStr = branchStr.substr(tokenStart, i - tokenStart);
                    if (!compoundStr.empty()) {
                        branch.compounds.push_back(parseCompound(compoundStr));
                    }

                    // Check for trailing combinator
                    size_t spaces = 0;
                    while (i < n && std::isspace(static_cast<unsigned char>(branchStr[i]))) {
                        ++spaces;
                        ++i;
                    }
                    if (i < n && branchStr[i] == '>') {
                        branch.combinators.push_back(Combinator::Child);
                        ++i;
                    } else if (spaces > 0 && i < n) {
                        branch.combinators.push_back(Combinator::Descendant);
                    }
                }

                if (!branch.compounds.empty()) {
                    branches.push_back(std::move(branch));
                }
            }

            return branches;
        }

        bool matchCompound(const MatchContextNode& ctx, const CompoundSelector& sel) {
            const auto& node = *ctx.node;
            if (node.tag == "#text") return false;

            // Tag match
            if (!sel.tag.empty() && sel.tag != "*") {
                if (toLower(node.tag) != toLower(sel.tag)) return false;
            }

            // ID match
            if (!sel.id.empty()) {
                if (nodeAttr(node, "id") != sel.id) return false;
            }

            // Class matches
            for (const auto& cls : sel.classes) {
                if (!nodeHasClass(node, cls)) return false;
            }

            // Attribute matches
            for (const auto& attr : sel.attributes) {
                if (attr.op.empty()) {
                    if (!nodeHasAttr(node, attr.name)) return false;
                } else if (attr.op == "=") {
                    if (nodeAttr(node, attr.name) != attr.value) return false;
                } else if (attr.op == "!=") {
                    if (nodeAttr(node, attr.name) == attr.value) return false;
                } else if (attr.op == "^=") {
                    if (!nodeAttr(node, attr.name).starts_with(attr.value)) return false;
                } else if (attr.op == "$=") {
                    if (!nodeAttr(node, attr.name).ends_with(attr.value)) return false;
                } else if (attr.op == "*=") {
                    if (nodeAttr(node, attr.name).find(attr.value) == std::string::npos) return false;
                }
            }

            // Pseudo matches
            for (const auto& pseudo : sel.pseudos) {
                if (pseudo == ":first-child") {
                    if (ctx.childIndex != 0) return false;
                } else if (pseudo == ":last-child") {
                    if (ctx.totalSiblings == 0 || ctx.childIndex + 1 != ctx.totalSiblings) return false;
                } else if (pseudo == ":disabled") {
                    if (!nodeHasAttr(node, "disabled") || nodeAttr(node, "disabled") == "false") return false;
                }
            }

            return true;
        }

        bool matchBranch(const MatchContextNode* ctx, const SelectorBranch& branch) {
            if (!ctx || branch.compounds.empty()) return false;

            int compIdx = static_cast<int>(branch.compounds.size()) - 1;
            if (!matchCompound(*ctx, branch.compounds[compIdx])) {
                return false;
            }

            const MatchContextNode* curr = ctx->parent;
            --compIdx;

            while (compIdx >= 0 && curr != nullptr) {
                Combinator comb = branch.combinators[compIdx];
                if (comb == Combinator::Child) {
                    if (!matchCompound(*curr, branch.compounds[compIdx])) {
                        return false;
                    }
                    curr = curr->parent;
                    --compIdx;
                } else { // Combinator::Descendant
                    while (curr != nullptr && !matchCompound(*curr, branch.compounds[compIdx])) {
                        curr = curr->parent;
                    }
                    if (curr == nullptr) return false;
                    curr = curr->parent;
                    --compIdx;
                }
            }

            return compIdx < 0;
        }

        bool matchNode(const MatchContextNode* ctx, const std::vector<SelectorBranch>& branches) {
            for (const auto& branch : branches) {
                if (matchBranch(ctx, branch)) return true;
            }
            return false;
        }

        void collectMatchingNodes(const MatchContextNode* ctx,
                                  const std::vector<SelectorBranch>& branches,
                                  std::vector<const UIEngine::VNode*>& out) {
            if (ctx->node->tag != "#text" && matchNode(ctx, branches)) {
                out.push_back(ctx->node);
            }

            // Filter child element nodes for sibling calculation
            std::vector<const UIEngine::VNode*> elementChildren;
            for (const auto& child : ctx->node->children) {
                if (child.tag != "#text") {
                    elementChildren.push_back(&child);
                }
            }

            size_t childElemIdx = 0;
            for (const auto& child : ctx->node->children) {
                if (child.tag == "#text") continue;
                MatchContextNode childCtx{
                    .node = &child,
                    .parent = ctx,
                    .childIndex = childElemIdx++,
                    .totalSiblings = elementChildren.size()
                };
                collectMatchingNodes(&childCtx, branches, out);
            }
        }
    } // namespace

    namespace Detail {
        struct MountedComponentImpl : public std::enable_shared_from_this<MountedComponentImpl> {
            std::shared_ptr<UIEngine::Session> session;
            std::function<UIEngine::VNode()> factory;
            UIEngine::VNode rootNode;
            HTTPServer::Json rootJson;
            std::string currentPath = "/";
            std::string currentLocale = "en";

            void rerender() {
                UIEngine::SessionContext ctx(session);
                session->currentPath = currentPath;
                session->locale = currentLocale;

                UIEngine::Detail::resetRenderContext();
                {
                    std::lock_guard lock(session->mutex);
                    session->handlerRegistry.clear();
                    session->nextHandlerId = 0;
                }

                rootNode = factory();

                UIEngine::StyleSheet treeStyles;
                UIEngine::Detail::harvestStylesFromTree(rootNode, treeStyles);
                if (!treeStyles.empty()) {
                    UIEngine::Detail::registerTreeStyles(treeStyles);
                }

                if (UIEngine::Detail::reconcileUnmountedComponents()) {
                    UIEngine::Detail::resetRenderContext();
                    {
                        std::lock_guard lock(session->mutex);
                        session->handlerRegistry.clear();
                        session->nextHandlerId = 0;
                    }
                    rootNode = factory();
                    UIEngine::StyleSheet recStyles;
                    UIEngine::Detail::harvestStylesFromTree(rootNode, recStyles);
                    if (!recStyles.empty()) {
                        UIEngine::Detail::registerTreeStyles(recStyles);
                    }
                    UIEngine::Detail::reconcileUnmountedComponents();
                }

                rootJson = rootNode.toJson();
            }

            TestElement find(const std::string& selector) {
                auto all = findAll(selector);
                if (!all.empty()) {
                    return all.front();
                }
                return TestElement(nullptr, UIEngine::VNode(), selector);
            }

            std::vector<TestElement> findAll(const std::string& selector) {
                std::vector<TestElement> results;
                auto branches = parseSelector(selector);
                if (branches.empty()) return results;

                std::vector<const UIEngine::VNode*> matches;
                MatchContextNode rootCtx{
                    .node = &rootNode,
                    .parent = nullptr,
                    .childIndex = 0,
                    .totalSiblings = 1
                };
                collectMatchingNodes(&rootCtx, branches, matches);

                for (const auto* m : matches) {
                    results.emplace_back(shared_from_this(), *m, selector);
                }
                return results;
            }
        };
    } // namespace Detail

    // --- TestElement Implementation ---
    TestElement::TestElement()
        : _mountedImpl(nullptr), _node(), _exists(false), _selector("") {}

    TestElement::TestElement(std::shared_ptr<Detail::MountedComponentImpl> mountedImpl,
                             UIEngine::VNode node,
                             std::string selector,
                             bool exists)
        : _mountedImpl(std::move(mountedImpl)),
          _node(std::move(node)),
          _exists(exists && _mountedImpl != nullptr),
          _selector(std::move(selector)) {}

    void TestElement::_checkExists() const {
        if (!_exists) {
            fail("Attempted operation on non-existent TestElement for selector: \"" + _selector + "\"", __FILE__, __LINE__);
        }
    }

    const std::string& TestElement::tag() const {
        _checkExists();
        return _node.tag;
    }

    std::string TestElement::text() const {
        _checkExists();
        return extractText(_node);
    }

    std::string TestElement::html() const {
        _checkExists();
        return extractHtml(_node);
    }

    std::string TestElement::attr(const std::string& name) const {
        _checkExists();
        return nodeAttr(_node, name);
    }

    bool TestElement::hasAttr(const std::string& name) const {
        _checkExists();
        return nodeHasAttr(_node, name);
    }

    bool TestElement::hasClass(const std::string& className) const {
        _checkExists();
        return nodeHasClass(_node, className);
    }

    bool TestElement::propBool(const std::string& name) const {
        _checkExists();
        auto it = _node.props.find(name);
        if (it != _node.props.end()) {
            if (it->second.isBool()) return it->second.asBool();
            if (it->second.isString()) return it->second.asString() == "true";
        }
        return false;
    }

    const UIEngine::Props& TestElement::props() const {
        _checkExists();
        return _node.props;
    }

    const UIEngine::VNode& TestElement::vnode() const {
        _checkExists();
        return _node;
    }

    std::vector<TestElement> TestElement::children() const {
        _checkExists();
        std::vector<TestElement> out;
        for (const auto& child : _node.children) {
            if (child.tag != "#text") {
                out.emplace_back(_mountedImpl, child, "");
            }
        }
        return out;
    }

    TestElement TestElement::find(const std::string& selector) const {
        _checkExists();
        auto all = findAll(selector);
        if (!all.empty()) {
            return all.front();
        }
        return TestElement(nullptr, UIEngine::VNode(), selector);
    }

    std::vector<TestElement> TestElement::findAll(const std::string& selector) const {
        _checkExists();
        std::vector<TestElement> results;
        auto branches = parseSelector(selector);
        if (branches.empty()) return results;

        std::vector<const UIEngine::VNode*> matches;
        MatchContextNode rootCtx{
            .node = &_node,
            .parent = nullptr,
            .childIndex = 0,
            .totalSiblings = 1
        };

        // Search descendant elements of this node
        std::vector<const UIEngine::VNode*> elementChildren;
        for (const auto& child : _node.children) {
            if (child.tag != "#text") elementChildren.push_back(&child);
        }
        size_t childElemIdx = 0;
        for (const auto& child : _node.children) {
            if (child.tag == "#text") continue;
            MatchContextNode childCtx{
                .node = &child,
                .parent = &rootCtx,
                .childIndex = childElemIdx++,
                .totalSiblings = elementChildren.size()
            };
            collectMatchingNodes(&childCtx, branches, matches);
        }

        for (const auto* m : matches) {
            results.emplace_back(_mountedImpl, *m, selector);
        }
        return results;
    }

    void TestElement::trigger(const std::string& eventType, const UIEngine::Event& event) {
        _checkExists();
        if (!_mountedImpl) {
            fail("Cannot trigger event: TestElement is not connected to a MountedComponent", __FILE__, __LINE__);
        }

        UIEngine::EventHandler handler = nullptr;

        std::string pascalType = eventType;
        if (!pascalType.empty()) {
            pascalType[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(pascalType[0])));
        }

        std::vector<std::string> candidateKeys = {
            "on" + pascalType,
            "on" + eventType,
            eventType,
            "data-on-" + eventType
        };

        for (const auto& key : candidateKeys) {
            auto it = _node.props.find(key);
            if (it != _node.props.end()) {
                if (it->second.isHandler()) {
                    handler = it->second.asHandler();
                    break;
                } else if (it->second.isString()) {
                    const std::string& hId = it->second.asString();
                    if (_mountedImpl->session) {
                        std::lock_guard lock(_mountedImpl->session->mutex);
                        auto hit = _mountedImpl->session->handlerRegistry.find(hId);
                        if (hit != _mountedImpl->session->handlerRegistry.end()) {
                            handler = hit->second;
                            break;
                        }
                    }
                }
            }
        }

        if (!handler) {
            fail("No event handler found for event type \"" + eventType + "\" on element <" + _node.tag + ">", __FILE__, __LINE__);
        }

        {
            UIEngine::SessionContext ctx(_mountedImpl->session);
            handler(event);
        }

        _mountedImpl->rerender();

        if (!_selector.empty()) {
            auto updated = _mountedImpl->find(_selector);
            if (updated.exists()) {
                _node = updated.vnode();
            }
        }
    }

    void TestElement::triggerClick() {
        trigger("click", UIEngine::Event{.type = "click"});
    }

    void TestElement::triggerInput(const std::string& value) {
        trigger("input", UIEngine::Event{.type = "input", .value = value});
    }

    void TestElement::triggerChange(const std::string& value) {
        trigger("change", UIEngine::Event{.type = "change", .value = value});
    }

    void TestElement::triggerSubmit(const std::map<std::string, std::string>& form) {
        trigger("submit", UIEngine::Event{.type = "submit", .form = form});
    }

    void TestElement::triggerKeyDown(const std::string& key) {
        trigger("keydown", UIEngine::Event{.type = "keydown", .key = key});
    }

    // --- MountedComponent Implementation ---
    MountedComponent::MountedComponent(std::shared_ptr<UIEngine::Session> session,
                                       std::function<UIEngine::VNode()> factory)
        : _impl(std::make_shared<Detail::MountedComponentImpl>()) {
        _impl->session = std::move(session);
        _impl->factory = std::move(factory);
        _impl->rerender();
    }

    MountedComponent::~MountedComponent() {
        if (_impl && _impl->session) {
            UIEngine::SessionManager::instance().removeSession(_impl->session->id);
        }
    }

    const UIEngine::VNode& MountedComponent::vnode() const {
        return _impl->rootNode;
    }

    const HTTPServer::Json& MountedComponent::json() const {
        return _impl->rootJson;
    }

    std::string MountedComponent::text() const {
        return extractText(_impl->rootNode);
    }

    std::string MountedComponent::html() const {
        return extractHtml(_impl->rootNode);
    }

    std::shared_ptr<UIEngine::Session> MountedComponent::session() const {
        return _impl->session;
    }

    void MountedComponent::navigate(const std::string& path) {
        _impl->currentPath = path;
        _impl->rerender();
    }

    void MountedComponent::setLocale(const std::string& locale) {
        _impl->currentLocale = locale;
        _impl->rerender();
    }

    void MountedComponent::rerender() {
        _impl->rerender();
    }

    TestElement MountedComponent::find(const std::string& selector) const {
        return _impl->find(selector);
    }

    std::vector<TestElement> MountedComponent::findAll(const std::string& selector) const {
        return _impl->findAll(selector);
    }

    bool MountedComponent::contains(const std::string& selector) const {
        return find(selector).exists();
    }

    // --- TestHarness Implementation ---
    MountedComponent TestHarness::mount(std::function<UIEngine::VNode()> factory,
                                        const std::string& initialPath,
                                        const std::string& initialLocale) {
        const std::string sessId = "keet_test_session_" + std::to_string(++nextTestSessionNumber);
        auto session = UIEngine::SessionManager::instance().getOrCreateSession(sessId);
        session->currentPath = initialPath;
        session->locale = initialLocale;

        MountedComponent mc(session, std::move(factory));
        return mc;
    }

} // namespace keet::Testing
