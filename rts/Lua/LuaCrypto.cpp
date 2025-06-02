/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <stdexcept>

#include "LuaCrypto.h"
#include "base64.h"

#include "LuaInclude.h"
#include "LuaUtils.h"

#ifdef UNUSED
#undef UNUSED
#endif

// random generator access
#if defined(_WIN32)
// * The defined WIN32_NO_STATUS macro disables return code definitions in
// * windows.h, which avoids "macro redefinition" MSVC warnings in ntstatus.h.

#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS
#include <ntstatus.h>
#include <bcrypt.h>
#elif defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__)
#include <sys/random.h>
#elif defined(__OpenBSD__)
#include <unistd.h>
#else
#error "Couldn't identify the OS"
#endif


// secp256k1

#include "secp256k1.h"
#include "secp256k1_ecdh.h"

// tiny-aes-c
#define AES256 1
#include "aes.hpp"

// cryptopp
#include "cryptopp/pch.h"
#include "cryptopp/aes.h"

#include "cryptopp/osrng.h"
using CryptoPP::AutoSeededRandomPool;

using CryptoPP::byte;
#include "cryptopp/cryptlib.h"
using CryptoPP::Exception;

#include "cryptopp/hex.h"
using CryptoPP::HexEncoder;
using CryptoPP::HexDecoder;

#include "cryptopp/filters.h"
using CryptoPP::StringSink;
using CryptoPP::StringSource;
using CryptoPP::StreamTransformationFilter;

#include "cryptopp/aes.h"
using CryptoPP::AES;

#include "cryptopp/ccm.h"
using CryptoPP::CBC_Mode;


/******************************************************************************
 * LuaCrypto
 ******************************************************************************/

