#include "lib_soft_spi.h"

#include <linux/gpio.h>
#include <linux/delay.h>
#include <asm/errno.h>
#include <linux/spinlock.h>

#define _SPI_100KHZ 10

extern spinlock_t wire_lock;

static unsigned _mosi_pin = DFLT_MOSI_PIN;
static unsigned _miso_pin = DFLT_MISO_PIN;
static unsigned _sck_pin = DFLT_SCK_PIN;
static unsigned _cs_pin = DFLT_CS_PIN;
static unsigned _cpol = 0;
static unsigned _cpha = 0;

unsigned data_xchg_flag;
unsigned long t_delay = _SPI_100KHZ;
void (*_spi_delay)(unsigned long secs) = ndelay;

#define SCK_HIGH() gpio_set_value(_sck_pin,1)
#define SCK_LOW() gpio_set_value(_sck_pin,0)

#define CS_LOW() gpio_set_value(_cs_pin,0)

#define CS_HIGH() gpio_set_value(_cs_pin,1)

#define MOSI_SET(val) gpio_set_value(_mosi_pin,(val))

#define MISO_GET() gpio_get_value(_miso_pin)

int spi_mosi_request(unsigned long mosi_pin)
{
    if(gpio_request(mosi_pin, NULL)){
        return -ENOSYS;
    }
    gpio_direction_output(mosi_pin, 1);

    _mosi_pin = mosi_pin;

    return 0;
}

void spi_mosi_free(void)
{
      gpio_free(_mosi_pin);
}

int spi_miso_request(unsigned long miso_pin)
{
    if(gpio_request(miso_pin, NULL)){
        return -ENOSYS;
    }
    gpio_direction_input(miso_pin);
    
    _miso_pin = miso_pin;

    return 0;
}

void spi_miso_free(void)
{
      gpio_free(_miso_pin);
}

int spi_sck_request(unsigned long sck_pin)
{

    if(gpio_request(sck_pin, NULL)){
        return -ENOSYS;
    }
    gpio_direction_output(sck_pin,0);

    _sck_pin = sck_pin;
    
    return 0;
}

void spi_sck_free(void)
{
      gpio_free(_sck_pin);
}

int spi_cs_request(unsigned long cs_pin)
{
    if(gpio_request(cs_pin, NULL)){
        return -ENOSYS;
    }
    gpio_direction_output(cs_pin,1);

    _cs_pin = cs_pin;

    return 0;
}

void spi_cs_free(void)
{
      gpio_free(_cs_pin);
}

void soft_spi_init(void)
{
    if(!_cpol){
        SCK_LOW();
    }
    else{
        SCK_HIGH();
    }

    data_xchg_flag = 0;
}

static void _spi_start(void)
{
    if(!_cpol){
        SCK_LOW();
    }
    else{
        SCK_HIGH();
    }

    CS_LOW();
    _spi_delay(t_delay);
}

static void _spi_end(void)
{
    if(!_cpol){
        SCK_LOW();
    }
    else{
        SCK_HIGH();
    }

    CS_HIGH();
}

static int _spi_bit_read_write(int bit)
{
    int res = 0;

    if(!_cpol && !_cpha){
        SCK_LOW();
        _spi_delay(t_delay);
        MOSI_SET(bit);
        _spi_delay(t_delay);
        SCK_HIGH();
        _spi_delay(t_delay);
        res = MISO_GET();
        SCK_LOW();
    }
    else if(!_cpol && _cpha){
        SCK_HIGH();
        _spi_delay(t_delay);
        MOSI_SET(bit);
        _spi_delay(t_delay);
        SCK_LOW();
        _spi_delay(t_delay);
        res = MISO_GET();
        SCK_HIGH();
    }
    else if(_cpol && !_cpha){
        SCK_HIGH();
        _spi_delay(t_delay);
        MOSI_SET(bit);
        _spi_delay(t_delay);
        SCK_LOW();
        _spi_delay(t_delay);
        res = MISO_GET();
        SCK_HIGH();
    }
    else{
        SCK_LOW();
        _spi_delay(t_delay);
        MOSI_SET(bit);
        _spi_delay(t_delay);
        SCK_HIGH();
        _spi_delay(t_delay);
        res = MISO_GET();
        SCK_LOW();
    }

    return res;
}

// This code is working for the reading operation,
// but has problems with writing.
// int _spi_bit_read_write(int bit)
// {
//     int res = 0;

//     if(!_cpol && !_cpha){
//         MOSI_SET(bit);
//         SCK_HIGH();
//         res = MISO_GET();
//         _spi_delay(t_delay);
//         SCK_LOW();
//         _spi_delay(t_delay);
//     }
//     else if(!_cpol && _cpha){
//         SCK_HIGH();
//         _spi_delay(t_delay);
//         MOSI_SET(bit);
//         res = MISO_GET();
//         _spi_delay(t_delay);
//         SCK_LOW();
//         _spi_delay(t_delay);
//         SCK_HIGH();
//     }
//     else if(_cpol && !_cpha){
//         SCK_HIGH();
//         _spi_delay(t_delay);
//         MOSI_SET(bit);
//         res = MISO_GET();
//         _spi_delay(t_delay);
//         SCK_LOW();
//         _spi_delay(t_delay);
//         SCK_HIGH();
//     }
//     else{
//         SCK_LOW();
//         _spi_delay(t_delay);
//         MOSI_SET(bit);
//         res = MISO_GET();
//         _spi_delay(t_delay);
//         SCK_HIGH();
//         _spi_delay(t_delay);
//         SCK_LOW();
//     }

//     return res;
// }

static int _spi_byte_write(uint8_t data)
{
    int i;

    spin_lock_irq(&wire_lock);

    for(i = 7 ; i >= 0 ; i--){
        _spi_bit_read_write((data >> i) & 1);
    }

    spin_unlock_irq(&wire_lock);

    return 0;
}

static uint8_t _spi_byte_read(void)
{
    int i;
    uint8_t data = 0;

    spin_lock_irq(&wire_lock);

    for(i = 0 ; i < 8 ; i++){
        data = (data << 1) | ((uint8_t)(_spi_bit_read_write(1)));
    }

    spin_unlock_irq(&wire_lock);

    return data;
}

static uint8_t _spi_byte_read_write(uint8_t data)
{
    int i;
    uint8_t ret_data = 0;

    spin_lock_irq(&wire_lock);

    for(i = 7 ; i >= 0 ; i--){
        ret_data = (ret_data << 1) | ((uint8_t)(_spi_bit_read_write((data >> i) & 1)));
    }

    spin_unlock_irq(&wire_lock);

    return ret_data;
}

int soft_spi_read(uint8_t reg_addr, uint8_t* data, const size_t len)
{
    int i = 0;

    _spi_start();
    _spi_byte_write(reg_addr);

    if(data_xchg_flag){
        for(i = 0 ; i < len ; i++){
            data[i] = _spi_byte_read_write(data[i]);
        }
    }
    else{
        for(i = 0 ; i < len ; i++){
            data[i] = _spi_byte_read();
        }
    }

    _spi_end();

    return 0;

}

int soft_spi_write(uint8_t reg_addr,uint8_t* data, const size_t len)
{
    int i = 0;

    _spi_start();
    _spi_byte_write(reg_addr);

    for(i = 0 ; i < len ; i++){
        _spi_byte_write(data[i]);
    }

    _spi_end();

    return 0;
}

