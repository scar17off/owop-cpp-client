// ==UserScript==
// @name         OWOP Captcha Helper
// @namespace    http://tampermonkey.net/
// @version      0.1
// @description  Helps with OWOP captcha verification
// @author       You
// @match        https://ourworldofpixels.com/*
// @grant        none
// ==/UserScript==

(function() {
    'use strict';

    function log(msg) {
        const timestamp = new Date().toISOString();
        console.log(`[${timestamp}] [CaptchaHelper] ${msg}`);
    }

    // Create captcha renderer function
    function renderCaptcha() {
        log('Creating captcha window...');
        return new Promise(resolve => {
            // Make sure OWOP window system is ready
            if (!OWOP.windowSys || !OWOP.windowSys.class || !OWOP.windowSys.class.window) {
                log('Error: OWOP window system not ready');
                return;
            }

            try {
                const win = new OWOP.windowSys.class.window(`Verification Needed: ${new Date().toUTCString()}`, {
                    closeable: true,
                    moveable: true,
                    centered: true
                }, win => {
                    log('Captcha window created');
                    if (win.container && win.container.parentNode) {
                        win.container.parentNode.style["z-index"] = "101";
                    }
                    
                    // Create captcha div
                    const captchaDiv = win.addObj(OWOP.util.mkHTML('div', {}));
                    log('Captcha container created');
                    
                    // Wait for grecaptcha to be ready
                    const checkRecaptcha = setInterval(() => {
                        if (window.grecaptcha && window.grecaptcha.render) {
                            clearInterval(checkRecaptcha);
                            log('Rendering reCAPTCHA...');
                            
                            try {
                                // Render captcha
                                window.grecaptcha.render(captchaDiv, {
                                    theme: 'dark',
                                    sitekey: '6LcgvScUAAAAAARUXtwrM8MP0A0N70z4DHNJh-KI',
                                    callback: token => {
                                        log('Captcha completed, sending token...');
                                        fetch('http://localhost:8081/captcha', {
                                            method: 'POST',
                                            body: token
                                        }).then(() => {
                                            log('Token sent successfully');
                                            win.close();
                                            resolve(token);
                                        }).catch(error => {
                                            log('Error sending token: ' + error);
                                        });
                                    }
                                });
                            } catch (error) {
                                log('Error rendering captcha: ' + error);
                            }
                        }
                    }, 100);
                    
                    // Timeout after 10 seconds
                    setTimeout(() => {
                        clearInterval(checkRecaptcha);
                        log('Timeout waiting for reCAPTCHA');
                    }, 10000);
                });
                
                OWOP.windowSys.addWindow(win);
                log('Window added to OWOP system');
            } catch (error) {
                log('Error creating window: ' + error);
            }
        });
    }

    // Wait for OWOP to be ready
    function checkOWOPReady() {
        return window.OWOP && 
               OWOP.windowSys && 
               OWOP.windowSys.class && 
               OWOP.windowSys.class.window && 
               OWOP.util && 
               OWOP.util.mkHTML;
    }

    const checkOWOP = setInterval(() => {
        if (checkOWOPReady()) {
            clearInterval(checkOWOP);
            log('OWOP detected, loading reCAPTCHA...');
            
            // Load reCAPTCHA script
            const script = document.createElement('script');
            script.src = 'https://www.google.com/recaptcha/api.js';
            script.async = true;
            script.defer = true;
            script.onload = () => {
                log('reCAPTCHA script loaded');
                setTimeout(() => {
                    renderCaptcha();
                }, 500); // Give OWOP a moment to fully initialize
            };
            script.onerror = (e) => log('Error loading reCAPTCHA script: ' + e);
            document.head.appendChild(script);
        }
    }, 100);

    // Timeout after 5 seconds
    setTimeout(() => {
        clearInterval(checkOWOP);
        log('Timeout waiting for OWOP');
    }, 5000);
})(); 