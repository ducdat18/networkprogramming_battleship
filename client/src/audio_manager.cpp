#include "audio_manager.h"
#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <cmath>
#include <fstream>

// Musical note frequencies (Hz) - Standard tuning A4=440Hz
#define NOTE_C4  261.63
#define NOTE_D4  293.66
#define NOTE_E4  329.63
#define NOTE_F4  349.23
#define NOTE_G4  392.00
#define NOTE_A4  440.00
#define NOTE_B4  493.88
#define NOTE_C5  523.25
#define NOTE_D5  587.33
#define NOTE_E5  659.25
#define NOTE_F5  698.46
#define NOTE_G5  783.99

// Helper function to play a musical note at a specific frequency
void AudioManager::playNote(int frequency, int duration_ms) {
    if (!sound_enabled_) return;

    // Try using beep command with frequency (if available)
    std::string cmd = "beep -f " + std::to_string(frequency) +
                     " -l " + std::to_string(duration_ms) + " 2>/dev/null &";
    int result = system(cmd.c_str());

    // If beep command doesn't work (WSL2), use fallback
    if (result != 0) {
        // Fallback: use simple tone via speaker-test or just print
        // In WSL2, audio might not work, so we just maintain timing
        usleep(duration_ms * 1000);
    }
}

void AudioManager::playTone(int freq, int duration_ms) {
    playNote(freq, duration_ms);
}

// Play a simple beep
void AudioManager::playBeep() {
    if (!sound_enabled_) return;
    playNote(440, 100);  // A4 note, 100ms
}

void AudioManager::playDoubleBeep() {
    if (!sound_enabled_) return;
    playNote(NOTE_E5, 100);
    usleep(50000);
    playNote(NOTE_E5, 100);
}

void AudioManager::playTripleBeep() {
    if (!sound_enabled_) return;
    for (int i = 0; i < 3; i++) {
        playNote(NOTE_A4, 80);
        if (i < 2) usleep(50000);
    }
}

void AudioManager::playSuccessSound() {
    if (!sound_enabled_) return;
    // Ascending melody - C D E G (happy sound)
    playNote(NOTE_C5, 80);
    usleep(30000);
    playNote(NOTE_D5, 80);
    usleep(30000);
    playNote(NOTE_E5, 80);
    usleep(30000);
    playNote(NOTE_G5, 150);
}

void AudioManager::playErrorSound() {
    if (!sound_enabled_) return;
    // Dissonant low notes for error
    playNote(NOTE_F4, 120);
    usleep(100000);
    playNote(NOTE_E4, 200);
}

void AudioManager::playShotSound() {
    if (!sound_enabled_) return;
    // Quick descending whoosh sound
    playNote(NOTE_G5, 50);
    playNote(NOTE_E5, 50);
}

void AudioManager::playHitSound() {
    if (!sound_enabled_) return;
    // Explosion sound - quick burst
    playNote(NOTE_C4, 80);
    usleep(20000);
    playNote(NOTE_E4, 100);
}

void AudioManager::playSunkSound() {
    if (!sound_enabled_) return;
    // Descending melody (ship sinking)
    playNote(NOTE_G5, 100);
    usleep(80000);
    playNote(NOTE_E5, 100);
    usleep(80000);
    playNote(NOTE_D5, 100);
    usleep(80000);
    playNote(NOTE_C5, 100);
    usleep(80000);
    playNote(NOTE_C4, 200);  // Deep final note
}

void AudioManager::playBackgroundMusic() {
    stopMusic();  // Stop any existing music
    music_running_ = true;

    if (music_thread_.joinable()) {
        music_thread_.join();
    }

    music_thread_ = std::thread([this]() {
        backgroundMusicLoop();
    });
}

void AudioManager::playGameMusic() {
    stopMusic();
    music_running_ = true;

    if (music_thread_.joinable()) {
        music_thread_.join();
    }

    music_thread_ = std::thread([this]() {
        gameMusicLoop();
    });
}

