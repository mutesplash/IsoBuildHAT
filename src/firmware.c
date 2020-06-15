/* firmware.c
 *
 * Copyright (c) Kynesim Ltd, 2020
 *
 * Python class for handling firmware upgrade
 */

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "structmember.h"

#include <stdint.h>

#include "firmware.h"
#include "cmd.h"
#include "i2c.h"
#include "callback.h"

/**

.. py:class:: Firmware

    Python class to handle firmware upgrades

    .. note::

        WRITE THIS DOCUMENTATION
*/


/* Status values for the firmware object */
#define FW_STATUS_IDLE    0
#define FW_STATUS_ERASING 1

#define FW_STATUS_MAX     (FW_STATUS_ERASING+1)


/* The actual Firmware type */
typedef struct
{
    PyObject_HEAD
    uint32_t current_address;
    volatile uint8_t status;
    PyObject *callback;
} FirmwareObject;

const char *status_messages[FW_STATUS_MAX] = {
    NULL,
    "Erase in progress"
};


static int
Firmware_traverse(FirmwareObject *self, visitproc visit, void *arg)
{
    Py_VISIT(self->callback);
    return 0;
}


static int
Firmware_clear(FirmwareObject *self)
{
    Py_CLEAR(self->callback);
    return 0;
}


static void
Firmware_dealloc(FirmwareObject *self)
{
    PyObject_GC_UnTrack(self);
    Firmware_clear(self);
    Py_TYPE(self)->tp_free((PyObject *)self);
}


static PyObject *
Firmware_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    FirmwareObject *self = (FirmwareObject *)type->tp_alloc(type, 0);
    if (self != NULL)
    {
        self->current_address = 0;
        self->status = FW_STATUS_IDLE;
        i2c_register_firmware_object((PyObject *)self);
        self->callback = Py_None;
        Py_INCREF(Py_None);
    }
    return (PyObject *)self;
}


static PyObject *
Firmware_appl_image_initialize(PyObject *self, PyObject *args)
{
    FirmwareObject *firmware = (FirmwareObject *)self;
    uint32_t nbytes;
    uint8_t status;

    if (!PyArg_ParseTuple(args, "k:nbytes", &nbytes))
        return NULL;

    status = firmware->status;
    if (status != FW_STATUS_IDLE)
    {
        if (status >= FW_STATUS_MAX)
        {
            PyErr_Format(cmd_get_exception(),
                         "Unexpected firmware state %d",
                         status);
        }
        else
        {
            PyErr_SetString(cmd_get_exception(), status_messages[status]);
        }
        return NULL;
    }

    firmware->current_address = 0;
    firmware->status = FW_STATUS_ERASING;
    if (cmd_firmware_init(nbytes) < 0)
    {
        firmware->status = FW_STATUS_IDLE;
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyObject *
Firmware_callback(PyObject *self, PyObject *args)
{
    FirmwareObject *firmware = (FirmwareObject *)self;
    PyObject *callable = NULL;

    if (!PyArg_ParseTuple(args, "|O:callback", &callable))
        return NULL;
    if (callable == NULL)
    {
        /* Return the current callback */
        Py_INCREF(firmware->callback);
        return firmware->callback;
    }

    /* Otherwise we are setting a new callback (or unsetting it with
     * None).  Ensure that we have something legitimate.
     */
    if (callable != Py_None && !PyCallable_Check(callable))
    {
        PyErr_SetString(PyExc_TypeError, "callback must be callable");
        return NULL;
    }
    Py_INCREF(callable);
    Py_XDECREF(firmware->callback);
    firmware->callback = callable;

    Py_RETURN_NONE;
}


static PyMethodDef Firmware_methods[] = {
    {
        "appl_image_initialize",
        Firmware_appl_image_initialize,
        METH_VARARGS,
        "Initialize HAT to receive a new firmware image"
    },
    {
        "callback",
        Firmware_callback,
        METH_VARARGS,
        "Get or set callback function"
    },
    { NULL, NULL, 0, NULL }
};


static PyTypeObject FirmwareType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "Firmware",
    .tp_doc = "Firmware Upgrade Manager",
    .tp_basicsize = sizeof(FirmwareObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_new = Firmware_new,
    .tp_dealloc = (destructor)Firmware_dealloc,
    .tp_traverse = (traverseproc)Firmware_traverse,
    .tp_clear = (inquiry)Firmware_clear,
    .tp_methods = Firmware_methods
};


int firmware_modinit(void)
{
    if (PyType_Ready(&FirmwareType) < 0)
        return -1;
    Py_INCREF(&FirmwareType);

    return 0;
}


void firmware_demodinit(void)
{
    Py_DECREF(&FirmwareType);
}


PyObject *firmware_init(void)
{
    return PyObject_CallObject((PyObject *)&FirmwareType, NULL);
}


int firmware_action_done(PyObject *firmware,
                         uint8_t reason,
                         uint8_t param)
{
    FirmwareObject *self = (FirmwareObject *)firmware;

    self->status = FW_STATUS_IDLE;

    return callback_queue(CALLBACK_FIRMWARE, reason, param, firmware);
}


int firmware_handle_callback(uint8_t reason, uint8_t param, PyObject *firmware)
{
    FirmwareObject *self = (FirmwareObject *)firmware;
    PyGILState_STATE gstate = PyGILState_Ensure();
    PyObject *arg_list;
    int rv;

    if (self->callback == Py_None)
    {
        PyGILState_Release(gstate);
        return 0;
    }

    arg_list = Py_BuildValue("(ii)", reason, param);
    rv = (PyObject_CallObject(self->callback, arg_list) != NULL) ? 0 : -1;
    Py_XDECREF(arg_list);
    PyGILState_Release(gstate);

    return rv;
}
