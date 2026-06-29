
# only one of the two targets needs this
remove_definitions(-DNO_SOUND)

# Dummy Sound implementation
set(noSoundSources
		IAudioChannel.cpp
		ISound.cpp
		Null/SoundChannels.cpp
		Null/NullSound.cpp
	)

add_library(no-sound STATIC EXCLUDE_FROM_ALL ${noSoundSources})
set_target_properties(no-sound PROPERTIES COMPILE_FLAGS "-DNO_SOUND")
target_link_libraries(no-sound Tracy::TracyClient)

# Define default sound implementation
if    (NO_SOUND)
	set(sound-impl no-sound PARENT_SCOPE)
else  (NO_SOUND)
	set(sound-impl sound PARENT_SCOPE)
endif (NO_SOUND)


# Real Sound implementation
if    (NOT NO_SOUND)
	set(soundSources
			ISound.cpp
			IAudioChannel.cpp
			Null/NullSound.cpp
			OpenAL/ALShared.cpp
			OpenAL/EFX.cpp
			OpenAL/EFXfuncs.cpp
			OpenAL/EFXPresets.cpp
			OpenAL/AudioChannel.cpp
			OpenAL/OggDecoder.cpp
			OpenAL/Mp3Decoder.cpp
			OpenAL/MusicStream.cpp
			OpenAL/Sound.cpp
			OpenAL/SoundChannels.cpp
			OpenAL/SoundBuffer.cpp
			OpenAL/SoundItem.cpp
			OpenAL/SoundSource.cpp
			OpenAL/VorbisShared.cpp
		)

	if    (APPLE)
		# macOS' system OpenAL.framework is frozen at a pre-EFX version and does
		# not ship efx.h / alext.h, so the EFX sound code cannot build (nor would
		# it work) against it. Prefer openal-soft, which implements EFX. Homebrew
		# installs it keg-only, so locate it explicitly and search it ahead of
		# system frameworks.
		find_program(BREW_COMMAND brew)
		if    (BREW_COMMAND)
			execute_process(
				COMMAND ${BREW_COMMAND} --prefix openal-soft
				OUTPUT_VARIABLE OPENAL_SOFT_PREFIX
				OUTPUT_STRIP_TRAILING_WHITESPACE
				ERROR_QUIET)
			if    (OPENAL_SOFT_PREFIX AND IS_DIRECTORY "${OPENAL_SOFT_PREFIX}")
				list(PREPEND CMAKE_PREFIX_PATH "${OPENAL_SOFT_PREFIX}")
				set(CMAKE_FIND_FRAMEWORK LAST)
			endif ()
		endif (BREW_COMMAND)
	endif (APPLE)

	find_package_static(OpenAL 1.18.2 REQUIRED)
	find_package_static(OggVorbis 1.3.4 REQUIRED)

	find_package(SDL2 MODULE REQUIRED)

	include_directories(${CMAKE_SOURCE_DIR}/include/)
	include_directories(${CMAKE_SOURCE_DIR}/include/AL)

	add_library(sound STATIC EXCLUDE_FROM_ALL ${soundSources})
	target_link_libraries(sound SDL2::SDL2)
	target_link_libraries(sound vorbis::vorbisfile vorbis::vorbis Ogg::ogg)
	target_link_libraries(sound OpenAL::OpenAL)
	target_link_libraries(sound Tracy::TracyClient)
	target_link_libraries(sound nowide::nowide)
endif (NOT NO_SOUND)