void AudioManager::playVictoryMusic() {
    stopMusic();
    music_running_ = true;

    if (music_thread_.joinable()) {
        music_thread_.join();
    }

    music_thread_ = std::thread([this]() {
        victoryMusicLoop();
    });
}

void AudioManager::playDefeatMusic() {
    stopMusic();
    music_running_ = true;

    if (music_thread_.joinable()) {
        music_thread_.join();
    }

    music_thread_ = std::thread([this]() {
        defeatMusicLoop();
    });
}

void AudioManager::stopMusic() {
    music_running_ = false;
    if (music_thread_.joinable()) {
        music_thread_.join();
    }
}

// Funky background music loop (upbeat lobby music)
void AudioManager::backgroundMusicLoop() {
    std::cout << "[AUDIO] 🎵 Starting FUNKY lobby music..." << std::endl;

    // Funky groove pattern - C major funk bass line with melody
    while (music_running_ && music_enabled_) {
        if (!music_running_) break;

        // BAR 1: Funky bass riff - C E G E
        playNote(NOTE_C4, 200);  // C bass
        usleep(100000);
        playNote(NOTE_E4, 150);  // E
        usleep(50000);
        playNote(NOTE_G4, 150);  // G
        usleep(50000);
        playNote(NOTE_E4, 120);  // E back
        usleep(180000);

        // Ghost note
        playNote(NOTE_G4, 60);
        usleep(100000);

        if (!music_running_) break;

        // BAR 2: Syncopated melody - G A C A G
        playNote(NOTE_G4, 180);  // Bass hit
        usleep(70000);
        playNote(NOTE_A4, 120);  // Hi-hat
        usleep(50000);
        playNote(NOTE_C5, 120);  // Snare
        usleep(50000);
        playNote(NOTE_A4, 150);  // Bass
        usleep(50000);
        playNote(NOTE_G4, 100);  // Hi-hat
        usleep(200000);

        if (!music_running_) break;

        // BAR 3: Build up - E F G A
        playNote(NOTE_E4, 150);
        usleep(100000);
        playNote(NOTE_F4, 150);
        usleep(100000);
        playNote(NOTE_G4, 150);
        usleep(100000);
        playNote(NOTE_A4, 150);
        usleep(100000);

        if (!music_running_) break;

        // BAR 4: Drop - Big C chord resolution
        playNote(NOTE_C5, 300);  // BASS DROP (high C)
        usleep(100000);
        playNote(NOTE_G4, 150);
        usleep(50000);
        playNote(NOTE_E4, 150);
        usleep(50000);
        playNote(NOTE_C4, 200);  // Back to bass

        // Short break before loop
        usleep(300000);
    }

    std::cout << "[AUDIO] Funky music stopped." << std::endl;
}

// Intense game music (faster tempo)
void AudioManager::gameMusicLoop() {
    std::cout << "[AUDIO] ⚔️ Starting intense battle music..." << std::endl;

    // Fast-paced battle music in E minor (tension and drama)
    while (music_running_ && music_enabled_) {
        if (!music_running_) break;

        // Battle theme - driving rhythm E minor
        // Pattern 1: E E E G | A G E E
        playNote(NOTE_E4, 120);
        usleep(30000);
        playNote(NOTE_E4, 120);
        usleep(30000);
        playNote(NOTE_E4, 150);
        usleep(50000);
        playNote(NOTE_G4, 120);
        usleep(50000);

        playNote(NOTE_A4, 150);
        usleep(50000);
        playNote(NOTE_G4, 120);
        usleep(30000);
        playNote(NOTE_E4, 120);
        usleep(30000);
        playNote(NOTE_E4, 150);
        usleep(100000);

        if (!music_running_) break;

        // Pattern 2: F G A B | C B A G
        playNote(NOTE_F4, 120);
        usleep(30000);
        playNote(NOTE_G4, 120);
        usleep(30000);
        playNote(NOTE_A4, 120);
        usleep(30000);
        playNote(NOTE_B4, 150);
        usleep(50000);

        playNote(NOTE_C5, 150);
        usleep(50000);
        playNote(NOTE_B4, 120);
        usleep(30000);
        playNote(NOTE_A4, 120);
        usleep(30000);
        playNote(NOTE_G4, 150);
        usleep(100000);

        if (!music_running_) break;

        // Quick dramatic ascent
        playNote(NOTE_E4, 100);
        usleep(20000);
        playNote(NOTE_G4, 100);
        usleep(20000);
        playNote(NOTE_B4, 100);
        usleep(20000);
        playNote(NOTE_E5, 200);  // High E - tension peak
        usleep(150000);

        // Resolution back to bass
        playNote(NOTE_E4, 200);
        usleep(200000);
    }

    std::cout << "[AUDIO] Battle music stopped." << std::endl;
}

