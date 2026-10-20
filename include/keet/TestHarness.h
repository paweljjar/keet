//
// Created by PAWEL on 20/10/2026.
//

#ifndef LORIKEET_TESTHARNESS_H
#define LORIKEET_TESTHARNESS_H

#include <keet/UIEngine.h>

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace keet::Testing {

    class TestElement;
    class MountedComponent;

    namespace Detail {
        struct MountedComponentImpl;
    }

    // Represents a DOM/VNode element within a mounted test component tree
    class TestElement {
        public:
            TestElement();
            TestElement(std::shared_ptr<Detail::MountedComponentImpl> mountedImpl,
                        UIEngine::VNode node,
                        std::string selector = "",
                        bool exists = true);

            [[nodiscard]] bool exists() const noexcept { return _exists; }
            explicit operator bool() const noexcept { return exists(); }

            [[nodiscard]] const std::string& tag() const;
            [[nodiscard]] std::string text() const;
            [[nodiscard]] std::string html() const;
            [[nodiscard]] std::string attr(const std::string& name) const;
            [[nodiscard]] bool hasAttr(const std::string& name) const;
            [[nodiscard]] bool hasClass(const std::string& className) const;
            [[nodiscard]] bool propBool(const std::string& name) const;
            [[nodiscard]] const UIEngine::Props& props() const;
            [[nodiscard]] const UIEngine::VNode& vnode() const;

            [[nodiscard]] std::vector<TestElement> children() const;
            [[nodiscard]] TestElement find(const std::string& selector) const;
            [[nodiscard]] std::vector<TestElement> findAll(const std::string& selector) const;

            // Event dispatchers that execute bound C++ event handlers and trigger synchronous re-render
            void trigger(const std::string& eventType, const UIEngine::Event& event = {});
            void triggerClick();
            void triggerInput(const std::string& value);
            void triggerChange(const std::string& value);
            void triggerSubmit(const std::map<std::string, std::string>& form = {});
            void triggerKeyDown(const std::string& key);

        private:
            void _checkExists() const;

            std::shared_ptr<Detail::MountedComponentImpl> _mountedImpl;
            UIEngine::VNode _node;
            bool _exists = false;
            std::string _selector;
    };

    // Represents a mounted component tree in an isolated test Session
    class MountedComponent {
        public:
            MountedComponent(std::shared_ptr<UIEngine::Session> session,
                             std::function<UIEngine::VNode()> factory);
            ~MountedComponent();

            [[nodiscard]] const UIEngine::VNode& vnode() const;
            [[nodiscard]] const HTTPServer::Json& json() const;
            [[nodiscard]] std::string text() const;
            [[nodiscard]] std::string html() const;
            [[nodiscard]] std::shared_ptr<UIEngine::Session> session() const;

            void navigate(const std::string& path);
            void setLocale(const std::string& locale);
            void rerender();

            [[nodiscard]] TestElement find(const std::string& selector) const;
            [[nodiscard]] std::vector<TestElement> findAll(const std::string& selector) const;
            [[nodiscard]] bool contains(const std::string& selector) const;

        private:
            std::shared_ptr<Detail::MountedComponentImpl> _impl;
            friend class TestElement;
    };

    // Headless testing harness entry point for mounting components and VNode hierarchies
    class TestHarness {
        public:
            template <typename CompT, typename... Args>
            requires std::derived_from<CompT, UIEngine::Component>
            static MountedComponent mount(Args&&... args) {
                auto factory = [argsTuple = std::make_tuple(std::forward<Args>(args)...)]() mutable {
                    auto comp = std::apply([](auto&&... unpacked) {
                        return CompT(std::forward<decltype(unpacked)>(unpacked)...);
                    }, argsTuple);
                    return UIEngine::renderComponent(std::move(comp));
                };
                return mount(std::move(factory));
            }

            static MountedComponent mount(std::function<UIEngine::VNode()> factory,
                                          const std::string& initialPath = "/",
                                          const std::string& initialLocale = "en");
    };

} // namespace keet::Testing

#endif // LORIKEET_TESTHARNESS_H
