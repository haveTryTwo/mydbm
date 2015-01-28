/*
 * This code implements the AUTODIN II polynomial
 * The variable corresponding to the macro argument "crc" should
 * be an unsigned long.
 * Oroginal code  by Spencer Garrett <srg@quick.com>
 */

#include <string>

/**
 * Return a 32-bit CRC of the contents of the buffer.
 */
uint32_t dbm_crc32(const std::string &str);
