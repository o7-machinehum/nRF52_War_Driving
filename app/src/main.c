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

static void start_scan(void);

static struct bt_conn *default_conn;

static struct bt_uuid_16 uuid = BT_UUID_INIT_16(0);
static struct bt_gatt_discover_params discover_params;
static struct bt_gatt_subscribe_params subscribe_params;

LOG_MODULE_REGISTER(main);

static FATFS fat_fs;
/* mounting info */
static struct fs_mount_t mp = {
	.type = FS_FATFS,
	.fs_data = &fat_fs,
};

/*
*  Note the fatfs library is able to mount only strings inside _VOLUME_STRS
*  in ffconf.h
*/
static const char *disk_mount_pt = "/SD:";

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char dev[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(addr, dev, sizeof(dev));

	printk("[DEVICE]: %s, AD evt type %u, AD data len %u, RSSI %i\n",
	    dev, type, ad->len, rssi);
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

    
	struct fs_file_t filep;
    char buf[128];
	fs_file_t_init(&filep);
    int f = fs_open(&filep, "/SD:/fname.txt", (FS_O_CREATE | FS_O_WRITE | FS_O_APPEND));
    size_t len = sprintf(buf, "Hello World\n");
    printk("sprintf(): %d\n", len);
    res = fs_write(&filep, buf, len);
    printk("fs_write(): %d\n", res);
    res = fs_close(&filep);
    printk("fs_close(): %d\n", res);
	res = fs_unmount(&mp);
    printk("fs_umount(): %d\n", res);
    printk("Disk Unmounted\n");

    while(1){};

	err = bt_enable(NULL);

	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	bt_conn_cb_register(&conn_callbacks);

	start_scan();
}

static int lsdir(const char *path)
{
	int res;
	struct fs_dir_t dirp;
	static struct fs_dirent entry;

	fs_dir_t_init(&dirp);

	/* Verify fs_opendir() */
	res = fs_opendir(&dirp, path);
	if (res) {
		printk("Error opening dir %s [%d]\n", path, res);
		return res;
	}

	printk("\nListing dir %s ...\n", path);
	for (;;) {
		/* Verify fs_readdir() */
		res = fs_readdir(&dirp, &entry);

		/* entry.name[0] == 0 means end-of-dir */
		if (res || entry.name[0] == 0) {
			break;
		}

		if (entry.type == FS_DIR_ENTRY_DIR) {
			printk("[DIR ] %s\n", entry.name);
		} else {
			printk("[FILE] %s (size = %zu)\n",
				entry.name, entry.size);
		}
	}

	/* Verify fs_closedir() */
	fs_closedir(&dirp);

	return res;
}
