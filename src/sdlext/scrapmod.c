/*
  pygame - Python Game Library
  Copyright (C) 2007  Rene Dudfield, Marcus von Appen

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public
  License along with this library; if not, write to the Free
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#define PYGAME_SDLEXTSCRAP_INTERNAL

#include "pgsdlext.h"
#include "pgsdl.h"
#include "scrap.h"

#define ASSERT_SCRAP_INIT(x)                                            \
    if (!pyg_scrap_was_init ())                                         \
    {                                                                   \
        PyErr_SetString (PyExc_PyGameError, "scrap not initialized");   \
        return (x);                                                     \
    }

static PyObject *_scrapselection = NULL;
static PyObject *_scrapclipboard = NULL;

static void _quit (void);

static PyObject* _scrap_init (PyObject* self);
static PyObject* _scrap_quit (PyObject* self);
static PyObject* _scrap_wasinit (PyObject* self);
static PyObject* _scrap_contains (PyObject* self, PyObject* args);
static PyObject* _scrap_get (PyObject* self, PyObject* args);
static PyObject* _scrap_put (PyObject* self, PyObject* args);
static PyObject* _scrap_gettypes (PyObject* self);
static PyObject* _scrap_lost (PyObject* self);
static PyObject* _scrap_setmode (PyObject* self, PyObject* args);
static PyObject* _scrap_getmode (PyObject* self);

static PyMethodDef _scrap_methods[] =
{
    { "init", (PyCFunction)_scrap_init, METH_NOARGS, "" },
    { "quit", (PyCFunction)_scrap_quit, METH_NOARGS, "" },
    { "was_init", (PyCFunction)_scrap_wasinit, METH_NOARGS, "" },
    { "contains", _scrap_contains, METH_VARARGS, "" },
    { "get", _scrap_get, METH_VARARGS, "" },
    { "put", _scrap_put, METH_VARARGS, "" },
    { "get_types", (PyCFunction)_scrap_gettypes, METH_NOARGS, "" },
    { "lost", (PyCFunction)_scrap_lost, METH_NOARGS, "" },
    { "set_mode", _scrap_setmode, METH_VARARGS, "" },
    { "get_mode", (PyCFunction)_scrap_getmode, METH_NOARGS, "" },
    { NULL, NULL, 0, NULL }
};

static void
_quit (void)
{
    if (pyg_scrap_was_init ())
        pyg_scrap_quit ();
}

static PyObject*
_scrap_init (PyObject* self)
{
    if (pyg_scrap_was_init ())
        Py_RETURN_NONE;
    if (!pyg_scrap_init ())
    {
        PyErr_SetString (PyExc_PyGameError, SDL_GetError ());
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject*
_scrap_quit (PyObject* self)
{
    Py_XDECREF (_scrapclipboard);
    Py_XDECREF (_scrapselection);
    _scrapclipboard = NULL;
    _scrapselection = NULL;

    _quit ();
    Py_RETURN_NONE;
}

static PyObject*
_scrap_wasinit (PyObject* self)
{
    if (pyg_scrap_was_init ())
        Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}

static PyObject*
_scrap_contains (PyObject* self, PyObject* args)
{
    char *type;
    int found;

    ASSERT_SCRAP_INIT (NULL);

    if (!PyArg_ParseTuple (args, "s:contains", &type))
        return NULL;

    found = pyg_scrap_contains (type);
    if (found == -1)
    {
        PyErr_SetString (PyExc_PyGameError, SDL_GetError ());
        return NULL;
    }
    return PyBool_FromLong (found);
}

static PyObject*
_scrap_get (PyObject* self, PyObject* args)
{
    char *type, *data;
    unsigned int size;
    int result;
    PyObject *val;

    ASSERT_SCRAP_INIT (NULL);

    if (!PyArg_ParseTuple (args, "s:get", &type))
        return NULL;

    if (!pyg_scrap_lost ())
    {
        /* We still maintain the clipboard. Get the data from the
         * internal dictionaries instead of the window manager.
         */
        switch (pyg_scrap_get_mode ())
        {
        case SCRAP_SELECTION:
            val = PyDict_GetItemString (_scrapselection, type);
            break;
        case SCRAP_CLIPBOARD:
        default:
            val = PyDict_GetItemString (_scrapclipboard, type);
            break;
        }
        if (!val)
            Py_RETURN_NONE;
        Py_INCREF (val);
        return val;
    }

    /* pyg_get_scrap() only returns NULL or !NULL, but won't set any
     * errors. */
    result = pyg_scrap_get (type, &data, &size);
    if (result == -1)
    {
        PyErr_SetString (PyExc_PyGameError, SDL_GetError ());
        return NULL;
    }
    if (!result)
        Py_RETURN_NONE;
    return Text_FromUTF8AndSize (data, size);
}

