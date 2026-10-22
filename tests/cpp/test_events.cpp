//
// Created by PAWEL on 20/10/2026.
//

#include <keet/Testing.h>
#include <keet/UIEngine.h>

using namespace UIEngine;
using namespace UIEngine::Tags;

class BubblingComponent : public Component {
    public:
        [[nodiscard]] std::string getComponentName() const override { return "BubblingComponent"; }

        [[nodiscard]] VNode render() const override {
            auto clickCount = useState<int>("clicks", 0);
            auto lastTarget = useState<std::string>("target", "none");

            return div({{"id", "wrapper"}}, {
                p({{"id", "click-summary"}}, "Clicks: " + std::to_string(clickCount.get())),
                p({{"id", "last-target"}}, "Target: " + lastTarget.get()),
                VNode::el("button", {
                    {"id", "parent-btn"},
                    {"onClick", [clickCount, lastTarget] {
                        clickCount.set(clickCount.get() + 1);
                        lastTarget.set("button");
                    }}
                }, {
                    span({{"class", "icon"}}, { VNode::textNode("⭐") }),
                    span({{"class", "btn-text"}}, { VNode::textNode("Star Project") })
                })
            });
        }
};

class FormComponent : public Component {
    public:
        [[nodiscard]] std::string getComponentName() const override { return "FormComponent"; }

        [[nodiscard]] VNode render() const override {
            auto submittedUser = useState<std::string>("sub_user", "");
            auto submittedBio = useState<std::string>("sub_bio", "");
            auto submittedRemember = useState<std::string>("sub_remember", "false");

            return div({}, {
                p({{"id", "status"}}, submittedUser.get().empty() ? "Unsubmitted" : "Submitted: " + submittedUser.get()),
                p({{"id", "bio-status"}}, submittedBio.get()),
                p({{"id", "remember-status"}}, submittedRemember.get()),
                form({
                    {"id", "login-form"},
                    {"onSubmit", [submittedUser, submittedBio, submittedRemember](const Event& ev) {
                        if (auto it = ev.form.find("username"); it != ev.form.end()) {
                            submittedUser.set(it->second);
                        }
                        if (auto it = ev.form.find("bio"); it != ev.form.end()) {
                            submittedBio.set(it->second);
                        }
                        if (auto it = ev.form.find("remember"); it != ev.form.end()) {
                            submittedRemember.set(it->second);
                        }
                    }}
                }, {
                    input({{"type", "text"}, {"name", "username"}, {"value", "john_doe"}}),
                    input({{"type", "checkbox"}, {"name", "remember"}, {"checked", true}, {"value", "yes"}}),
                    textarea({{"name", "bio"}, {"value", "Full-stack C++ dev"}}),
                    button({{"type", "submit"}, {"id", "submit-btn"}}, "Save Profile")
                })
            });
        }
};

class KeyboardAndMouseComponent : public Component {
    public:
        [[nodiscard]] std::string getComponentName() const override { return "KeyboardAndMouseComponent"; }

        [[nodiscard]] VNode render() const override {
            auto lastKey = useState<std::string>("last_key", "");
            auto ctrlPressed = useState<bool>("ctrl_pressed", false);
            auto mouseCoords = useState<std::string>("mouse_coords", "0,0");

            return div({}, {
                p({{"id", "key-display"}}, "Key: " + lastKey.get() + (ctrlPressed.get() ? " (Ctrl)" : "")),
                p({{"id", "coords-display"}}, "Coords: " + mouseCoords.get()),
                input({
                    {"id", "hotkey-input"},
                    {"onKeyDown", [lastKey, ctrlPressed](const Event& ev) {
                        lastKey.set(ev.key);
                        ctrlPressed.set(ev.ctrlKey);
                    }}
                }),
                div({
                    {"id", "clickable-area"},
                    {"onClick", [mouseCoords](const Event& ev) {
                        mouseCoords.set(std::to_string(static_cast<int>(ev.clientX)) + "," +
                                        std::to_string(static_cast<int>(ev.clientY)));
                    }}
                }, { VNode::textNode("Click Canvas") })
            });
        }
};

KEET_TEST(EventDispatcher, EventBubblingFromChildToParent) {
    auto mounted = keet::Testing::TestHarness::mount<BubblingComponent>();

    KEET_ASSERT_TEXT(mounted.find("#click-summary"), "Clicks: 0");

    // Click on inner .icon span, which does NOT have an onClick handler
    auto icon = mounted.find(".icon");
    KEET_ASSERT_EXISTS(icon);
    icon.triggerClick();

    // Event should bubble to parent-btn, incrementing clickCount
    KEET_ASSERT_TEXT(mounted.find("#click-summary"), "Clicks: 1");
    KEET_ASSERT_TEXT(mounted.find("#last-target"), "Target: button");

    // Click on .btn-text span
    mounted.find(".btn-text").triggerClick();
    KEET_ASSERT_TEXT(mounted.find("#click-summary"), "Clicks: 2");
}

KEET_TEST(EventDispatcher, AutomaticFormFieldHarvestingOnSubmit) {
    auto mounted = keet::Testing::TestHarness::mount<FormComponent>();

    KEET_ASSERT_TEXT(mounted.find("#status"), "Unsubmitted");

    // Trigger submit directly on the form
    auto formEl = mounted.find("#login-form");
    formEl.triggerSubmit();

    KEET_ASSERT_TEXT(mounted.find("#status"), "Submitted: john_doe");
    KEET_ASSERT_TEXT(mounted.find("#bio-status"), "Full-stack C++ dev");
    KEET_ASSERT_TEXT(mounted.find("#remember-status"), "yes");
}

KEET_TEST(EventDispatcher, SubmitBubblingFromSubmitButton) {
    auto mounted = keet::Testing::TestHarness::mount<FormComponent>();

    // Trigger submit via clicking or submitting on submit button inside the form
    auto submitBtn = mounted.find("#submit-btn");
    submitBtn.trigger("submit");

    KEET_ASSERT_TEXT(mounted.find("#status"), "Submitted: john_doe");
}

KEET_TEST(EventDispatcher, KeyboardEventsAndModifiers) {
    auto mounted = keet::Testing::TestHarness::mount<KeyboardAndMouseComponent>();

    auto inputEl = mounted.find("#hotkey-input");
    inputEl.triggerKeyDown("Enter", "Enter", /*shift*/false, /*ctrl*/true);

    KEET_ASSERT_TEXT(mounted.find("#key-display"), "Key: Enter (Ctrl)");

    inputEl.triggerKeyDown("Escape", "Escape", /*shift*/false, /*ctrl*/false);
    KEET_ASSERT_TEXT(mounted.find("#key-display"), "Key: Escape");
}

KEET_TEST(EventDispatcher, MouseCoordinates) {
    auto mounted = keet::Testing::TestHarness::mount<KeyboardAndMouseComponent>();

    auto canvas = mounted.find("#clickable-area");
    canvas.triggerClick(320.0, 240.0);

    KEET_ASSERT_TEXT(mounted.find("#coords-display"), "Coords: 320,240");
}

KEET_TEST_MAIN()
