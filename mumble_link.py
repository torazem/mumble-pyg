"""
Transcribed from https://wiki.mumble.info/wiki/Link
"""

import gc
import logging
import math
import os
import subprocess
from collections import namedtuple
from ctypes import (
    Structure,
    c_char,
    c_float,
    c_uint32,
    c_wchar,
    create_string_buffer,
    create_unicode_buffer,
    sizeof,
)
from multiprocessing import shared_memory

logger = logging.getLogger(__name__)

c_Vector3 = c_float * 3
Point = namedtuple("Point", ["x", "y"])


class _MumbleLinkedMemory(Structure):
    _fields_ = [
        ("uiVersion", c_uint32),
        ("uiTick", c_uint32),
        ("fAvatarPosition", c_Vector3),
        ("fAvatarFront", c_Vector3),
        ("fAvatarTop", c_Vector3),
        ("name", c_wchar * 256),
        ("fCameraPosition", c_Vector3),
        ("fCameraFront", c_Vector3),
        ("fCameraTop", c_Vector3),
        ("identity", c_wchar * 256),
        ("context_len", c_uint32),
        ("context", c_char * 256),
        ("description", c_wchar * 2048),
    ]


class MumbleConnection:
    def __init__(self, app_name, app_description, context, user):
        self._shared_mem_name = f"PyMumbleLink.{os.getuid()}"
        print(self._shared_mem_name)
        logger.info("Using shared memory %s", self._shared_mem_name)

        kwargs = dict(name=self._shared_mem_name, size=sizeof(_MumbleLinkedMemory))
        try:
            self._shm = shared_memory.SharedMemory(**kwargs)
        except FileNotFoundError:
            self._shm = shared_memory.SharedMemory(**kwargs, create=True)

        self._linked_mem = _MumbleLinkedMemory.from_buffer(self._shm.buf)

        self._linked_mem.uiVersion = c_uint32(2)

        self._linked_mem.name = create_unicode_buffer(app_name, 256).value
        self._linked_mem.description = create_unicode_buffer(
            app_description, 2048
        ).value

        self.context = context
        self.user = user

        self.position = Point(0, 0)
        self.set_rotation(0)

        self._linked_mem.fAvatarTop = c_Vector3(0, 1, 0)
        self._linked_mem.fCameraFront = self._linked_mem.fAvatarFront
        self._linked_mem.fCameraPosition = self._linked_mem.fAvatarPosition
        self._linked_mem.fCameraTop = self._linked_mem.fAvatarTop

    def sync(self):
        # TODO: make python C++ extension
        subprocess.run(["./mumble_link"])

    def tick(self):
        self._linked_mem.uiTick += 1

    @property
    def context(self):
        return self._linked_mem.context.decode("utf-8")

    @property
    def user(self):
        return self._linked_mem.identity

    @property
    def position(self):
        raise NotImplementedError("TODO: getter undefined")
        return self._linked_mem.fAvatarPosition

    @property
    def front(self):
        raise NotImplementedError("TODO: getter undefined")
        return self._linked_mem.fAvatarFront

    @context.setter
    def context(self, value):
        logger.info("Updating context to %s", value)
        self._linked_mem.context = create_string_buffer(value, 256).value
        self._linked_mem.context_len = c_uint32(len(value))

    @user.setter
    def user(self, value):
        logger.info("Updating user to %s", value)
        self._linked_mem.identity = create_unicode_buffer(value, 256).value

    @position.setter
    def position(self, value):
        logger.debug("Position %s", value)
        self._linked_mem.fAvatarPosition = c_Vector3(value.x, 0, value.y)

    def set_rotation(self, value):
        logger.debug("Rotation %f degrees", value)
        r = math.radians(value)
        x, y = math.cos(r), math.sin(r)
        logger.debug("Front vector: (%f, %f)", x, y)
        self._linked_mem.fAvatarFront = c_Vector3(x, 0, y)

    def close(self):
        del self._linked_mem
        gc.collect()
        self._shm.close()
        self._shm.unlink()

        print("Closed successfully")
        logger.info("Closed successfully")

    def __enter__(self):
        return self

    def __exit__(self, *args, **kwargs):
        self.close()
