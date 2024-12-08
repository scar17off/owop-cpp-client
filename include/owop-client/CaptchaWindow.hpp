#pragma once
#include <string>
#include <functional>
#include <imgui.h>

namespace owop {

class CaptchaWindow {
public:
    using TokenCallback = std::function<void(const std::string&)>;

    CaptchaWindow();
    ~CaptchaWindow();

    void show(bool* p_open = nullptr);
    void setOnToken(TokenCallback callback) { onToken = callback; }

private:
    bool initialized;
    void* webView;  // ImGui web view handle
    TokenCallback onToken;

    void initWebView();
    void checkToken();
};

} // namespace owop 