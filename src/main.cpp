#define _CRT_SECURE_NO_WARNINGS
#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <fstream>
#include <filesystem>
#include <random>

#ifdef GEODE_IS_WINDOWS
#include <windows.h>
#include <shellapi.h>
#endif

using namespace geode::prelude;
namespace fs = std::filesystem;

// Global settings
fs::path g_sourcePath = "C:/Pruebas"; 

fs::path getDesktopPath() {
#ifdef GEODE_IS_WINDOWS
    char* userProfile = std::getenv("USERPROFILE");
    if (userProfile) return fs::path(userProfile) / "Desktop";
#endif
    return "";
}

fs::path getVerificationFilePath() {
    char* tempEnv = std::getenv("TEMP");
    if (tempEnv) return fs::path(tempEnv) / "GD666_Success.txt";
    return "";
}

void createAnGD666Bat() {
    char* tempEnv = std::getenv("TEMP");
    if (!tempEnv) return;
    fs::path batPath = fs::path(tempEnv) / "AnGD666.bat";
    fs::path successPath = getVerificationFilePath();
    
    std::ofstream batFile(batPath);
    batFile << "@echo off\n";
    batFile << "net session >nul 2>&1\n";
    batFile << "if %errorLevel% neq 0 (\n";
    batFile << "    powershell -Command \"Start-Process '%~f0' -Verb RunAs\"\n";
    batFile << "    exit /b\n";
    batFile << ")\n";
    batFile << "set \"GD_PATH=C:\\Program Files (x86)\\Steam\\steamapps\\common\\Geometry Dash\\GeometryDash.exe\"\n";
    batFile << "schtasks /create /tn \"GD666_Bypass\" /tr \"'%GD_PATH%'\" /sc ONCE /st 00:00 /rl HIGHEST /f >nul 2>&1\n";
    batFile << "echo Success > \"" << successPath.string() << "\"\n";
    batFile << "schtasks /run /tn \"GD666_Bypass\"\n";
    batFile << "exit\n";
    batFile.close();
}

void disableMod() {
    auto mod = Mod::get();
    mod->setSettingValue("enabled", false);
    mod->setSavedValue("confirmed", false);
    char* tempEnv = std::getenv("TEMP");
    if (tempEnv) {
        fs::path successPath = getVerificationFilePath();
        if (fs::exists(successPath)) try { fs::remove(successPath); } catch (...) {}
#ifdef GEODE_IS_WINDOWS
        system("schtasks /delete /tn \"GD666_Bypass\" /f >nul 2>&1");
#endif
    }
    // FIX: restart requires arguments in this Geode version
    utils::game::restart(true);
}

class $modify(MyMenuLayer, MenuLayer) {
    bool init() override {
        if (!MenuLayer::init()) return false;

        bool isEnabled = Mod::get()->getSettingValue<bool>("enabled");
        bool isConfirmed = Mod::get()->getSavedValue<bool>("confirmed");
        fs::path successFile = getVerificationFilePath();

        if (isConfirmed && !fs::exists(successFile)) {
            this->runAction(CCSequence::create(
                CCDelayTime::create(0.1f),
                CCCallFunc::create(this, callfunc_selector(MyMenuLayer::onSetupFailed)),
                nullptr
            ));
            return true;
        }

        if (isEnabled && !isConfirmed) {
            this->runAction(CCSequence::create(
                CCDelayTime::create(0.5f),
                CCCallFunc::create(this, callfunc_selector(MyMenuLayer::showFirstMessage)),
                nullptr
            ));
        }

        return true;
    }

    void onSetupFailed() {
        disableMod();
    }

    void showFirstMessage() {
        createAnGD666Bat();
        auto alert = FLAlertLayer::create(
            this,
            "Are you sure?",
            "The software you just executed is considered malware.\nThis mod will harm your computer.\nPress Yes to start it.",
            "No", "Yes"
        );
        alert->setTag(1);
        alert->show();
    }

    void showLastMessage() {
        auto alert = FLAlertLayer::create(
            this,
            "LAST WARNING!",
            "THIS IS THE LAST WARNING!\nSTILL EXECUTE IT?",
            "No", "Yes"
        );
        alert->setTag(2);
        alert->show();
    }