// Victory fanfare
void AudioManager::victoryMusicLoop() {
    std::cout << "[AUDIO] 🎉 Playing VICTORY fanfare..." << std::endl;

    if (!music_enabled_) return;

    // Triumphant fanfare in C major
    // Opening: C C C - E E E - G G G (building excitement)
    playNote(NOTE_C5, 180);
    usleep(50000);
    playNote(NOTE_C5, 180);
    usleep(50000);
    playNote(NOTE_C5, 250);
    usleep(200000);

    playNote(NOTE_E5, 180);
    usleep(50000);
    playNote(NOTE_E5, 180);
    usleep(50000);
    playNote(NOTE_E5, 250);
    usleep(200000);

    playNote(NOTE_G5, 180);
    usleep(50000);
    playNote(NOTE_G5, 180);
    usleep(50000);
    playNote(NOTE_G5, 300);
    usleep(300000);

    // Victory melody: C D E F G A B C (triumphant scale)
    playNote(NOTE_C5, 120);
    usleep(30000);
    playNote(NOTE_D5, 120);
    usleep(30000);
    playNote(NOTE_E5, 120);
    usleep(30000);
    playNote(NOTE_F5, 120);
    usleep(30000);
    playNote(NOTE_G5, 150);
    usleep(50000);
    playNote(NOTE_A4, 150);
    usleep(50000);
    playNote(NOTE_B4, 150);
    usleep(50000);
    playNote(NOTE_C5, 400);  // Final triumphant high C
    usleep(300000);

    // Final flourish - rapid upward arpeggio
    playNote(NOTE_C5, 80);
    usleep(20000);
    playNote(NOTE_E5, 80);
    usleep(20000);
    playNote(NOTE_G5, 80);
    usleep(20000);
    playNote(NOTE_C5, 80);
    usleep(20000);
    playNote(NOTE_E5, 80);
    usleep(20000);
    playNote(NOTE_G5, 80);
    usleep(20000);
    playNote(NOTE_C5, 500);  // FINAL BIG NOTE!

    music_running_ = false;
    std::cout << "[AUDIO] 🏆 Victory fanfare complete!" << std::endl;
}

// Defeat sound
void AudioManager::defeatMusicLoop() {
    std::cout << "[AUDIO] 💔 Playing defeat music..." << std::endl;

    if (!music_enabled_) return;

    // Somber, sad melody in E minor (descending)
    // Opening: Slow, mournful tones
    playNote(NOTE_E5, 350);  // High sad note
    usleep(250000);
    playNote(NOTE_D5, 350);
    usleep(250000);
    playNote(NOTE_C5, 400);
    usleep(350000);

    // Descending sorrow: B A G F E
    playNote(NOTE_B4, 300);
    usleep(200000);
    playNote(NOTE_A4, 300);
    usleep(200000);
    playNote(NOTE_G4, 350);
    usleep(250000);
    playNote(NOTE_F4, 350);
    usleep(250000);
    playNote(NOTE_E4, 450);  // Lower, darker
    usleep(400000);

    // Final descending phrase (giving up)
    playNote(NOTE_D4, 400);
    usleep(300000);
    playNote(NOTE_C4, 500);  // Deep, final low note
    usleep(500000);

    // Very long, sad final note
    playNote(NOTE_E4, 800);  // Long sad ending

    music_running_ = false;
    std::cout << "[AUDIO] 😢 Defeat music complete." << std::endl;
}
