/*
 * Copyright (C) 2015 Pengutronix, Marc Kleine-Budde <kernel@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 as published by the
 * Free Software Foundation.
 */

#include <common.h>
#include <crypto/keystore.h>
#include <base64.h>
#include <barebox-state.h>
#include <state.h>

static const char keystore_state_name[] = "/blobs";
static const char blob_gen_payload[] = "/sys/bus/platform/devices/blob_gen/payload";
static const char blob_gen_modifier[] = "/sys/bus/platform/devices/blob_gen/modifier";
static const char blob_gen_blob[] = "/sys/bus/platform/devices/blob_gen/blob";

static struct state *state;

int keystore_get_secret(const char *name, const unsigned char **key, int *key_len)
{
	FILE *fp;
	char *blob, *payload;
	u8 *blob_bin, *payload_bin;
	ssize_t len;
	int fd, ret;

	if (!state) {
		struct state *tmp;

		tmp = state_get(keystore_state_name, NULL, true, false);
		if (IS_ERR(tmp))
			return  PTR_ERR(tmp);
		state = tmp;
	}

	/* modifier */
	fp = fopen(blob_gen_modifier, "w");
	if (!fp)
		return -errno;

	ret = fprintf(fp, "user:%s", name);
	if (ret < 0) {
		fclose(fp);
		return ret;
	}

	ret = fclose(fp);
	if (ret == EOF)
		return -errno;


	/* blob */
	blob = state_get_var(state, name);
	if (!blob)
		return -ENOENT;

	len = strlen(blob) + 1;
	blob_bin = xzalloc(len);
	len = decode_base64(blob_bin, len, blob);
	free(blob);

	fd = open(blob_gen_blob, O_WRONLY);
	if (fd < 0) {
		free(blob_bin);
		return -errno;
	}

	ret = write(fd, blob_bin, len);
	free(blob_bin);
	if (ret != len) {
		return -errno;
	}

	ret = close(fd);
	if (ret)
		return -errno;


	/* payload */
	fd = open(blob_gen_payload, O_RDONLY);
	if (fd < 0)
		return -errno;

	payload = xzalloc(len);
	len = read(fd, payload, len);
	close(fd);
	if (len <= 0) {
		free(payload);
		return -errno;
	}

	payload_bin = xzalloc(len);
	len = decode_base64(payload_bin, len, payload);
	free(payload);

	*key = payload_bin;
	*key_len = len;

	return 0;
}
