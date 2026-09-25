/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Ed25519.h"

#include <monocypher-ed25519.h>

namespace ed25519 {

bool Verify(
	const std::uint8_t* message,
	std::size_t messageSize,
	const std::uint8_t signature[SIGNATURE_SIZE],
	const std::uint8_t publicKey[PUBLIC_KEY_SIZE]
)
{
	if ((message == nullptr && messageSize != 0) || signature == nullptr || publicKey == nullptr)
		return false;

	return crypto_ed25519_check(signature, publicKey, message, messageSize) == 0;
}

} // namespace ed25519
