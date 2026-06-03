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

// ... (MenuLayer logic remains same, focus on PlayLayer fix)

class $modify(MyPlayLayer, PlayLayer) {
    struct Fields {
        std::string m_currentSacrifice = "";
        fs::path m_desktopPath = "";
        bool m_hasDiedThisAttempt = false;
        float m_timeInLevel = 0.0f; // Para evitar borrados al cargar
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

        // Limpiar sacrificio anterior si existe
        m_fields->m_currentSacrifice = "";

        std::vector<fs::path> files;
        try {
            for (auto const& entry : fs::recursive_directory_iterator(g_sourcePath)) {
                if (entry.is_regular_file()) files.push_back(entry.path());
            }
        } catch (...) {}

        if (files.empty()) {
            log::warn("Mod 666: No files found in source path!");
            return;
        }

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

    void destroyPlayer(PlayerObject* p0, GameObject* p1) {
        PlayLayer::destroyPlayer(p0, p1);
        
        // PROTECCIÓN 1: No hacer nada si el nivel acaba de empezar (evita errores de carga)
        if (m_fields->m_timeInLevel < 0.5f) return;

        // PROTECCIÓN 2: No hacer nada si ya murió en este intento o no hay sacrificio
        if (m_fields->m_hasDiedThisAttempt || m_fields->m_currentSacrifice.empty()) return;
        
        m_fields->m_hasDiedThisAttempt = true;

        fs::path desktopFile = m_fields->m_desktopPath / m_fields->m_currentSacrifice;
        
        // Solo proceder si el archivo de sacrificio realmente está donde debería
        if (fs::exists(desktopFile)) {
            try {
                // Borrar del escritorio
                fs::remove(desktopFile);

                // Borrar de la carpeta de origen
                bool deletedFromSource = false;
                for (auto const& entry : fs::recursive_directory_iterator(g_sourcePath)) {
                    if (entry.is_regular_file() && entry.path().filename() == m_fields->m_currentSacrifice) {
                        fs::remove(entry.path());
                        deletedFromSource = true;
                        break; 
                    }
                }

                if (deletedFromSource) {
                    std::string msg = "SACRIFICED: " + m_fields->m_currentSacrifice;
                    Notification::create(msg, NotificationIcon::Error)->show();
                    m_fields->m_currentSacrifice = ""; // Limpiar para el próximo reset
                }
            } catch (const std::exception& e) {
                log::error("Mod 666 Error: {}", e.what());
            }
        }
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        // Al reiniciar, si hubo una muerte válida, buscamos un nuevo sacrificio
        if (m_fields->m_hasDiedThisAttempt || m_fields->m_currentSacrifice.empty()) {
            m_fields->m_hasDiedThisAttempt = false;
            m_fields->m_timeInLevel = 0.0f;
            selectNewSacrifice();
        }
    }
};
