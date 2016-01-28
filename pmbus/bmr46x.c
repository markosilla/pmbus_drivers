/*
 * Hardware monitoring driver for BMR46x and compatibles
 *
 * Copyright (c) 2016 Ericsson AB.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/ktime.h>
#include <linux/delay.h>
#include "pmbus.h"

enum chips { BMR461, BMR463 };

struct bmr46x_data {
    int id;
    ktime_t access; /* chip access time */
    int delay;		/* Delay between chip accesses in uS */
    struct pmbus_driver_info info;
};

#define to_bmr46x_data(x)  container_of(x, struct bmr46x_data, info)

#define MFR_VMON_OV_FAULT_LIMIT		0xf5
#define MFR_VMON_UV_FAULT_LIMIT		0xf6
#define MFR_READ_VMON			0xf7

#define VMON_UV_WARNING			(1 << 5)
#define VMON_OV_WARNING			(1 << 4)
#define VMON_UV_FAULT			(1 << 1)
#define VMON_OV_FAULT			(1 << 0)

#define BMR46X_WAIT_TIME		1000	/* uS	*/

static ushort delay = BMR46X_WAIT_TIME;
module_param(delay, ushort, 0644);
MODULE_PARM_DESC(delay, "Delay between chip accesses in uS");

/* Convert linear sensor value to milli-units */
static long bmr46x_l2d(s16 l)
{
    s16 exponent;
    s32 mantissa;
    long val;

    exponent = l >> 11;
    mantissa = ((s16)((l & 0x7ff) << 5)) >> 5;

    val = mantissa;

    /* scale result to milli-units */
    val = val * 1000L;

    if (exponent >= 0)
        val <<= exponent;
    else
        val >>= -exponent;

    return val;
}

#define MAX_MANTISSA	(1023 * 1000)
#define MIN_MANTISSA	(511 * 1000)

static u16 bmr46x_d2l(long val)
{
    s16 exponent = 0, mantissa;
    bool negative = false;

    /* simple case */
    if (val == 0)
        return 0;

    if (val < 0) {
        negative = true;
        val = -val;
    }

    /* Reduce large mantissa until it fits into 10 bit */
    while (val >= MAX_MANTISSA && exponent < 15) {
        exponent++;
        val >>= 1;
    }
    /* Increase small mantissa to improve precision */
    while (val < MIN_MANTISSA && exponent > -15) {
        exponent--;
        val <<= 1;
    }

    /* Convert mantissa from milli-units to units */
    mantissa = DIV_ROUND_CLOSEST(val, 1000);

    /* Ensure that resulting number is within range */
    if (mantissa > 0x3ff)
        mantissa = 0x3ff;

    /* restore sign */
    if (negative)
        mantissa = -mantissa;

    /* Convert to 5 bit exponent, 11 bit mantissa */
    return (mantissa & 0x7ff) | ((exponent << 11) & 0xf800);
}

/* Some chips need a delay between accesses */
static inline void bmr46x_wait(const struct zl6100_data *data)
{
    if (data->delay) {
        s64 delta = ktime_us_delta(ktime_get(), data->access);
        if (delta < data->delay)
            udelay(data->delay - delta);
    }
}

static int bmr46x_read_word_data(struct i2c_client *client, int page, int reg)
{
    const struct pmbus_driver_info *info = pmbus_get_driver_info(client);
    struct bmr46x_data *data = to_bmr46x_data(info);
    int ret, vreg;

    if (page > 0)
        return -ENXIO;

    if (data->id == zl2005) {
        /*
         * Limit register detection is not reliable on ZL2005.
         * Make sure registers are not erroneously detected.
         */
        switch (reg) {
        case PMBUS_VOUT_OV_WARN_LIMIT:
        case PMBUS_VOUT_UV_WARN_LIMIT:
        case PMBUS_IOUT_OC_WARN_LIMIT:
            return -ENXIO;
        }
    }

    switch (reg) {
    case PMBUS_VIRT_READ_VMON:
        vreg = MFR_READ_VMON;
        break;
    case PMBUS_VIRT_VMON_OV_WARN_LIMIT:
    case PMBUS_VIRT_VMON_OV_FAULT_LIMIT:
        vreg = MFR_VMON_OV_FAULT_LIMIT;
        break;
    case PMBUS_VIRT_VMON_UV_WARN_LIMIT:
    case PMBUS_VIRT_VMON_UV_FAULT_LIMIT:
        vreg = MFR_VMON_UV_FAULT_LIMIT;
        break;
    default:
        if (reg >= PMBUS_VIRT_BASE)
            return -ENXIO;
        vreg = reg;
        break;
    }

    bmr46x_wait(data);
    ret = pmbus_read_word_data(client, page, vreg);
    data->access = ktime_get();
    if (ret < 0)
        return ret;

    switch (reg) {
    case PMBUS_VIRT_VMON_OV_WARN_LIMIT:
        ret = bmr46x_d2l(DIV_ROUND_CLOSEST(bmr46x_l2d(ret) * 9, 10));
        break;
    case PMBUS_VIRT_VMON_UV_WARN_LIMIT:
        ret = bmr46x_d2l(DIV_ROUND_CLOSEST(bmr46x_l2d(ret) * 11, 10));
        break;
    }

    return ret;
}

