#pragma once

constexpr uint32_t Sig2Int(const char (&key)[5]) {
	return (uint32_t) (uint8_t) key[0] |
	       (uint32_t) (uint8_t) key[1] << 8 |
	       (uint32_t) (uint8_t) key[2] << 16 |
	       (uint32_t) (uint8_t) key[3] << 24;
}
