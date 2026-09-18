/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>

#include <catch_amalgamated.hpp>

#include "System/Crypto/Ed25519.h"

namespace {

template<std::size_t Size>
std::array<std::uint8_t, Size> FromHex(std::string_view hex)
{
	REQUIRE(hex.size() == Size * 2);
	std::array<std::uint8_t, Size> bytes;

	for (std::size_t i = 0; i < Size; ++i) {
		auto nibble = [](char value) {
			return static_cast<std::uint8_t>(value <= '9' ? value - '0' : value - 'a' + 10);
		};
		bytes[i] = static_cast<std::uint8_t>((nibble(hex[i * 2]) << 4) | nibble(hex[i * 2 + 1]));
	}

	return bytes;
}

} // namespace

TEST_CASE("Ed25519 RFC 8032 vectors")
{
	SECTION("empty message") {
		const auto publicKey = FromHex<32>("d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a");
		auto signature = FromHex<64>(
			"e5564300c360ac729086e2cc806e828a84877f1eb8e5d974d873e06522490155"
			"5fb8821590a33bacc61e39701cf9b46bd25bf5f0595bbe24655141438e7a100b"
		);

		CHECK(ed25519::Verify(nullptr, 0, signature.data(), publicKey.data()));
		signature[0] ^= 1;
		CHECK_FALSE(ed25519::Verify(nullptr, 0, signature.data(), publicKey.data()));
	}

	SECTION("one-byte message") {
		const auto publicKey = FromHex<32>("3d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c");
		const auto signature = FromHex<64>(
			"92a009a9f0d4cab8720e820b5f642540a2b27b5416503f8fb3762223ebdb69da"
			"085ac1e43e15996e458f3613d0f11d8c387b2eaeb4302aeeb00d291612bb0c00"
		);
		const std::uint8_t message = 0x72;

		CHECK(ed25519::Verify(&message, 1, signature.data(), publicKey.data()));
		const std::uint8_t alteredMessage = 0x73;
		CHECK_FALSE(ed25519::Verify(&alteredMessage, 1, signature.data(), publicKey.data()));
	}
}

TEST_CASE("Ed25519 rejects non-canonical inputs")
{
	const auto publicKey = FromHex<32>("d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a");
	auto signature = FromHex<64>(
		"e5564300c360ac729086e2cc806e828a84877f1eb8e5d974d873e06522490155"
		"5fb8821590a33bacc61e39701cf9b46bd25bf5f0595bbe24655141438e7a100b"
	);

	std::fill(signature.begin() + 32, signature.end(), 0xff);
	CHECK_FALSE(ed25519::Verify(nullptr, 0, signature.data(), publicKey.data()));
	CHECK_FALSE(ed25519::Verify(nullptr, 1, signature.data(), publicKey.data()));
	CHECK_FALSE(ed25519::Verify(nullptr, 0, nullptr, publicKey.data()));
	CHECK_FALSE(ed25519::Verify(nullptr, 0, signature.data(), nullptr));

	const auto identityForgery = FromHex<64>(
		"5866666666666666666666666666666666666666666666666666666666666666"
		"0100000000000000000000000000000000000000000000000000000000000000"
	);
	const std::array weakKeys = {
		std::string_view("0000000000000000000000000000000000000000000000000000000000000000"),
		std::string_view("0100000000000000000000000000000000000000000000000000000000000000"),
		std::string_view("26e8958fc2b227b045c3f489f2ef98f0d5dfac05d3c63339b13802886d53fc05"),
		std::string_view("c7176a703d4dd84fba3c0b760d10670f2a2053fa2c39ccc64ec7fd7792ac037a"),
		std::string_view("ecffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff7f"),
		std::string_view("edffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff7f"),
		std::string_view("eeffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff7f"),
	};

	for (const std::string_view encodedKey: weakKeys) {
		auto weakKey = FromHex<32>(encodedKey);
		CHECK_FALSE(ed25519::Verify(nullptr, 0, identityForgery.data(), weakKey.data()));
		auto weakSignature = identityForgery;
		std::copy(weakKey.begin(), weakKey.end(), weakSignature.begin());
		CHECK_FALSE(ed25519::Verify(nullptr, 0, weakSignature.data(), publicKey.data()));

		weakKey[31] |= 0x80;
		CHECK_FALSE(ed25519::Verify(nullptr, 0, identityForgery.data(), weakKey.data()));
		std::copy(weakKey.begin(), weakKey.end(), weakSignature.begin());
		CHECK_FALSE(ed25519::Verify(nullptr, 0, weakSignature.data(), publicKey.data()));
	}
}