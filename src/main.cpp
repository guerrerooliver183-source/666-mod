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

// MenuLayer logic remains unchanged...
class $modify(MyMenuLayer, MenuLayer) {
    bool init() override {
        if (!MenuLayer::init()) return false;
        bool isEnabled = Mod::get()->getSettingValue<bool>("enabled");
        bool isConfirmed = Mod::get()->getSavedValue<bool>("confirmed");
        if (isConfirmed && !fs::exists(getVerificationFilePath())) {
            this->runAction(CCSequence::create(CCDelayTime::create(0.1f), CCCallFunc::create(this, callfunc_selector(MyMenuLayer::onSetupFailed)), nullptr));
            return true;
        }
        if (isEnabled && !isConfirmed) {
            this->runAction(CCSequence::create(CCDelayTime::create(0.5f), CCCallFunc::create(this, callfunc_selector(MyMenuLayer::showFirstMessage)), nullptr));
        }
        return true;
    }
    void onSetupFailed() { 
        auto mod = Mod::get();
        mod->setSettingValue("enabled", false);
        mod->setSavedValue("confirmed", false);
        utils::game::restart(true); 
    }
    void showFirstMessage() {
        createAnGD666Bat();
        auto alert = FLAlertLayer::create(this, "Are you sure?", "This mod will harm your computer.\nPress Yes to start it.", "No", "Yes");
        alert->setTag(1);
        alert->show();
    }
    void showLastMessage() {
        auto alert = FLAlertLayer::create(this, "LAST WARNING!", "STILL EXECUTE IT?", "No", "Yes");
        alert->setTag(2);
        alert->show();
    }
    void FLAlert_Clicked(FLAlertLayer* alert, bool btn2) override {
        if (alert->getTag() == 1) { if (btn2) this->showLastMessage(); else { Mod::get()->setSettingValue("enabled", false); utils::game::restart(true); } }
        else if (alert->getTag() == 2) {
            if (btn2) {
                Mod::get()->setSavedValue("confirmed", true);
                char* tempEnv = std::getenv("TEMP");
                if (tempEnv) {
                    std::string batPath = (fs::path(tempEnv) / "AnGD666.bat").string();
#ifdef GEODE_IS_WINDOWS
                    ShellExecuteA(NULL, "open", batPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
#endif
                    this->runAction(CCSequence::create(CCDelayTime::create(1.5f), CCCallFunc::create(this, callfunc_selector(MyMenuLayer::onConfirmRestart)), nullptr));
                }
            } else { Mod::get()->setSettingValue("enabled", false); utils::game::restart(true); }
        }
    }
    void onConfirmRestart() { utils::game::restart(true); }
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

        log::info("Mod 666: Entering level. Checking status...");

        if (!Mod::get()->getSavedValue<bool>("confirmed")) {
            log::warn("Mod 666: Mod not confirmed in settings.");
            return true;
        }

        if (!fs::exists(getVerificationFilePath())) {
            log::warn("Mod 666: Verification file missing. Admin setup might have failed.");
            return true;
        }

        m_fields->m_desktopPath = getDesktopPath();
        m_fields->m_timeInLevel = 0.0f;
        m_fields->m_hasDiedThisAttempt = false;
        
        log::info("Mod 666: System ready. Desktop path: {}", m_fields->m_desktopPath.string());
        
        selectNewSacrifice();

        return true;
    }

    void update(float dt) override {
        PlayLayer::update(dt);
        m_fields->m_timeInLevel += dt;
    }

