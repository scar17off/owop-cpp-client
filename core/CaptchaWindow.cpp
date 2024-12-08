#include <owop-client/CaptchaWindow.hpp>
#include <owop-client/Constants.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include <WebView2.h>
#include <wrl.h>
#include <wil/com.h>
#include <string>
#include <sstream>
#include <Windows.h>

using namespace Microsoft::WRL;

namespace owop {

class WebView2Handler {
public:
    ComPtr<ICoreWebView2> webView;
    ComPtr<ICoreWebView2Controller> controller;
    HWND hwnd;
    std::function<void(const std::string&)> onToken;
};

CaptchaWindow::CaptchaWindow() : initialized(false), webView(nullptr) {
    webView = new WebView2Handler();
}

CaptchaWindow::~CaptchaWindow() {
    if (webView) {
        delete static_cast<WebView2Handler*>(webView);
    }
}

void CaptchaWindow::show(bool* p_open) {
    if (!initialized) {
        initWebView();
        initialized = true;
    }

    ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Verification Required", p_open)) {
        ImGui::Text("Please complete the CAPTCHA verification");
        
        auto handler = static_cast<WebView2Handler*>(webView);
        if (handler->webView) {
            // Update WebView position to match ImGui window
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImVec2 size = ImGui::GetContentRegionAvail();
            RECT bounds = { 
                static_cast<LONG>(pos.x), 
                static_cast<LONG>(pos.y),
                static_cast<LONG>(pos.x + size.x), 
                static_cast<LONG>(pos.y + size.y) 
            };
            handler->controller->put_Bounds(bounds);
        }

        ImGui::BeginChild("CaptchaFrame", ImVec2(0, 500), true);
        ImGui::EndChild();
    }
    ImGui::End();
}

void CaptchaWindow::initWebView() {
    auto handler = static_cast<WebView2Handler*>(webView);
    handler->hwnd = GetActiveWindow();
    handler->onToken = onToken;

    CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [handler](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
                env->CreateCoreWebView2Controller(handler->hwnd,
                    Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [handler](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
                            if (controller != nullptr) {
                                handler->controller = controller;
                                controller->get_CoreWebView2(&handler->webView);

                                // Configure WebView2 settings
                                ComPtr<ICoreWebView2Settings> settings;
                                handler->webView->get_Settings(&settings);
                                settings->put_IsScriptEnabled(TRUE);
                                settings->put_AreDefaultScriptDialogsEnabled(TRUE);
                                settings->put_IsWebMessageEnabled(TRUE);

                                // Set up message handler
                                handler->webView->add_WebMessageReceived(
                                    Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                                        [handler](ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
                                            wil::unique_cotaskmem_string message;
                                            args->TryGetWebMessageAsString(&message);
                                            if (handler->onToken && message) {
                                                int size = WideCharToMultiByte(CP_UTF8, 0, message.get(), -1, nullptr, 0, nullptr, nullptr);
                                                std::string token(size - 1, '\0');
                                                WideCharToMultiByte(CP_UTF8, 0, message.get(), -1, &token[0], size, nullptr, nullptr);
                                                handler->onToken(token);
                                            }
                                            return S_OK;
                                        }).Get(), nullptr);

                                // Create HTML content
                                std::string html = R"html(
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <script src="https://www.google.com/recaptcha/api.js" async defer></script>
</head>
<body style='margin:0;display:flex;justify-content:center;align-items:center;height:100vh;background:#333;'>
    <div id="recaptcha" 
         class="g-recaptcha"
         data-sitekey="6LcgvScUAAAAAARUXtwrM8MP0A0N70z4DHNJh-KI"
         data-callback="onCaptchaCompleted"
         data-size="invisible"
         data-badge="inline"></div>
    <button onclick="grecaptcha.execute()">Verify</button>
    <script>
        function onCaptchaCompleted(token) {
            window.chrome.webview.postMessage(token);
        }
        // Auto-click the verify button
        window.onload = function() {
            setTimeout(function() {
                grecaptcha.execute();
            }, 1000);
        };
    </script>
</body>
</html>
)html";

                                // Convert to wide string
                                int wsize = MultiByteToWideChar(CP_UTF8, 0, html.c_str(), -1, nullptr, 0);
                                std::wstring whtml(wsize - 1, L'\0');
                                MultiByteToWideChar(CP_UTF8, 0, html.c_str(), -1, &whtml[0], wsize);

                                // Navigate
                                handler->webView->NavigateToString(whtml.c_str());
                            }
                            return S_OK;
                        }).Get());
                return S_OK;
            }).Get());
}

void CaptchaWindow::checkToken() {
    // Token is handled by WebView2 message handler
}

} // namespace owop 