/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr.h>
#include <sys/printk.h>

#include <device.h>
#include <drivers/gpio.h>
#include <storage/disk_access.h>
#include <logging/log.h>
#include <fs/fs.h>
#include <ff.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <sys/byteorder.h>

#include <stdio.h>

// Enable this if you want output on the serial bus
#define DEBUG false

#define BUT1_NODE   DT_NODELABEL(button0)
#define BUT2_NODE   DT_NODELABEL(button1)

static const struct gpio_dt_spec start_pb = GPIO_DT_SPEC_GET_OR(BUT1_NODE, gpios,
                                  {0});

static const struct gpio_dt_spec stop_pb = GPIO_DT_SPEC_GET_OR(BUT2_NODE, gpios,
                                  {0});

static struct gpio_dt_spec led = GPIO_DT_SPEC_GET_OR(DT_NODELABEL(led1), gpios,
						     {0});

static void start_scan(void);

LOG_MODULE_REGISTER(main);

static FATFS fat_fs;
/* mounting info */
static struct fs_mount_t mp = {
    .type = FS_FATFS,
    .fs_data = &fat_fs,
};

struct fs_file_t filep;
char buf[512];

/*
*  Note the fatfs library is able to mount only strings inside _VOLUME_STRS
*  in ffconf.h
*/
static const char *disk_mount_pt = "/SD:";

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
             struct net_buf_simple *ad)
{
    char dev[BT_ADDR_LE_STR_LEN];
    uint32_t t = k_uptime_get_32();

    bt_addr_le_to_str(addr, dev, sizeof(dev));

    if(DEBUG)
        printk("t(ms): %d [DEVICE]: %s, AD evt type %u, AD data len %u, RSSI %i\n",
        t, dev, type, ad->len, rssi);

    size_t len = sprintf(buf, "t(ms): %d [DEVICE]: %s, AD evt type %u, \
        AD data len %u, RSSI %i\n", t, dev, type, ad->len, rssi);
    fs_write(&filep, buf, len);

    k_msleep(1000);
	if(gpio_pin_get_dt(&stop_pb))
    {
        printk("Shutting Down \n"); 
        len = sprintf(buf, "Shutting Down \n"); 
        fs_write(&filep, buf, len);

        fs_close(&filep);
        fs_unmount(&mp);
        k_msleep(1000);
	    gpio_pin_set_dt(&led, true);
        while(true){};
    };
}

static void start_scan(void)
{
    int err;

    /* Use active scanning and disable duplicate filtering to handle any
     * devices that might update their advertising data at runtime. */
    struct bt_le_scan_param scan_param = {
        .type       = BT_LE_SCAN_TYPE_ACTIVE,
        .options    = BT_LE_SCAN_OPT_NONE,
        .interval   = BT_GAP_SCAN_FAST_INTERVAL,
        .window     = BT_GAP_SCAN_FAST_WINDOW,
    };

    err = bt_le_scan_start(&scan_param, device_found);
    if (err) {
        printk("Scanning failed to start (err %d)\n", err);
        return;
    }

    printk("Scanning successfully started\n");
}

static struct bt_conn_cb conn_callbacks = {
    .connected = NULL,
    .disconnected = NULL,
};

void main(void)
{
    int err;
    
    gpio_pin_configure_dt(&start_pb, GPIO_INPUT);
    gpio_pin_configure_dt(&stop_pb, GPIO_INPUT);
	gpio_pin_configure_dt(&led, GPIO_OUTPUT);

	gpio_pin_set_dt(&led, true);
	while(!gpio_pin_get_dt(&start_pb)){};
	gpio_pin_set_dt(&led, false);

    /* raw disk i/o */
    do {
        static const char *disk_pdrv = "SD";
        uint64_t memory_size_mb;
        uint32_t block_count;
        uint32_t block_size;

        if (disk_access_init(disk_pdrv) != 0) {
            LOG_ERR("Storage init ERROR!");
            break;
        }

        if (disk_access_ioctl(disk_pdrv,
                DISK_IOCTL_GET_SECTOR_COUNT, &block_count)) {
            LOG_ERR("Unable to get sector count");
            break;
        }
        LOG_INF("Block count %u", block_count);

        if (disk_access_ioctl(disk_pdrv,
                DISK_IOCTL_GET_SECTOR_SIZE, &block_size)) {
            LOG_ERR("Unable to get sector size");
            break;
        }
        printk("Sector size %u\n", block_size);

        memory_size_mb = (uint64_t)block_count * block_size;
        printk("Memory Size(MB) %u\n", (uint32_t)(memory_size_mb >> 20));
    } while (0);

    mp.mnt_point = disk_mount_pt;

    int res = fs_mount(&mp);

    if (res == FR_OK) {
        printk("Disk mounted.\n");
    } else {
        printk("Error mounting disk.\n");
        while(1){};
    }
    
    fs_file_t_init(&filep);
    fs_open(&filep, "/SD:/fname.txt", (FS_O_CREATE | FS_O_WRITE | FS_O_APPEND));

    size_t len = sprintf(buf, "Starting Up\n-----------\n"); 
    fs_write(&filep, buf, len);

    err = bt_enable(NULL);

    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
        return;
    }

    printk("Bluetooth initialized\n");

    bt_conn_cb_register(&conn_callbacks);

    start_scan();
}
