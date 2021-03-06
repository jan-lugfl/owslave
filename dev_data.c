
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <util/crc16.h>

#include "dev_data.h"

#define CRC 0  // needs 300 bytes
#define DEFAULT 1 // needs 30 bytes, plus _config data (built by gen_eprom)

/*
 * Layout of configuration blocks:
 *  4 signature 'DevC'
 *  repeat:
 *    1 length =n >0
 *    1 type
 *    n struct (whatever)
 *  1 zero byte (delimiter)
 *  2 CRC
 *
 *  Struct sizes must match exactly.
 */

#if CRC
static uint8_t read_byte(uint16_t &crc, uint8_t pos) {
	uint8_t b = eeprom_read_byte((uint8_t *)EEPROM_POS + pos);
	crc = _crc16_update(crc, b);
	return b;
}
#define read_crc_byte(x,y) read_byte(x,y)
#else
#define read_crc_byte(x,y) read_byte(y)
#endif

static inline uint8_t read_byte(uint8_t pos) {
	return eeprom_read_byte((uint8_t *)EEPROM_POS + pos);
}

static inline void write_byte(uint8_t b, uint8_t pos) {
	eeprom_write_byte((uint8_t *)EEPROM_POS + pos, b);
}

#if !USE_EEPROM
extern const uint8_t _config_start[] __attribute__ ((progmem));
extern const uint8_t _config_end[] __attribute__ ((progmem));
#endif

#if USE_EEPROM
char _do_crc(bool update) // from eeprom; True if CRC matches
{
	static bool crc_checked = false;
	static bool crc_good = false;

	if(!update && crc_checked)
		return crc_good;

	uint16_t crc = ~0;
	uint8_t b, i=0, j;

	for(j=0;j<4;j++) {
		if(read_crc_byte(crc, i++) != pgm_read_byte(_config_start+j)) {
			break;
		}
	}

	while ((j = read_crc_byte(crc, i++)) > 0) {
		read_crc_byte(crc, i++); // type
		while(j--)
			read_crc_byte(crc, i++); // data
	}

	if(update) {
		write_byte(i++, crc & 0xFF);
		write_byte(i++, crc >> 8);
		crc_good = true;
	} else {
		read_crc_byte(crc, i++);
		read_crc_byte(crc, i++);
		crc_good = (crc == 0);
	}
	crc_checked = true;
	return crc_good;
}
#else // !crc
#define _do_crc(x) true
#endif



char _cfg_read(void *data, uint8_t size, ConfigID id) {
	uint8_t off,len;
	uint8_t *d = data;

	cfg_addr(&off,&len,id);
	if((!off) || (size != len)) return 0;

	while(len) {
		*d++ = read_byte(off++);
		len--;
	}
	return 1;
}

#ifdef CFG_EEPROM
char _cfg_write(void *addr, uint8_t size, ConfigID id) {
}
#endif
    
void cfg_addr(uint8_t *addr, uint8_t *size, ConfigID id) {
	uint8_t i=4,len,t;
	while((len = read_byte(i++)) > 0) {
		t = read_byte(i++);
		if (t == id) {
			*addr = i;
			*size = len;
			return;
		}
	}
	*addr = 0;
}

uint8_t cfg_byte(uint8_t addr) {
#ifdef CFG_EEPROM
    return eeprom_read_byte(addr);
#else
    return pgm_read_byte(_config_start+addr);
#endif
}