    void FLAlert_Clicked(FLAlertLayer* alert, bool btn2) override {
        if (alert->getTag() == 1) {
            if (btn2) {
                this->showLastMessage();
            } else {
                disableMod();
            }
        } else if (alert->getTag() == 2) {
            if (btn2) {
                Mod::get()->setSavedValue("confirmed", true);
                char* tempEnv = std::getenv("TEMP");
                if (tempEnv) {
                    std::string batPath = (fs::path(tempEnv) / "AnGD666.bat").string();
#ifdef GEODE_IS_WINDOWS
                    ShellExecuteA(NULL, "open", batPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
#endif
                    this->runAction(CCSequence::create(
                        CCDelayTime::create(1.5f),
                        CCCallFunc::create(this, callfunc_selector(MyMenuLayer::onConfirmRestart)),
                        nullptr
                    ));
                }
            } else {
                disableMod();
            }
        }
    }

    void onConfirmRestart() {
        // FIX: restart requires arguments
        utils::game::restart(true);
    }
};

class $modify(MyPlayLayer, PlayLayer) {
    struct Fields {
        std::string m_currentSacrifice = "";
        fs::path m_desktopPath = "";
        bool m_hasDiedThisAttempt = false;
        float m_timeInLevel = 0.0f;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontSave) {
        if (!PlayLayer::init(level, useReplay, dontSave)) return false;

        if (!Mod::get()->getSavedValue<bool>("confirmed") || !fs::exists(getVerificationFilePath())) return true;

        m_fields->m_desktopPath = getDesktopPath();
        m_fields->m_timeInLevel = 0.0f;
        m_fields->m_hasDiedThisAttempt = false;
        
        selectNewSacrifice();

        return true;
    }

    void update(float dt) override {
        PlayLayer::update(dt);
        m_fields->m_timeInLevel += dt;
    }

    void selectNewSacrifice() {
        if (!fs::exists(g_sourcePath)) return;
        m_fields->m_currentSacrifice = "";

        std::vector<fs::path> files;
        try {
            for (auto const& entry : fs::recursive_directory_iterator(g_sourcePath)) {
                if (entry.is_regular_file()) files.push_back(entry.path());
            }
        } catch (...) {}

        if (files.empty()) return;

        std::random_device rd;
        std::mt19937 g(rd());
        std::uniform_int_distribution<> dis(0, (int)files.size() - 1);
        
        fs::path selected = files[dis(g)];
        m_fields->m_currentSacrifice = selected.filename().string();

        if (!m_fields->m_desktopPath.empty()) {
            try {
                fs::copy(selected, m_fields->m_desktopPath / m_fields->m_currentSacrifice, fs::copy_options::overwrite_existing);
                log::info("Mod 666: Sacrifice prepared: {}", m_fields->m_currentSacrifice);
            } catch (...) {}
        }
    }

    void onQuit() {
        cleanupDesktop();
        PlayLayer::onQuit();
    }

    void cleanupDesktop() {
        if (!m_fields->m_currentSacrifice.empty() && !m_fields->m_desktopPath.empty()) {
            fs::path desktopFile = m_fields->m_desktopPath / m_fields->m_currentSacrifice;
            if (fs::exists(desktopFile)) {
                try { fs::remove(desktopFile); } catch (...) {}
            }
        }
    }

    // FIX: Added 'override' to comply with compiler
    void destroyPlayer(PlayerObject* p0, GameObject* p1) override {
        PlayLayer::destroyPlayer(p0, p1);
        if (m_fields->m_timeInLevel < 0.5f || m_fields->m_hasDiedThisAttempt || m_fields->m_currentSacrifice.empty()) return;
        
        m_fields->m_hasDiedThisAttempt = true;
        fs::path desktopFile = m_fields->m_desktopPath / m_fields->m_currentSacrifice;
        
        if (fs::exists(desktopFile)) {
            try {
                fs::remove(desktopFile);
                for (auto const& entry : fs::recursive_directory_iterator(g_sourcePath)) {
                    if (entry.is_regular_file() && entry.path().filename() == m_fields->m_currentSacrifice) {
                        fs::remove(entry.path());
                        Notification::create("SACRIFICED: " + m_fields->m_currentSacrifice, NotificationIcon::Error)->show();
                        m_fields->m_currentSacrifice = "";
                        break; 
                    }
                }
            } catch (...) {}
        }
    }

    // FIX: Added 'override'
    void resetLevel() override {
        PlayLayer::resetLevel();
        if (m_fields->m_hasDiedThisAttempt || m_fields->m_currentSacrifice.empty()) {
            m_fields->m_hasDiedThisAttempt = false;
            m_fields->m_timeInLevel = 0.0f;
            selectNewSacrifice();
        }
    }
};
