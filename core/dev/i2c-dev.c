/*
 * Copyright (c) 2016-2016, Yanzi Networks
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Company nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COMPANY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *
 */

#include "dev/i2c-dev.h"
#include "sys/cc.h"
#include <string.h>
/*---------------------------------------------------------------------------*/
int
i2c_dev_has_bus(const i2c_device_t *dev)
{
  if(dev == NULL || dev->bus == NULL) {
    return 0;
  }
  return dev->bus->lock != 0 && dev->bus->lock_device == dev;
}
/*---------------------------------------------------------------------------*/
uint8_t
i2c_dev_acquire(i2c_device_t *dev)
{
  uint8_t status;
  if(dev == NULL || dev->bus == NULL) {
    return BUS_STATUS_EINVAL;
  }

  /* If the bus is already ours - things are ok */
  if(dev->bus->lock_device == dev && dev->bus->lock == 1) {
    return BUS_STATUS_OK;
  }

  if(++dev->bus->lock == 1) {
    dev->bus->lock_device = dev;

    /* lock the bus */
    status = i2c_arch_lock(dev);
    if(status == BUS_STATUS_OK) {
      /* Bus has been locked */
      return BUS_STATUS_OK;
    }

    dev->bus->lock_device = NULL;

    /* Continue down to unlock */
  }

  /* problem... unlock */
  dev->bus->lock--;
  return BUS_STATUS_BUS_LOCKED;
}
/*---------------------------------------------------------------------------*/
uint8_t
i2c_dev_release(i2c_device_t *dev)
{
  if(!i2c_dev_has_bus(dev)) {
    /* The device does not own the bus */
    return BUS_STATUS_EINVAL;
  }

  /* unlock the bus */
  dev->bus->lock_device = NULL;
  if(--dev->bus->lock == 0) {
    return i2c_arch_unlock(dev);
  }
  return BUS_STATUS_EINVAL;
}
/*---------------------------------------------------------------------------*/
uint8_t
i2c_dev_restart_timeout(i2c_device_t *dev)
{
  if(!i2c_dev_has_bus(dev)) {
    return BUS_STATUS_EINVAL;
  }
  return i2c_arch_restart_timeout(dev);
}
/*---------------------------------------------------------------------------*/
uint8_t
i2c_dev_write_byte(i2c_device_t *dev, uint8_t data)
{
  if(!i2c_dev_has_bus(dev)) {
    return BUS_STATUS_BUS_LOCKED;
  }
  return i2c_arch_write(dev, &data, 1);
}
/*---------------------------------------------------------------------------*/
uint8_t
i2c_dev_write(i2c_device_t *dev, const uint8_t *data, int size)
{
  if(!i2c_dev_has_bus(dev)) {
    return BUS_STATUS_BUS_LOCKED;
  }
  return i2c_arch_write(dev, data, size);
}
/*---------------------------------------------------------------------------*/
uint8_t
i2c_dev_read_byte(i2c_device_t *dev, uint8_t *data)
{
  if(!i2c_dev_has_bus(dev)) {
    return BUS_STATUS_BUS_LOCKED;
  }
  return i2c_arch_read(dev, data, 1);
}
/*---------------------------------------------------------------------------*/
uint8_t
i2c_dev_read(i2c_device_t *dev, uint8_t *data, int size)
{
  if(!i2c_dev_has_bus(dev)) {
    return BUS_STATUS_BUS_LOCKED;
  }
  return i2c_arch_read(dev, data, size);
}
/*---------------------------------------------------------------------------*/
/* Read a register - e.g. first write a data byte to select register to read from, then read */
uint8_t
i2c_dev_read_register(i2c_device_t *dev, uint8_t reg, uint8_t *data, int size)
{
  uint8_t status;
  /* write the register first */
  status = i2c_dev_write_byte(dev, reg);
  if(status != BUS_STATUS_OK) {
    return status;
  }
  /* then read the value */
  status = i2c_dev_read(dev, data, size);
  if(status != BUS_STATUS_OK) {
    return status;
  }
  return i2c_arch_stop(dev);
}
/*---------------------------------------------------------------------------*/
/* Write a register - e.g. first write a data byte to select register to write to, then write */
uint8_t
i2c_dev_write_register(i2c_device_t *dev, uint8_t reg, uint8_t data)
{
  uint8_t buffer[2];
  uint8_t status;

  /* write the register first */
  buffer[0] = reg;
  buffer[1] = data;

  status = i2c_dev_write(dev, buffer, 2);
  if(status != BUS_STATUS_OK) {
    return status;
  }

  return i2c_arch_stop(dev);
}
/*---------------------------------------------------------------------------*/
/* Write a register - e.g. first write a data byte to select register to write to, then write */
uint8_t
i2c_dev_write_register_buf(i2c_device_t *dev, uint8_t reg, const uint8_t *data, int size)
{
  uint8_t buffer[size + 1];
  uint8_t status;

  /* write the register first */
  buffer[0] = reg;
  memcpy(&buffer[1], data, size);

  status = i2c_dev_write(dev, buffer, size + 1);
  if(status != BUS_STATUS_OK) {
    return status;
  }

  return i2c_arch_stop(dev);
}
/*---------------------------------------------------------------------------*/
uint8_t
i2c_dev_stop(i2c_device_t *dev)
{
  if(!i2c_dev_has_bus(dev)) {
    return BUS_STATUS_BUS_LOCKED;
  }
  return i2c_arch_stop(dev);
}
/*---------------------------------------------------------------------------*/