static PyObject*
_scrap_put (PyObject* self, PyObject* args)
{
    char *type, *data;
    unsigned int length;
    PyObject *tmp;

    ASSERT_SCRAP_INIT (NULL);
    
    if (!PyArg_ParseTuple (args, "st#", &type, &data, &length))
        return NULL;

    if (!pyg_scrap_put (type, data, length) == -1)
    {
        PyErr_SetString (PyExc_PyGameError, SDL_GetError ());
        return NULL;
    }

    /* Add or replace the set value. */
    switch (pyg_scrap_get_mode ())
    {
    case SCRAP_SELECTION:
    {
        tmp = Text_FromUTF8AndSize (data, length);
        PyDict_SetItemString (_scrapselection, type, tmp);
        Py_DECREF (tmp);
        break;
    }
    case SCRAP_CLIPBOARD:
    default:
    {
        tmp = Text_FromUTF8AndSize (data, length);
        PyDict_SetItemString (_scrapclipboard, type, tmp);
        Py_DECREF (tmp);
        break;
    }
    }
    Py_RETURN_NONE;
}

static PyObject*
_scrap_gettypes (PyObject* self)
{
    int i = 0, result;
    char **types = NULL;
    PyObject *list, *item;
    
    ASSERT_SCRAP_INIT (NULL);
    
    if (!pyg_scrap_lost ())
    {
        switch (pyg_scrap_get_mode ())
        {
        case SCRAP_SELECTION:
            return PyDict_Keys (_scrapselection);
        case SCRAP_CLIPBOARD:
        default:
            return PyDict_Keys (_scrapclipboard);
        }
    }

    list = PyList_New (0);
    if (!list)
        return NULL;

    result = pyg_scrap_get_types (types);
    if (result == -1)
    {
        PyErr_SetString (PyExc_PyGameError, SDL_GetError ());
        return NULL;
    }
    
    if (!types)
        return list;

    while (types[i])
    {
        item = Text_FromUTF8 (types[i]);
        PyList_Append (list, item);
        Py_DECREF (item);
        free (types[i]);
        i++;
    }
    free (types);
    return list;
}

static PyObject*
_scrap_lost (PyObject* self)
{
    ASSERT_SCRAP_INIT (NULL);
    if (pyg_scrap_lost ())
        Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}

static PyObject*
_scrap_setmode (PyObject* self, PyObject* args)
{
    int mode, _setmode;

    ASSERT_SCRAP_INIT (NULL);
    if (!PyArg_ParseTuple (args, "i", &mode))
        return NULL;

    _setmode = pyg_scrap_set_mode (mode);
    if (_setmode == -1)
    {
        PyErr_SetString (PyExc_PyGameError, SDL_GetError ());
        return NULL;
    }
    return PyInt_FromLong (_setmode);
}

static PyObject*
_scrap_getmode (PyObject* self)
{
    ASSERT_SCRAP_INIT (NULL);
    return PyLong_FromUnsignedLong (pyg_scrap_get_mode ());
}

#if PY_VERSION_HEX >= 0x03000000
PyMODINIT_FUNC PyInit_scrap (void)
#else
PyMODINIT_FUNC initscrap (void)
#endif
{
    PyObject *mod;

#if PY_VERSION_HEX >= 0x03000000
    static struct PyModuleDef _module = {
        PyModuleDef_HEAD_INIT,
        "scrap",
        "",
        -1,
        _scrap_methods,
        NULL, NULL, NULL, NULL
    };
#endif

#if PY_VERSION_HEX < 0x03000000
    mod = Py_InitModule3 ("scrap", _scrap_methods, "");
#else
    mod = PyModule_Create (&_module);
#endif
    if (!mod)
        goto fail;
    if (import_pygame2_base () < 0)
        goto fail;
    if (import_pygame2_sdl_base () < 0)
        goto fail;
    if (import_pygame2_sdl_video () < 0)
        goto fail;
    RegisterQuitCallback (_quit);
    MODINIT_RETURN (mod);
fail:
    Py_XDECREF (mod);
    MODINIT_RETURN (NULL);
}