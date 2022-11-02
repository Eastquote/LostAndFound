#include "AudioManager.h"

#include "GameWorld.h"
#include "Engine/MathEasings.h"
#include "Engine/Actor.h"
#include <SFML/Audio.hpp>
#include <iostream>

namespace
{
	const std::vector<std::string> s_soundNameList = {
			"BallEnter",
			"BallExit",
			"BeamFire",
			"Blank",
			"CreatureHit",
			"CreatureHit2",
			"Die",
			"Door",
			"EnergyPickup",
			"EnergyPickupLg",
			"Explosion",
			"Footstep00",
			"Footstep01",
			"Footstep02",
			"Footstep03",
			"Footstep04",
			"Footstep05",
			"GroundHit",
			"Hurt",
			"Jump",
			"MissileFire",
			"MissileFireLg",
			"MissilePickup",
			"Pause",
			"WallHit",
			"WallHit2"
	};
}

//--- AUDIO MANAGER CODE ---//

std::shared_ptr<AudioManager> AudioManager::s_audioManager;

std::shared_ptr<AudioManager> AudioManager::Get() {
	if(!s_audioManager) {
		s_audioManager = std::shared_ptr<AudioManager>(new AudioManager());
	}
	return s_audioManager;
}
AudioManager::AudioManager() {
	for(auto soundName : s_soundNameList) {
		FindOrAddSoundBuffer(soundName);
	}
	m_music = std::unique_ptr<sf::Music>(new sf::Music());
}
void AudioManager::Update() {
	if(!m_fadeTask.IsDone()) {
		m_fadeTask.Resume();
	}
}
void AudioManager::PlaySound(std::string in_soundName, float in_startTime, float in_volume) {
	auto buffer = FindOrAddSoundBuffer(in_soundName);
	auto foundSoundInstance = std::find_if(	m_soundInstances.begin(), m_soundInstances.end(), 
											[buffer](std::shared_ptr<SoundInstance> in_bsp) {
		return (in_bsp->buffer == buffer);
	});
	sf::Time startTime = sf::seconds(in_startTime);
	if(foundSoundInstance != m_soundInstances.end()) {
		foundSoundInstance->get()->sound->setPlayingOffset(startTime);
		foundSoundInstance->get()->sound->setVolume(in_volume * 100.0f);
		foundSoundInstance->get()->sound->play();
	}
	else {
		auto soundInstance = std::make_shared<SoundInstance>();
		soundInstance->buffer = buffer;
		soundInstance->sound = std::make_unique<sf::Sound>();
		soundInstance->sound->setBuffer(*soundInstance->buffer);
		soundInstance->sound->setPlayingOffset(startTime);
		soundInstance->sound->setVolume(in_volume * 100.0f);
		soundInstance->sound->play();
		m_soundInstances.push_back(soundInstance);
	}
}
bool AudioManager::IsSoundPlaying(std::string in_soundName) {
	auto buffer = FindOrAddSoundBuffer(in_soundName);
	auto foundSoundInstance = std::find_if(	m_soundInstances.begin(), m_soundInstances.end(), 
											[buffer](std::shared_ptr<SoundInstance> in_bsp) {
		return (in_bsp->buffer == buffer);
	});
	if(foundSoundInstance != m_soundInstances.end()) {
		const auto& sound = foundSoundInstance->get()->sound;
		if(sound->getStatus() == sf::SoundSource::Status::Playing) {
			return true;
		}
	}
	return false;
}
void AudioManager::PlayMusic(std::string in_musicName, float in_startTime, float in_volume, bool in_bLoop) {
	if(m_music && m_music->getStatus() == sf::SoundSource::Status::Playing ) {
		m_prevMusicPos = m_music->getPlayingOffset().asSeconds();
		m_prevMusicVol = m_music->getVolume() / 100;
		m_prevMusicName = m_currentMusicName;
	}
	m_currentMusicName = in_musicName;
	auto musicFileName = "data/music/" + in_musicName + ".ogg";
	if(!m_music->openFromFile(musicFileName)) {
		throw std::runtime_error(musicFileName + " not found!");
	}
	sf::Time startTime = sf::seconds(in_startTime);
	m_music->setPlayingOffset(startTime);
	m_music->setVolume(in_volume * 100);
	m_music->setLoop(in_bLoop);
	m_music->play();
}
void AudioManager::ResumePrevMusic() {
	PlayMusic(m_prevMusicName, m_prevMusicPos, m_prevMusicVol);
}
WeakTaskHandle AudioManager::FadeMusic(float in_duration, float in_volumeTarget, bool in_bPauseWhenDone) {
	m_fadeTask = FadeMusicTask(in_duration, in_volumeTarget, in_bPauseWhenDone);
	return m_fadeTask;
}
Task<> AudioManager::FadeMusicTask(float in_duration, float in_volumeTarget, bool in_bPauseWhenDone) {
	auto elapsedTime = 0.0f;
	auto world = GameWorld::Get();
	auto currentVolume = m_music->getVolume() / 100;
	auto volumeDelta = in_volumeTarget - currentVolume;
	auto fadeAlpha = 0.0f;
	while(elapsedTime < in_duration) {
		elapsedTime += world->DT();
		fadeAlpha = world->EaseAlpha(elapsedTime, in_duration, Math::EaseInOutSmoothstep);
		m_music->setVolume((currentVolume + (fadeAlpha * volumeDelta)) * 100.0f);
		co_await Suspend();
	}
	m_music->setVolume(in_volumeTarget * 100.0f);
	if(in_bPauseWhenDone) {
		PauseMusic();
	}
}
void AudioManager::PauseMusic() {
	m_music->pause();
}
void AudioManager::ResumeMusic() {
	m_music->play();
}
std::shared_ptr<sf::SoundBuffer> AudioManager::FindOrAddSoundBuffer(std::string in_soundName) {
	auto filename = "data/sounds/" + in_soundName + ".wav";
	auto foundBuffer = m_soundBufferCache.find(in_soundName);
	if(foundBuffer != m_soundBufferCache.end()) {
		return  foundBuffer->second;
	}
	auto newBuffer = std::make_shared<sf::SoundBuffer>();
	if(!newBuffer->loadFromFile(filename)) {
		throw std::runtime_error(filename + " not found!");
	}
	m_soundBufferCache.insert({ in_soundName, newBuffer });
	return newBuffer;
}
