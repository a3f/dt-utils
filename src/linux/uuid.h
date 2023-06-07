/*
 * Copyright (C) 2023 Pengutronix, Ahmad Fatoum <a.fatoum@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef _LINUX_UUID_H_
#define _LINUX_UUID_H_

#define   UUID_STRING_LEN         36

/* We only do string comparisons anyway */
typedef struct { const char str[UUID_STRING_LEN + 1]; } guid_t;

#define GUID_INIT(str) { str }

#endif