/* Returns 1 on success, and 0 on failure. */
static int fill_random(unsigned char* data, size_t size) {
#if defined(_WIN32)
	NTSTATUS res = BCryptGenRandom(NULL, data, size, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
	if (res != STATUS_SUCCESS || size > ULONG_MAX) {
		return 0;
	} else {
		return 1;
	}
#elif defined(__linux__) || defined(__FreeBSD__)
/* If `getrandom(2)` is not available you should fallback to /dev/urandom */
	ssize_t res = getrandom(data, size, 0);
	if (res < 0 || (size_t)res != size ) {
		return 0;
	} else {
	return 1;
}
#elif defined(__APPLE__) || defined(__OpenBSD__)
/* If `getentropy(2)` is not available you should fallback to either
* `SecRandomCopyBytes` or /dev/urandom */
int res = getentropy(data, size);
	if (res == 0) {
		return 1;
	} else {
		return 0;
	}
#endif
	return 0;
}

#if defined(_MSC_VER)
// For SecureZeroMemory
#include <Windows.h>
#endif
/* Cleanses memory to prevent leaking sensitive info. Won't be optimized out. */
static void secure_erase(void *ptr, size_t len) {
#if defined(_MSC_VER)
    /* SecureZeroMemory is guaranteed not to be optimized out by MSVC. */
    SecureZeroMemory(ptr, len);
#elif defined(__GNUC__)
    /* We use a memory barrier that scares the compiler away from optimizing out the memset.
     *
     * Quoting Adam Langley <agl@google.com> in commit ad1907fe73334d6c696c8539646c21b11178f20f
     * in BoringSSL (ISC License):
     *    As best as we can tell, this is sufficient to break any optimisations that
     *    might try to eliminate "superfluous" memsets.
     * This method used in memzero_explicit() the Linux kernel, too. Its advantage is that it is
     * pretty efficient, because the compiler can still implement the memset() efficiently,
     * just not remove it entirely. See "Dead Store Elimination (Still) Considered Harmful" by
     * Yang et al. (USENIX Security 2017) for more background.
     */
    memset(ptr, 0, len);
    __asm__ __volatile__("" : : "r"(ptr) : "memory");
#else
    void *(*volatile const volatile_memset)(void *, int, size_t) = memset;
    volatile_memset(ptr, 0, len);
#endif
}

bool LuaCrypto::PushEntries(lua_State* L)
{
	REGISTER_LUA_CFUNC(TestEcdsa);
	REGISTER_LUA_CFUNC(TestEcdh);
	REGISTER_LUA_CFUNC(TestAes);
	REGISTER_LUA_CFUNC(TestTinyAes);

	return true;
}

int LuaCrypto::TestEcdh(lua_State* L)
{
	unsigned char seckey1[32];
	unsigned char seckey2[32];
	unsigned char compressed_pubkey1[33];
	unsigned char compressed_pubkey2[33];
	unsigned char shared_secret1[32];
	unsigned char shared_secret2[32];
	unsigned char randomize[32];
	int return_val;
	size_t len;
	secp256k1_pubkey pubkey1;
	secp256k1_pubkey pubkey2;

	/* Before we can call actual API functions, we need to create a "context". */
	secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
	if (!fill_random(randomize, sizeof(randomize))) {
		printf("Failed to generate randomness\n");
		return EXIT_FAILURE;
	}
	/* Randomizing the context is recommended to protect against side-channel
	* leakage See `secp256k1_context_randomize` in secp256k1.h for more
	* information about it. This should never fail. */
	return_val = secp256k1_context_randomize(ctx, randomize);
	assert(return_val);

	/*** Key Generation ***/
	if (!fill_random(seckey1, sizeof(seckey1)) || !fill_random(seckey2, sizeof(seckey2))) {
		printf("Failed to generate randomness\n");
		return EXIT_FAILURE;
	}
	/* If the secret key is zero or out of range (greater than secp256k1's
	* order), we fail. Note that the probability of this occurring is negligible
	* with a properly functioning random number generator. */
	if (!secp256k1_ec_seckey_verify(ctx, seckey1) || !secp256k1_ec_seckey_verify(ctx, seckey2)) {
		printf("Generated secret key is invalid. This indicates an issue with the random number generator.\n");
		return EXIT_FAILURE;
	}

	/* Public key creation using a valid context with a verified secret key should never fail */
	return_val = secp256k1_ec_pubkey_create(ctx, &pubkey1, seckey1);
	assert(return_val);
	return_val = secp256k1_ec_pubkey_create(ctx, &pubkey2, seckey2);
	assert(return_val);

	/* Serialize pubkey1 in a compressed form (33 bytes), should always return 1 */
	len = sizeof(compressed_pubkey1);
	return_val = secp256k1_ec_pubkey_serialize(ctx, compressed_pubkey1, &len, &pubkey1, SECP256K1_EC_COMPRESSED);
	assert(return_val);
	/* Should be the same size as the size of the output, because we passed a 33 byte array. */
	assert(len == sizeof(compressed_pubkey1));

	/* Serialize pubkey2 in a compressed form (33 bytes) */
	len = sizeof(compressed_pubkey2);
	return_val = secp256k1_ec_pubkey_serialize(ctx, compressed_pubkey2, &len, &pubkey2, SECP256K1_EC_COMPRESSED);
	assert(return_val);
	/* Should be the same size as the size of the output, because we passed a 33 byte array. */
	assert(len == sizeof(compressed_pubkey2));

	/*** Creating the shared secret ***/

	/* Perform ECDH with seckey1 and pubkey2. Should never fail with a verified
	* seckey and valid pubkey */
	return_val = secp256k1_ecdh(ctx, shared_secret1, &pubkey2, seckey1, NULL, NULL);
	assert(return_val);

	/* Perform ECDH with seckey2 and pubkey1. Should never fail with a verified
	* seckey and valid pubkey */
	return_val = secp256k1_ecdh(ctx, shared_secret2, &pubkey1, seckey2, NULL, NULL);
	assert(return_val);

	/* Both parties should end up with the same shared secret */
	return_val = memcmp(shared_secret1, shared_secret2, sizeof(shared_secret1));
	assert(return_val == 0);

	/*printf("Secret Key1: ");
	print_hex(seckey1, sizeof(seckey1));
	printf("Compressed Pubkey1: ");
	print_hex(compressed_pubkey1, sizeof(compressed_pubkey1));
	printf("\nSecret Key2: ");
	print_hex(seckey2, sizeof(seckey2));
	printf("Compressed Pubkey2: ");
	print_hex(compressed_pubkey2, sizeof(compressed_pubkey2));
	printf("\nShared Secret: ");
	print_hex(shared_secret1, sizeof(shared_secret1));*/

	/* This will clear everything from the context and free the memory */
	secp256k1_context_destroy(ctx);

	/* It's best practice to try to clear secrets from memory after using them.
	* This is done because some bugs can allow an attacker to leak memory, for
	* example through "out of bounds" array access (see Heartbleed), or the OS
	* swapping them to disk. Hence, we overwrite the secret key buffer with zeros.
	*
	* Here we are preventing these writes from being optimized out, as any good compiler
	* will remove any writes that aren't used. */
	secure_erase(seckey1, sizeof(seckey1));
	secure_erase(seckey2, sizeof(seckey2));
	secure_erase(shared_secret1, sizeof(shared_secret1));
	secure_erase(shared_secret2, sizeof(shared_secret2));

	auto res = base64_encode(compressed_pubkey1, sizeof(compressed_pubkey1));
	lua_pushstring(L, res.c_str());
	return 1;
}

int LuaCrypto::TestEcdsa(lua_State* L)
{
	unsigned char msg_hash[32] = {
		0x31, 0x5F, 0x5B, 0xDB, 0x76, 0xD0, 0x78, 0xC4,
		0x3B, 0x8A, 0xC0, 0x06, 0x4E, 0x4A, 0x01, 0x64,
		0x61, 0x2B, 0x1F, 0xCE, 0x77, 0xC8, 0x69, 0x34,
		0x5B, 0xFC, 0x94, 0xC7, 0x58, 0x94, 0xED, 0xD3,
	};
	unsigned char seckey[32];
	unsigned char randomize[32];
	unsigned char compressed_pubkey[33];
	unsigned char serialized_signature[64];
	size_t len;
	int is_signature_valid, is_signature_valid2;
	int return_val;
	secp256k1_pubkey pubkey;
	secp256k1_ecdsa_signature sig;
	/* Before we can call actual API functions, we need to create a "context". */
	secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
	if (!fill_random(randomize, sizeof(randomize))) {
		printf("Failed to generate randomness\n");
		return 0;
	}
	/* Randomizing the context is recommended to protect against side-channel
	* leakage See `secp256k1_context_randomize` in secp256k1.h for more
	* information about it. This should never fail. */
	return_val = secp256k1_context_randomize(ctx, randomize);
	assert(return_val);

	/*** Key Generation ***/
	if (!fill_random(seckey, sizeof(seckey))) {
		printf("Failed to generate randomness\n");
		return 0;
	}
	/* If the secret key is zero or out of range (greater than secp256k1's
	* order), we fail. Note that the probability of this occurring is negligible
	* with a properly functioning random number generator. */
	if (!secp256k1_ec_seckey_verify(ctx, seckey)) {
		printf("Generated secret key is invalid. This indicates an issue with the random number generator.\n");
		return 0;
	}
	return_val = secp256k1_ec_pubkey_create(ctx, &pubkey, seckey);
	assert(return_val);

	/* Serialize the pubkey in a compressed form(33 bytes). Should always return 1. */
	len = sizeof(compressed_pubkey);
	return_val = secp256k1_ec_pubkey_serialize(ctx, compressed_pubkey, &len, &pubkey, SECP256K1_EC_COMPRESSED);
	assert(return_val);
	/* Should be the same size as the size of the output, because we passed a 33 byte array. */
	assert(len == sizeof(compressed_pubkey));

	/*** Signing ***/

	/* Generate an ECDSA signature `noncefp` and `ndata` allows you to pass a
	* custom nonce function, passing `NULL` will use the RFC-6979 safe default.
	* Signing with a valid context, verified secret key
	* and the default nonce function should never fail. */
	return_val = secp256k1_ecdsa_sign(ctx, &sig, msg_hash, seckey, NULL, NULL);
	assert(return_val);

	/* Serialize the signature in a compact form. Should always return 1
	* according to the documentation in secp256k1.h. */
	return_val = secp256k1_ecdsa_signature_serialize_compact(ctx, serialized_signature, &sig);
	assert(return_val);


	/*** Verification ***/

	/* Deserialize the signature. This will return 0 if the signature can't be parsed correctly. */
	if (!secp256k1_ecdsa_signature_parse_compact(ctx, &sig, serialized_signature)) {
		printf("Failed parsing the signature\n");
		return EXIT_FAILURE;
	}

	/* Deserialize the public key. This will return 0 if the public key can't be parsed correctly. */
	if (!secp256k1_ec_pubkey_parse(ctx, &pubkey, compressed_pubkey, sizeof(compressed_pubkey))) {
		printf("Failed parsing the public key\n");
		return EXIT_FAILURE;
	}

	/* Verify a signature. This will return 1 if it's valid and 0 if it's not. */
	is_signature_valid = secp256k1_ecdsa_verify(ctx, &sig, msg_hash, &pubkey);

	/*printf("Is the signature valid? %s\n", is_signature_valid ? "true" : "false");
	printf("Secret Key: ");
	print_hex(seckey, sizeof(seckey));
	printf("Public Key: ");
	print_hex(compressed_pubkey, sizeof(compressed_pubkey));
	printf("Signature: ");
	print_hex(serialized_signature, sizeof(serialized_signature));*/

	/* This will clear everything from the context and free the memory */
	secp256k1_context_destroy(ctx);

	/* Bonus example: if all we need is signature verification (and no key
	generation or signing), we don't need to use a context created via
	secp256k1_context_create(). We can simply use the static (i.e., global)
	context secp256k1_context_static. See its description in
	include/secp256k1.h for details. */
	is_signature_valid2 = secp256k1_ecdsa_verify(secp256k1_context_static,
						 &sig, msg_hash, &pubkey);
	assert(is_signature_valid2 == is_signature_valid);

	/* It's best practice to try to clear secrets from memory after using them.
	* This is done because some bugs can allow an attacker to leak memory, for
	* example through "out of bounds" array access (see Heartbleed), or the OS
	* swapping them to disk. Hence, we overwrite the secret key buffer with zeros.
	*
	* Here we are preventing these writes from being optimized out, as any good compiler
	* will remove any writes that aren't used. */
	secure_erase(seckey, sizeof(seckey));
	auto res = base64_encode(compressed_pubkey, sizeof(compressed_pubkey));
	lua_pushstring(L, res.c_str());
	auto res2 = base64_encode(serialized_signature, sizeof(compressed_pubkey));
	lua_pushstring(L, res2.c_str());
	lua_pushboolean(L, is_signature_valid);
	return 3;
}

int LuaCrypto::TestTinyAes(lua_State* L)
{
#if defined(AES256)
    uint8_t key[] = { 0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe, 0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81,
                      0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61, 0x08, 0xd7, 0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4 };
    uint8_t out[] = { 0xf5, 0x8c, 0x4c, 0x04, 0xd6, 0xe5, 0xf1, 0xba, 0x77, 0x9e, 0xab, 0xfb, 0x5f, 0x7b, 0xfb, 0xd6,
                      0x9c, 0xfc, 0x4e, 0x96, 0x7e, 0xdb, 0x80, 0x8d, 0x67, 0x9f, 0x77, 0x7b, 0xc6, 0x70, 0x2c, 0x7d,
                      0x39, 0xf2, 0x33, 0x69, 0xa9, 0xd9, 0xba, 0xcf, 0xa5, 0x30, 0xe2, 0x63, 0x04, 0x23, 0x14, 0x61,
                      0xb2, 0xeb, 0x05, 0xe2, 0xc3, 0x9b, 0xe9, 0xfc, 0xda, 0x6c, 0x19, 0x07, 0x8c, 0x6a, 0x9d, 0x1b };
	lua_pushinteger(L, 1);
#elif defined(AES192)
    uint8_t key[] = { 0x8e, 0x73, 0xb0, 0xf7, 0xda, 0x0e, 0x64, 0x52, 0xc8, 0x10, 0xf3, 0x2b, 0x80, 0x90, 0x79, 0xe5, 0x62, 0xf8, 0xea, 0xd2, 0x52, 0x2c, 0x6b, 0x7b };
    uint8_t out[] = { 0x4f, 0x02, 0x1d, 0xb2, 0x43, 0xbc, 0x63, 0x3d, 0x71, 0x78, 0x18, 0x3a, 0x9f, 0xa0, 0x71, 0xe8,
                      0xb4, 0xd9, 0xad, 0xa9, 0xad, 0x7d, 0xed, 0xf4, 0xe5, 0xe7, 0x38, 0x76, 0x3f, 0x69, 0x14, 0x5a,
                      0x57, 0x1b, 0x24, 0x20, 0x12, 0xfb, 0x7a, 0xe0, 0x7f, 0xa9, 0xba, 0xac, 0x3d, 0xf1, 0x02, 0xe0,
                      0x08, 0xb0, 0xe2, 0x79, 0x88, 0x59, 0x88, 0x81, 0xd9, 0x20, 0xa9, 0xe6, 0x4f, 0x56, 0x15, 0xcd };
	lua_pushinteger(L, 2);
#elif defined(AES128)
    uint8_t key[] = { 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c };
    uint8_t out[] = { 0x76, 0x49, 0xab, 0xac, 0x81, 0x19, 0xb2, 0x46, 0xce, 0xe9, 0x8e, 0x9b, 0x12, 0xe9, 0x19, 0x7d,
                      0x50, 0x86, 0xcb, 0x9b, 0x50, 0x72, 0x19, 0xee, 0x95, 0xdb, 0x11, 0x3a, 0x91, 0x76, 0x78, 0xb2,
                      0x73, 0xbe, 0xd6, 0xb8, 0xe3, 0xc1, 0x74, 0x3b, 0x71, 0x16, 0xe6, 0x9e, 0x22, 0x22, 0x95, 0x16,
                      0x3f, 0xf1, 0xca, 0xa1, 0x68, 0x1f, 0xac, 0x09, 0x12, 0x0e, 0xca, 0x30, 0x75, 0x86, 0xe1, 0xa7 };
	lua_pushinteger(L, 3);
#endif
    uint8_t iv[]  = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };
    uint8_t in[]  = { 0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
                      0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c, 0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51,
                      0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11, 0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef,
                      0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17, 0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10 };
    struct AES_ctx ctx;

    AES_init_ctx_iv(&ctx, key, iv);
    AES_CBC_encrypt_buffer(&ctx, in, 64);

    printf("CBC encrypt: ");

    lua_pushstring(L, base64_encode(in, sizeof(in)).c_str());
    lua_pushstring(L, base64_encode(out, sizeof(out)).c_str());
    if (0 == memcmp((char*) out, (char*) in, 64)) {
	lua_pushboolean(L, true);
    } else {
	lua_pushboolean(L, false);
    }
    return 4;
}

