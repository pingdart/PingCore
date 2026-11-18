// examples/login_form/main.cpp
// PingDartUI Secure Reactive Login Form Example (PKG-138)
#include <pingdart/ui.h>
#include <iostream>
#include <string_view>

using namespace pingdart::ui;

Component LoginForm() {
    auto email_input = Input("email", "email", "doctor@hospital.org", "doctor@stanford.edu");
    auto pass_input = Input("password", "password", "Enter secure password...", "ClinicalPass2026!");

    return Card({
        Heading("Clinician Portal Login", 2),
        Text("Authenticate to access patient clinical health records.").color("#64748B"),
        Divider(),
        Form({
            FormField("Email Address", email_input),
            FormField("Account Password", pass_input),
            Button("Sign In")
                .variant("primary")
                .attr("type", "submit")
                .padding(12, 24)
        }).onSubmit([](const FormData& data) {
            std::cout << "[Login Submitted]: email=" << data.get("email") 
                      << ", password=" << (data.has("password") ? "********" : "missing") 
                      << std::endl;
        })
    }).className("login-card").padding(28).border(1, "#E2E8F0").borderRadius(12).width("420px");
}

int main(int argc, char** argv) {
    if (argc > 1 && std::string_view(argv[1]) == "--render-html") {
        std::cout << "<!doctype html>\n"
                  << "<html lang=\"en\"><head><meta charset=\"utf-8\">"
                  << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
                  << "<title>Clinician Login</title></head><body>"
                  << render_to_html(LoginForm())
                  << "</body></html>\n";
        return 0;
    }

    std::cout << "==========================================" << std::endl;
    std::cout << " PingDartUI — Secure Login Form Demo      " << std::endl;
    std::cout << "==========================================" << std::endl;

    auto app = App(argc, argv).title("Clinician Login").render(LoginForm);
    app.run();

    auto form = LoginForm();
    std::string html = render_to_html(form);
    std::cout << "[Web SSR Output]:\n" << html << std::endl;

    bool has_email_attr = (html.find("type=\"email\"") != std::string::npos && html.find("name=\"email\"") != std::string::npos);
    bool has_pass_attr = (html.find("type=\"password\"") != std::string::npos && html.find("name=\"password\"") != std::string::npos);
    std::cout << "Explicit Field Names & Types Verified: " << (has_email_attr && has_pass_attr ? "YES" : "NO") << std::endl;

    return 0;
}
