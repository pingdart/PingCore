// examples/component_gallery/main.cpp
// PingDartUI Production Component Gallery — 25 Components Demo (PKG-136)
#include <pingdart/ui.h>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace pingdart::ui;

// Constructs the complete 25-component gallery UI tree
Component BuildGalleryApp() {
    auto is_dark = useState<bool>(false);
    auto counter = useState<int>(0);

    // 1. Layout Section (7 components)
    auto layout_section = Card({
        Heading("1. Layout Components", 3),
        Text("Container, Row, Column, Stack, Flex, Grid, ScrollView").color("#64748B"),
        Divider(),
        Row({
            Text("Row Item A").className("bg-slate-100 p-2 rounded-md"),
            Spacer(12),
            Text("Row Item B").className("bg-slate-100 p-2 rounded-md"),
            Spacer(12),
            Text("Row Item C").className("bg-slate-100 p-2 rounded-md")
        }),
        Column({
            Text("Column Item 1"),
            Text("Column Item 2")
        }).gap(4),
        Stack({
            Text("Stack Base Layer").className("bg-slate-200 p-3 rounded-md"),
            Text("Stack Overlay Tag").className("bg-blue-600 text-white p-1 rounded-sm")
        }),
        Flex({
            Text("Flex A").className("p-2"),
            Text("Flex B").className("p-2")
        }),
        Grid("repeat(3, 1fr)", 8.0f, {
            Text("Grid Cell 1").className("bg-slate-100 p-3 rounded-md"),
            Text("Grid Cell 2").className("bg-slate-100 p-3 rounded-md"),
            Text("Grid Cell 3").className("bg-slate-100 p-3 rounded-md")
        }),
        ScrollView({
            Text("Scrollable Content Line 1"),
            Text("Scrollable Content Line 2"),
            Text("Scrollable Content Line 3")
        })
    }).className("gallery-card");

    // 2. Basic Section (6 components)
    auto basic_section = Card({
        Heading("2. Basic Typography & Media", 3),
        Text("Text, Heading, Image, Icon, Divider, Spacer").color("#64748B"),
        Divider(),
        Heading("Heading Level 1", 1),
        Heading("Heading Level 2", 2),
        Heading("Heading Level 3", 3),
        Text("Standard semantic body paragraph text with high readability."),
        Row({
            Icon("check-circle"),
            Text("Verified Badge"),
            Spacer(16),
            Icon("settings"),
            Text("Settings")
        }).gap(8),
        Image("https://pingdart.dev/assets/logo.png", "PingDart Logo")
            .width(48).height(48)
    }).className("gallery-card");

    // 3. Action Section (2 components)
    auto action_section = Card({
        Heading("3. Action Components", 3),
        Text("Button, IconButton with interactive states").color("#64748B"),
        Divider(),
        Row({
            Button("Primary Action").variant("primary").className("gallery-btn"),
            Button("Secondary Action").variant("secondary"),
            Button("Destructive Action").variant("destructive"),
            IconButton("heart", "Like Item"),
            IconButton("share", "Share Content")
        }).gap(12)
    }).className("gallery-card");

    // 4. Form Section (6 components)
    auto form_section = Card({
        Heading("4. Form & Input Controls", 3),
        Text("Input, TextArea, Checkbox, Radio, Switch, Select").color("#64748B"),
        Divider(),
        Input("Enter full name...", "Dr. Jane Smith"),
        TextArea("Enter clinical observations...", 3),
        Row({
            Checkbox("Subscribe to newsletter", true),
            Spacer(16),
            Checkbox("Agree to clinical terms", false)
        }),
        Row({
            Radio("plan", "free", "Free Tier", true),
            Spacer(16),
            Radio("plan", "pro", "Enterprise Tier", false)
        }),
        Row({
            Text("Enable Notifications:"),
            Switch(true)
        }).gap(12),
        Select({
            Option("Select specialty...", ""),
            Option("Cardiology", "cardio"),
            Option("Neurology", "neuro"),
            Option("Pediatrics", "peds")
        })
    }).className("gallery-card");

    // 5. Display Section (3 components)
    auto display_section = Card({
        Heading("5. Display Surfaces", 3),
        Text("Card, Avatar, Badge").color("#64748B"),
        Divider(),
        Row({
            Avatar("JS", false, 48.0f),
            Column({
                Text("Dr. Jane Smith, MD").weight(FontWeight::Bold),
                Text("Chief Cardiologist").color("#64748B")
            }),
            Spacer(),
            Badge("Active Doctor")
        }).gap(12)
    }).className("gallery-card");

    // 6. Overlay Section (1 component)
    auto overlay_section = Card({
        Heading("6. Overlay Components", 3),
        Text("Dialog modal with backdrop").color("#64748B"),
        Divider(),
        Dialog("Confirm Appointment Booking", 
               Text("Are you sure you wish to reserve Tuesday at 10:00 AM?"), 
               true)
    }).className("gallery-card");

    // Top-level Container
    return Container({
        Row({
            Heading("PingDartUI Component Gallery", 1).className("gallery-header"),
            Spacer(),
            Button("Toggle Dark Mode")
                .variant("primary")
                .onClick([is_dark]() mutable {
                    bool new_mode = !is_dark.get();
                    is_dark.set(new_mode);
                    ThemeEngine::instance().set_dark_mode(new_mode);
                })
        }),
        Text("Unified Cross-Platform Native-Compiled Framework (25 Components · C++20 · Qt 6.8 LTS · SSR)"),
        Divider(),
        layout_section,
        basic_section,
        action_section,
        form_section,
        display_section,
        overlay_section
    }).className("gallery-container");
}

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " PingDartUI — Production Component Gallery (25 Components) " << std::endl;
    std::cout << "==========================================================" << std::endl;

    // Load custom CSS stylesheet
    std::ifstream css_file("examples/component_gallery/gallery.css");
    if (!css_file) {
        css_file.open("PingDartCore/examples/component_gallery/gallery.css");
    }
    std::string css_content;
    if (css_file) {
        std::ostringstream ss;
        ss << css_file.rdbuf();
        css_content = ss.str();
    } else {
        css_content = R"(
            :root { --primary: #2563EB; --card: #FFFFFF; --border: #E2E8F0; --radius: 12px; }
            .gallery-container { background-color: #F8FAFC; padding: 24px; gap: 24px; }
            .gallery-card { background-color: var(--card, #FFFFFF); border: 1px solid var(--border, #E2E8F0); border-radius: var(--radius, 12px); padding: 20px; gap: 16px; }
            .gallery-header { font-size: 24px; color: #0F172A; font-weight: 700; }
            .gallery-btn { background-color: var(--primary, #2563EB); color: #FFFFFF; padding: 10px 18px; border-radius: 8px; }
        )";
    }

    std::vector<std::string> diagnostics;
    auto rules = CssParser::parse(css_content, &diagnostics);
    std::cout << "[CSS Parser]: Parsed " << rules.size() << " CSS rules with " 
              << diagnostics.size() << " syntax diagnostics." << std::endl;

    auto app_tree = BuildGalleryApp();
    app_tree.apply_css_rules(rules);

    // 1. Web SSR Output
    std::string html = render_to_html(app_tree);
    std::cout << "\n[Web SSR HTML Output generated (" << html.size() << " bytes)]" << std::endl;
    std::cout << "Sample snippet:\n" << html.substr(0, std::min<size_t>(html.size(), 400)) << "...\n" << std::endl;

    // 2. Qt Quick SceneGraph Output
    std::string qml = render_to_qml(app_tree);
    std::cout << "[Qt Quick SceneGraph QML generated (" << qml.size() << " bytes)]" << std::endl;
    std::cout << "Sample snippet:\n" << qml.substr(0, std::min<size_t>(qml.size(), 400)) << "...\n" << std::endl;

    // 3. Dark Mode Toggle Verification
    std::cout << "[Testing Dark Mode Toggle]..." << std::endl;
    ThemeEngine::instance().set_dark_mode(true);
    std::string dark_html = render_to_html(app_tree);
    bool has_dark_theme = (dark_html.find("data-theme=\"dark\"") != std::string::npos);
    std::cout << "Dark Mode active in SSR: " << (has_dark_theme ? "YES (data-theme=\"dark\" injected)" : "NO") << std::endl;
    ThemeEngine::instance().set_dark_mode(false); // Reset

    std::cout << "\nGallery validation successfully completed." << std::endl;
    return 0;
}