int LuaCrypto::TestAes(lua_State* L)
{
	AutoSeededRandomPool prng;

	byte key[AES::DEFAULT_KEYLENGTH];
	prng.GenerateBlock(key, sizeof(key));

	byte iv[AES::BLOCKSIZE];
	prng.GenerateBlock(iv, sizeof(iv));

	std::string plain = "CBC Mode Test";
	std::string cipher, encoded, recovered;

	/*********************************\
	\*********************************/

	// Pretty print key
	encoded.clear();
	StringSource(key, sizeof(key), true,
		new HexEncoder(
			new StringSink(encoded)
		) // HexEncoder
	); // StringSource

	// Pretty print iv
	encoded.clear();
	StringSource(iv, sizeof(iv), true,
		new HexEncoder(
			new StringSink(encoded)
		) // HexEncoder
	); // StringSource

	/*********************************\
	\*********************************/

	try
	{
		CBC_Mode< AES >::Encryption e;
		e.SetKeyWithIV(key, sizeof(key), iv);

		// The StreamTransformationFilter removes
		//  padding as required.
		StringSource s(plain, true,
			new StreamTransformationFilter(e,
				new StringSink(cipher)
			) // StreamTransformationFilter
		); // StringSource
	}
	catch (const CryptoPP::Exception& e)
	{
		lua_pushboolean(L, false);
		lua_pushstring(L, e.what());
		return 2;
	}

	/*********************************\
	\*********************************/

	// Pretty print
	encoded.clear();
	StringSource(cipher, true,
		new HexEncoder(
			new StringSink(encoded)
		) // HexEncoder
	); // StringSource

	/*********************************\
	\*********************************/

	try
	{
		CBC_Mode< AES >::Decryption d;
		d.SetKeyWithIV(key, sizeof(key), iv);

		// The StreamTransformationFilter removes
		//  padding as required.
		StringSource s(cipher, true,
			new StreamTransformationFilter(d,
				new StringSink(recovered)
			) // StreamTransformationFilter
		); // StringSource
	}
	catch (const CryptoPP::Exception& e)
	{
		lua_pushboolean(L, false);
		lua_pushstring(L, e.what());
		return 2;
	}

	/*********************************\
	\*********************************/

	lua_pushboolean(L, true);
	lua_pushstring(L, encoded.c_str());
	return 2;
}
