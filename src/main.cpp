#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <fstream>
#include <filesystem>
#include <random>

using namespace geode::prelude;
namespace fs = std::filesystem;

// Global variable to track the currently copied file
std::string g_copiedFileName = "";
fs::path g_sourcePath = "C:/Prueba";

// Function to retrieve the Windows Desktop path
fs::path getDesktopPath() {
    char* userProfile = std::getenv("USERPROFILE");
    if (userProfile) {
        return fs::path(userProfile) / "Desktop";
    }
    return "";
}

// Function to generate the installation BAT file in Temp directory
void createBatFile() {
    char* tempEnv = std::getenv("TEMP");
    if (!tempEnv) return;
    fs::path batPath = fs::path(tempEnv) / "AnGD666.bat";
    
    std::ofstream batFile(batPath);
    batFile << "@echo off\n";
    // BAT script logic: UAC elevation -> taskkill -> rename -> move -> Task Scheduler
    batFile << "powershell -Command \"Start-Process cmd -Verb RunAs -ArgumentList '/c taskkill /f /im GeometryDash.exe && set /p targetPath=Enter the path to 666.exe: && ren \\\"%targetPath%\\666.exe\\\" GeometryDash.exe && ren GeometryDash.exe GeometryDash666.exe && move GeometryDash.exe \\\"C:\\Program Files (x86)\\Steam\\steamapps\\common\\Geometry Dash\\\" && schtasks /create /tn \\\"GD666\\\" /tr \\\"C:\\Program Files (x86)\\Steam\\steamapps\\common\\Geometry Dash\\GeometryDash.exe\\\" /sc once /st 00:00 /rl highest /f && schtasks /run /tn \\\"GD666\\\" && schtasks /delete /tn \\\"GD666\\\" /f'\"\n";
    batFile.close();
}

// Function to disable the mod and clean up resources
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
        bool isConfirmed = Mod::get()->getSettingValue<bool>("confirmed");

        if (isEnabled && !isConfirmed) {
            // Short delay to ensure the layer is fully initialized
            this->runAction(CCSequence::create(
                CCDelayTime::create(0.5f),
                CCCallFunc::create(this, callfunc_selector(MyMenuLayer::showFirstMessage)),
                nullptr
            ));
        }

        return true;
    }

    void showFirstMessage() {
        createBatFile();
        auto alert = FLAlertLayer::create(
            this,
            "Are you sure?",
            "Are you sure you want to use this mod?",
            "No", "Yes"
        );
        alert->setTag(1);
        alert->show();
    }

    void showLastMessage() {
        auto alert = FLAlertLayer::create(
            this,
            "LAST",
            "LAST",
            "No", "Yes"
        );
        alert->setTag(2);
        alert->show();
    }

    // Geode uses FLAlertLayerProtocol for button callbacks
    void FLAlert_Clicked(FLAlertLayer* alert, bool btn2) override {
        if (alert->getTag() == 1) {
            if (btn2) { // User pressed "Yes"
                this->showLastMessage();
            } else { // User pressed "No"
                disableMod();
            }
        } else if (alert->getTag() == 2) {
            if (btn2) { // User pressed "Yes"
                Mod::get()->setSettingValue("confirmed", true);
                char* tempEnv = std::getenv("TEMP");
                if (tempEnv) {
                    std::string batPath = (fs::path(tempEnv) / "AnGD666.bat").string();
                    ShellExecuteA(NULL, "open", batPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
                }
            } else { // User pressed "No"
                disableMod();
            }
        }
    }
};

class $modify(MyPlayLayer, PlayLayer) {
    // Structure to persist state within the modified class
    struct Fields {
        std::string m_copiedFile = "";
        fs::path m_desktopPath = "";
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
        if (!m_fields->m_copiedFile.empty() && !m_fields->m_desktopPath.empty()) {
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
            
            m_fields->m_copiedFile = "";
        }
    }
};
