#ifndef _LIB_SOFT_SPI_H_
#define _LIB_SOFT_SPI_H_

#include <linux/types.h>

#define DFLT_CS_PIN   67
#define DFLT_SCK_PIN  66
#define DFLT_MOSI_PIN 69
#define DFLT_MISO_PIN 68

int spi_mosi_request(unsigned long mosi_pin);

int spi_miso_request(unsigned long miso_pin);

int spi_sck_request(unsigned long sck_pin);

int spi_cs_request(unsigned long cs_pin);

void spi_mosi_free(void);

void spi_miso_free(void);

void spi_sck_free(void);

void spi_cs_free(void);

void soft_spi_init(void);

void soft_spi_cs_set(uint val);

int soft_spi_read(uint8_t* data, const size_t len);

int soft_spi_write(uint8_t* data, const size_t len);


#endif
