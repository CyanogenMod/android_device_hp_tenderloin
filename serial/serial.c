/*
 * Copyright (C) 2011 CyanogenMod
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mount.h>

int main()
{
    char serial[50];
    char out[128];
    FILE *fd;

    if ((fd = fopen("/proc/nduid", "r")) == NULL)
        return -1;

    fgets(serial, 50, fd);
    fclose(fd);

    if (strlen(serial) <= 0)
        return -1;

    mount("rootfs", "/", "rootfs", MS_REMOUNT|0, NULL);

    if ((fd = fopen("/default.prop", "a")) == NULL)
        return -1;

    sprintf(out, "ro.serialno=%s", serial);
    fputs(out, fd);
    fclose(fd);

    return 0;
}
