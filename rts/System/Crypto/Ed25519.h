/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <cstddef>
#include <cstdint>

namespace ed25519 {

static constexpr std::size_t SIGNATURE_SIZE = 64;
static constexpr std::size_t PUBLIC_KEY_SIZE = 32;

bool Verify(
	const std::uint8_t* message,
	std::size_t messageSize,
	const std::uint8_t signature[SIGNATURE_SIZE],
	const std::uint8_t publicKey[PUBLIC_KEY_SIZE]
);

} // namespace ed25519
