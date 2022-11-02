#pragma once

#include "Task.h"
#include <memory>
#include <map>

namespace sf {
	class SoundBuffer;
	class Sound;
	class Music;
}

// Manages the loading, playing, pausing, resuming, fading, and swapping of music
class AudioManager {
public:
	static std::shared_ptr<AudioManager> Get();
	void Update();
	void PlaySound(std::string in_soundName, float in_startTime = 0.0f, float in_volume = 1.0f);
	bool IsSoundPlaying(std::string in_soundName);
	void PlayMusic(std::string in_musicName, float in_startTime = 0.0f, float in_volume = 1.0f, bool in_bLoop = true);
	void ResumePrevMusic();
	void PauseMusic();
	void ResumeMusic();
	WeakTaskHandle FadeMusic(float in_duration = 1.0f, float in_volumeTarget = 0.0f, bool in_bPauseWhenDone = false);

private:
	using SoundBufferCache = std::map<std::string, std::shared_ptr<sf::SoundBuffer>>;
	struct SoundInstance {
		std::shared_ptr<sf::SoundBuffer> buffer;
		std::unique_ptr<sf::Sound> sound;
	};

	// AudioManager is a singleton, so we hide the constructors
	AudioManager();
	AudioManager(const AudioManager&);
	AudioManager& operator=(const AudioManager&);
	static std::shared_ptr<AudioManager> s_audioManager;

	Task<> FadeMusicTask(float in_duration, float in_volumeTarget, bool in_bPauseWhenDone);
	std::shared_ptr<sf::SoundBuffer> FindOrAddSoundBuffer(std::string in_soundName);

	SoundBufferCache m_soundBufferCache;
	std::vector<std::shared_ptr<SoundInstance>> m_soundInstances;
	std::unique_ptr<sf::Music> m_music;
	std::string m_prevMusicName;
	std::string m_currentMusicName;
	float m_prevMusicPos = 0.0f;
	float m_prevMusicVol = 0.025f;
	Task<> m_fadeTask;
};