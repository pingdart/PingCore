#include <gtest/gtest.h>
#include <pingdart/web/dom_event.h>
#include "../../src/web/dom/dom_event_dispatcher.h"

using namespace pingdart::web;

TEST(DomEventsTest, DispatchByHandle) {
    DomEventDispatcher dispatcher;

    DomNodeHandle btn_handle{42};
    bool clicked = false;

    dispatcher.add_handler(btn_handle, DomEventType::Click, [&](const DomEvent& ev) {
        clicked = true;
        EXPECT_EQ(ev.target, btn_handle);
    });

    DomEvent event{
        .type = DomEventType::Click,
        .target = btn_handle
    };

    bool handled = dispatcher.dispatch(event);
    EXPECT_TRUE(handled);
    EXPECT_TRUE(clicked);
}

TEST(DomEventsTest, DispatchByDeterministicId) {
    DomEventDispatcher dispatcher;

    std::string text_value;
    dispatcher.add_handler_by_id("pd_root_k_search_input", DomEventType::Input, [&](const DomEvent& ev) {
        text_value = ev.value;
    });

    DomEvent input_event{
        .type = DomEventType::Input,
        .target_pd_id = "pd_root_k_search_input",
        .value = "Cardiology"
    };

    bool handled = dispatcher.dispatch(input_event);
    EXPECT_TRUE(handled);
    EXPECT_EQ(text_value, "Cardiology");
}

TEST(DomEventsTest, PreventDefaultHandling) {
    DomEventDispatcher dispatcher;

    DomNodeHandle form_handle{100};
    dispatcher.add_handler(form_handle, DomEventType::Submit, [](const DomEvent& ev) {
        // Prevent default browser form post
        const_cast<DomEvent&>(ev).prevent_default();
    });

    DomEvent submit_event{
        .type = DomEventType::Submit,
        .target = form_handle
    };

    EXPECT_FALSE(submit_event.default_prevented);
    dispatcher.dispatch(submit_event);
    EXPECT_TRUE(submit_event.default_prevented);
}
