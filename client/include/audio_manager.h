#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <thread>
#include <atomic>
#include <string>
#include <iostream>
#include <unistd.h>

class AudioManager {
public:
    static AudioManager& getInstance() {
        static AudioManager instance;
        return instance;
    }

    // Music control
    void playBackgroundMusic();
    void playGameMusic();
    void playVictoryMusic();
    void playDefeatMusic();
    void stopMusic();

    // Sound effects
    void playBeep();
    void playDoubleBeep();
    void playTripleBeep();
    void playSuccessSound();
    void playErrorSound();
    void playShotSound();
    void playHitSound();
    void playSunkSound();

    // Volume control
    void setMusicEnabled(bool enabled) { music_enabled_ = enabled; }
    void setSoundEnabled(bool enabled) { sound_enabled_ = enabled; }
    bool isMusicEnabled() const { return music_enabled_; }
    bool isSoundEnabled() const { return sound_enabled_; }

private:
    AudioManager() : music_enabled_(true), sound_enabled_(true), music_running_(false) {}
    ~AudioManager() { stopMusic(); }

    // Prevent copying
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    // Music thread functions
    void backgroundMusicLoop();
    void gameMusicLoop();
    void victoryMusicLoop();
    void defeatMusicLoop();

    // Helper functions
    void playNote(int frequency, int duration_ms);
    void playTone(int freq, int duration_ms);

    // State
    std::atomic<bool> music_enabled_;
    std::atomic<bool> sound_enabled_;
    std::atomic<bool> music_running_;
    std::thread music_thread_;
};

#endif // AUDIO_MANAGER_H