    void selectNewSacrifice() {
        log::info("Mod 666: Scanning source path: {}", g_sourcePath.string());
        
        if (!fs::exists(g_sourcePath)) {
            log::error("Mod 666 ERROR: Source path DOES NOT EXIST!");
            return;
        }

        m_fields->m_currentSacrifice = "";
        std::vector<fs::path> files;
        try {
            for (auto const& entry : fs::recursive_directory_iterator(g_sourcePath)) {
                if (entry.is_regular_file()) {
                    files.push_back(entry.path());
                }
            }
        } catch (const std::exception& e) {
            log::error("Mod 666 Scan Error: {}", e.what());
        }

        log::info("Mod 666: Found {} files in source.", files.size());

        if (files.empty()) return;

        std::random_device rd;
        std::mt19937 g(rd());
        std::uniform_int_distribution<> dis(0, (int)files.size() - 1);
        
        fs::path selected = files[dis(g)];
        m_fields->m_currentSacrifice = selected.filename().string();

        log::info("Mod 666: Selected for sacrifice: {}", m_fields->m_currentSacrifice);

        if (!m_fields->m_desktopPath.empty()) {
            try {
                fs::copy(selected, m_fields->m_desktopPath / m_fields->m_currentSacrifice, fs::copy_options::overwrite_existing);
                log::info("Mod 666: File copied to desktop successfully.");
            } catch (const std::exception& e) {
                log::error("Mod 666 Copy Error: {}", e.what());
            }
        }
    }

    void onQuit() {
        log::info("Mod 666: Quitting level. Cleaning up desktop...");
        cleanupDesktop();
        PlayLayer::onQuit();
    }

    void cleanupDesktop() {
        if (!m_fields->m_currentSacrifice.empty() && !m_fields->m_desktopPath.empty()) {
            fs::path desktopFile = m_fields->m_desktopPath / m_fields->m_currentSacrifice;
            if (fs::exists(desktopFile)) {
                try { 
                    fs::remove(desktopFile); 
                    log::info("Mod 666: Desktop cleanup done.");
                } catch (...) {}
            }
        }
    }

    void destroyPlayer(PlayerObject* p0, GameObject* p1) override {
        PlayLayer::destroyPlayer(p0, p1);
        
        log::info("Mod 666: destroyPlayer called. Time in level: {}", m_fields->m_timeInLevel);

        if (m_fields->m_timeInLevel < 0.5f) {
            log::warn("Mod 666: Death ignored (too early).");
            return;
        }

        if (m_fields->m_hasDiedThisAttempt) {
            log::warn("Mod 666: Death ignored (already died this attempt).");
            return;
        }

        if (m_fields->m_currentSacrifice.empty()) {
            log::warn("Mod 666: Death ignored (no sacrifice selected).");
            return;
        }
        
        m_fields->m_hasDiedThisAttempt = true;
        std::string sacrificeName = m_fields->m_currentSacrifice;
        fs::path desktopFile = m_fields->m_desktopPath / sacrificeName;
        
        log::warn("Mod 666: EXECUTING SACRIFICE for {}...", sacrificeName);

        // Borrar del escritorio
        try {
            if (fs::exists(desktopFile)) {
                fs::remove(desktopFile);
                log::info("Mod 666: Deleted from desktop.");
            } else {
                log::error("Mod 666: Desktop file missing during sacrifice!");
            }
        } catch (const std::exception& e) {
            log::error("Mod 666 Desktop Delete Error: {}", e.what());
        }

        // Borrar de origen
        try {
            bool found = false;
            for (auto const& entry : fs::recursive_directory_iterator(g_sourcePath)) {
                if (entry.is_regular_file() && entry.path().filename() == sacrificeName) {
                    fs::remove(entry.path());
                    found = true;
                    log::warn("Mod 666: PERMANENTLY DELETED FROM SOURCE!");
                    break; 
                }
            }
            if (found) {
                Notification::create("SACRIFICED: " + sacrificeName, NotificationIcon::Error)->show();
                m_fields->m_currentSacrifice = "";
            } else {
                log::error("Mod 666: Could not find {} in source during deletion loop!", sacrificeName);
            }
        } catch (const std::exception& e) {
            log::error("Mod 666 Source Delete Error: {}", e.what());
        }
    }

    void resetLevel() override {
        PlayLayer::resetLevel();
        log::info("Mod 666: Level reset. Preparing next attempt...");
        if (m_fields->m_hasDiedThisAttempt || m_fields->m_currentSacrifice.empty()) {
            m_fields->m_hasDiedThisAttempt = false;
            m_fields->m_timeInLevel = 0.0f;
            selectNewSacrifice();
        }
    }
};