static int bmr46x_read_byte_data(struct i2c_client *client, int page, int reg)
{
    const struct pmbus_driver_info *info = pmbus_get_driver_info(client);
    struct bmr46x_data *data = to_bmr46x_data(info);
    int ret, status;

    if (page > 0)
        return -ENXIO;

    bmr46x_wait(data);

    switch (reg) {
    case PMBUS_VIRT_STATUS_VMON:
        ret = pmbus_read_byte_data(client, 0,
                       PMBUS_STATUS_MFR_SPECIFIC);
        if (ret < 0)
            break;

        status = 0;
        if (ret & VMON_UV_WARNING)
            status |= PB_VOLTAGE_UV_WARNING;
        if (ret & VMON_OV_WARNING)
            status |= PB_VOLTAGE_OV_WARNING;
        if (ret & VMON_UV_FAULT)
            status |= PB_VOLTAGE_UV_FAULT;
        if (ret & VMON_OV_FAULT)
            status |= PB_VOLTAGE_OV_FAULT;
        ret = status;
        break;
    default:
        ret = pmbus_read_byte_data(client, page, reg);
        break;
    }
    data->access = ktime_get();

    return ret;
}

static int bmr46x_write_word_data(struct i2c_client *client, int page, int reg,
                  u16 word)
{
    const struct pmbus_driver_info *info = pmbus_get_driver_info(client);
    struct bmr46x_data *data = to_bmr46x_data(info);
    int ret, vreg;

    if (page > 0)
        return -ENXIO;

    switch (reg) {
    case PMBUS_VIRT_VMON_OV_WARN_LIMIT:
        word = bmr46x_d2l(DIV_ROUND_CLOSEST(bmr46x_l2d(word) * 10, 9));
        vreg = MFR_VMON_OV_FAULT_LIMIT;
        pmbus_clear_cache(client);
        break;
    case PMBUS_VIRT_VMON_OV_FAULT_LIMIT:
        vreg = MFR_VMON_OV_FAULT_LIMIT;
        pmbus_clear_cache(client);
        break;
    case PMBUS_VIRT_VMON_UV_WARN_LIMIT:
        word = bmr46x_d2l(DIV_ROUND_CLOSEST(bmr46x_l2d(word) * 10, 11));
        vreg = MFR_VMON_UV_FAULT_LIMIT;
        pmbus_clear_cache(client);
        break;
    case PMBUS_VIRT_VMON_UV_FAULT_LIMIT:
        vreg = MFR_VMON_UV_FAULT_LIMIT;
        pmbus_clear_cache(client);
        break;
    default:
        if (reg >= PMBUS_VIRT_BASE)
            return -ENXIO;
        vreg = reg;
    }

    bmr46x_wait(data);
    ret = pmbus_write_word_data(client, page, vreg, word);
    data->access = ktime_get();

    return ret;
}

static int bmr46x_write_byte(struct i2c_client *client, int page, u8 value)
{
    const struct pmbus_driver_info *info = pmbus_get_driver_info(client);
    struct bmr46x_data *data = to_bmr46x_data(info);
    int ret;

    if (page > 0)
        return -ENXIO;

    bmr46x_wait(data);
    ret = pmbus_write_byte(client, page, value);
    data->access = ktime_get();

    return ret;
}

static const struct i2c_device_id bmr46x_id[] = {
    {"bmr461", BMR461},
    {"bmr463", BMR463},
    { }
};
MODULE_DEVICE_TABLE(i2c, bmr46x_id);

static int bmr46x_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret;
    struct bmr46x_data *data;
    struct pmbus_driver_info *info;
    char * device_id = "bmr461";
    const struct i2c_device_id *mid;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_READ_WORD_DATA))
        return -ENODEV;

    dev_info(&client->dev, "Device ID %s\n", device_id);

    mid = NULL;
    for (mid = bmr46x_id; mid->name[0]; mid++) {
        if (!strncasecmp(mid->name, device_id, strlen(mid->name)))
            break;
    }
    if (!mid->name[0]) {
        dev_err(&client->dev, "Unsupported device\n");
        return -ENODEV;
    }
    if (id->driver_data != mid->driver_data)
        dev_notice(&client->dev,
               "Device mismatch: Configured %s, detected %s\n",
               id->name, mid->name);

    data = devm_kzalloc(&client->dev, sizeof(struct bmr46x_data),GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    data->id = mid->driver_data;

    /*
     * According to information from the chip vendor, all currently
     * supported chips are known to require a wait time between I2C
     * accesses.
     */
    data->delay = delay;

    /*
     * Since there was a direct I2C device access above, wait before
     * accessing the chip again.
     */
    data->access = ktime_get();
    bmr46x_wait(data);

    info = &data->info;

    info->pages = 1;
    info->func[0] = PMBUS_HAVE_VIN | PMBUS_HAVE_STATUS_INPUT
      | PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT
      | PMBUS_HAVE_IOUT | PMBUS_HAVE_STATUS_IOUT
      | PMBUS_HAVE_TEMP | PMBUS_HAVE_STATUS_TEMP;

    data->access = ktime_get();
    bmr46x_wait(data);

    info->read_word_data = bmr46x_read_word_data;
    info->read_byte_data = bmr46x_read_byte_data;
    info->write_word_data = bmr46x_write_word_data;
    info->write_byte = bmr46x_write_byte;

    return pmbus_do_probe(client, mid, info);
}

static struct i2c_driver bmr46x_driver = {
    .driver = {
           .name = "bmr46x",
           },
    .probe = bmr46x_probe,
    .remove = pmbus_do_remove,
    .id_table = bmr46x_id,
};

module_i2c_driver(bmr46x_driver);

MODULE_AUTHOR("Ericsson");
MODULE_DESCRIPTION("PMBus driver for BMR46x series, verified on BMR463, BMR461 v1.0");
MODULE_LICENSE("GPL");
