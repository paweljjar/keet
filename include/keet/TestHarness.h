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

    [[noreturn]] void fail(const std::string& message, const char* file, int line);

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
                        bool exists = true,
                        std::vector<size_t> path = {});

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
            [[nodiscard]] const std::vector<size_t>& path() const noexcept { return _path; }

            [[nodiscard]] std::vector<TestElement> children() const;
            [[nodiscard]] TestElement find(const std::string& selector) const;
            [[nodiscard]] std::vector<TestElement> findAll(const std::string& selector) const;

            // Event dispatchers that execute bound C++ event handlers and trigger synchronous re-render
            void trigger(const std::string& eventType, const UIEngine::Event& event = {}, bool bubble = true);
            void triggerClick(double clientX = 0.0, double clientY = 0.0);
            void triggerDoubleClick();
            void triggerMouseDown();
            void triggerMouseUp();
            void triggerInput(const std::string& value);
            void triggerChange(const std::string& value);
            void triggerCheck(bool checked = true);
            void triggerSubmit(const std::map<std::string, std::string>& form = {});
            void triggerFocus();
            void triggerBlur();
            void triggerKeyDown(const std::string& key,
                                const std::string& code = "",
                                bool shiftKey = false,
                                bool ctrlKey = false,
                                bool altKey = false,
                                bool metaKey = false);
            void triggerKeyUp(const std::string& key,
                              const std::string& code = "",
                              bool shiftKey = false,
                              bool ctrlKey = false,
                              bool altKey = false,
                              bool metaKey = false);

        private:
            void _checkExists() const;

            std::shared_ptr<Detail::MountedComponentImpl> _mountedImpl;
            UIEngine::VNode _node;
            bool _exists = false;
            std::string _selector;
            std::vector<size_t> _path;
    };

    // Represents configuration options for mounting test components
    struct MountOptions {
        std::string path = "/";
        std::string locale = "en";
        std::map<std::string, std::any> initialState;
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
            [[nodiscard]] std::string sessionId() const;

            // Session state inspection and mutation helpers
            template <typename T>
            void setState(const std::string& key, T value);

            template <typename T>
            [[nodiscard]] T getState(const std::string& key) const;

            [[nodiscard]] bool hasState(const std::string& key) const;

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
            static MountedComponent mount(const MountOptions& options, Args&&... args) {
                auto factory = [argsTuple = std::make_tuple(std::forward<Args>(args)...)]() mutable {
                    auto comp = std::apply([](auto&&... unpacked) {
                        return CompT(std::forward<decltype(unpacked)>(unpacked)...);
                    }, argsTuple);
                    return UIEngine::renderComponent(std::move(comp));
                };
                return mount(std::move(factory), options);
            }

            template <typename CompT>
            requires std::derived_from<CompT, UIEngine::Component>
            static MountedComponent mount() {
                return mount<CompT>(MountOptions{});
            }

            template <typename CompT, typename First, typename... Rest>
            requires (std::derived_from<CompT, UIEngine::Component> &&
                      !std::is_same_v<std::decay_t<First>, MountOptions>)
            static MountedComponent mount(First&& first, Rest&&... rest) {
                return mount<CompT>(MountOptions{}, std::forward<First>(first), std::forward<Rest>(rest)...);
            }

            static MountedComponent mount(std::function<UIEngine::VNode()> factory,
                                          const MountOptions& options = {});

            static MountedComponent mount(std::function<UIEngine::VNode()> factory,
                                          const std::string& initialPath,
                                          const std::string& initialLocale = "en") {
                return mount(std::move(factory), MountOptions{.path = initialPath, .locale = initialLocale});
            }
    };

    // Snapshot testing utilities for VNode hierarchies
    namespace Snapshot {
        [[nodiscard]] std::string formatVNode(const UIEngine::VNode& node, int indent = 0);

        void assertMatchesSnapshot(const std::string& actualHtml,
                                   const std::string& snapshotName,
                                   const std::string& snapshotDir,
                                   const char* file,
                                   int line);

        void assertMatchesSnapshot(const MountedComponent& mounted,
                                   const std::string& snapshotName,
                                   const std::string& snapshotDir,
                                   const char* file,
                                   int line);

        void assertMatchesSnapshot(const TestElement& element,
                                   const std::string& snapshotName,
                                   const std::string& snapshotDir,
                                   const char* file,
                                   int line);
    } // namespace Snapshot

    template <typename T>
    void MountedComponent::setState(const std::string& key, T value) {
        auto sess = session();
        if (!sess) fail("MountedComponent has no active session", __FILE__, __LINE__);
        std::lock_guard lock(sess->mutex);
        sess->stateStore[key] = std::any(value);
        std::string suffix = "." + key;
        for (auto& [k, v] : sess->stateStore) {
            if (k.ends_with(suffix)) {
                v = std::any(value);
            }
        }
        rerender();
    }

    template <typename T>
    T MountedComponent::getState(const std::string& key) const {
        auto sess = session();
        if (!sess) fail("MountedComponent has no active session", __FILE__, __LINE__);
        std::lock_guard lock(sess->mutex);
        auto it = sess->stateStore.find(key);
        if (it != sess->stateStore.end()) {
            return std::any_cast<T>(it->second);
        }
        std::string suffix = "." + key;
        for (const auto& [k, v] : sess->stateStore) {
            if (k.ends_with(suffix)) {
                return std::any_cast<T>(v);
            }
        }
        fail("State key not found in session: \"" + key + "\"", __FILE__, __LINE__);
    }

    inline bool MountedComponent::hasState(const std::string& key) const {
        auto sess = session();
        if (!sess) return false;
        std::lock_guard lock(sess->mutex);
        if (sess->stateStore.contains(key)) return true;
        std::string suffix = "." + key;
        for (const auto& [k, v] : sess->stateStore) {
            if (k.ends_with(suffix)) return true;
        }
        return false;
    }

} // namespace keet::Testing

#endif // LORIKEET_TESTHARNESS_H
