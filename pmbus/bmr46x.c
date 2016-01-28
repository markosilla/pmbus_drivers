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

#define BMR46X_WAIT_TIME		1000	/* uS	*/

static ushort delay = BMR46X_WAIT_TIME;
module_param(delay, ushort, 0644);
MODULE_PARM_DESC(delay, "Delay between chip accesses in uS");

/* Some chips need a delay between accesses */
static inline void bmr46x_wait(const struct bmr46x_data *data)
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

    /*
     * Limit register detection.
     */
    switch (reg) {
    case PMBUS_VOUT_OV_WARN_LIMIT:
    case PMBUS_VOUT_UV_WARN_LIMIT:
    case PMBUS_IOUT_OC_WARN_LIMIT:
    case PMBUS_IOUT_OC_LV_FAULT_LIMIT: /* NotImpl on BMR461 */
        return -ENXIO;
    default:
        if (reg >= PMBUS_VIRT_BASE)
            return -ENXIO;
        vreg = reg;
        break;
    }

    bmr46x_wait(data);
    ret = pmbus_read_word_data(client, page, vreg);
    data->access = ktime_get();

    return ret;
}

static int bmr46x_read_byte_data(struct i2c_client *client, int page, int reg)
{
    const struct pmbus_driver_info *info = pmbus_get_driver_info(client);
    struct bmr46x_data *data = to_bmr46x_data(info);
    int ret, status;

    if (page > 0)
        return -ENXIO;
    /*
     * Limit register detection.
     */
    switch (reg) {
    case PMBUS_VOUT_OV_WARN_LIMIT:
    case PMBUS_VOUT_UV_WARN_LIMIT:
    case PMBUS_IOUT_OC_WARN_LIMIT:
    case PMBUS_IOUT_OC_LV_FAULT_LIMIT: /* NotImpl on BMR461 */
        return -ENXIO;
    default:
        if (reg >= PMBUS_VIRT_BASE)
            return -ENXIO;
        break;
    }

    bmr46x_wait(data);
    ret = pmbus_read_byte_data(client, page, reg);
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

    /*
     * Limit register detection.
     */
    switch (reg) {
    case PMBUS_VOUT_OV_WARN_LIMIT:
    case PMBUS_VOUT_UV_WARN_LIMIT:
    case PMBUS_IOUT_OC_WARN_LIMIT:
    case PMBUS_IOUT_OC_LV_FAULT_LIMIT: /* NotImpl on BMR461 */
        return -ENXIO;
    default:
        if (reg >= PMBUS_VIRT_BASE)
            return -ENXIO;
        vreg = reg;
        break;
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

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_READ_WORD_DATA))
        return -ENODEV;

    data = devm_kzalloc(&client->dev, sizeof(struct bmr46x_data),GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    data->id = id->driver_data;

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

    return pmbus_do_probe(client, id, info);
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
