/*
 * Author: Thomas Wouters <thomas@xs4all.net>
 *
 * Copyright (c) 2001-2002 Thomas Wouters
 *   All rights reserved.
 *
 * Loosely based on Markus Stenberg's python-patch to TinyMUSH 3.0.
 */

#include "config.h"
#include "db.h"
#include "externs.h"
#include "mudconf.h"
#include "config.h"

#ifdef USE_PYTHON
#include "Python.h"

static dbref cause = -1;
static dbref who = -1;

static long last_sec;
static long last_run;

static PyObject *MakeFakeStdout(dbref i);

static PyObject *sysmod = NULL;
static PyObject *muxmod = NULL;
static PyObject *maindict = NULL;

/* 0 = not started
  -1 = not functional (something severe happened)
   1 = started and ready to serve
*/
static int MUXPy_State = 0;

extern void raw_notify_raw(dbref, char *, char *);
static void init_muxmodule(void);

void MUXPy_ReportError(dbref who, int clearerr)
{
	/* XXX report the traceback, somehow */
	if(clearerr)
		PyErr_Clear();
}

void MUXPy_Abort(char *message)
{
	/* Something went massively wrong. Disable the python runtime
	   and report the error. */

	MUXPy_State = -1;
	STARTLOG(LOG_PROBLEMS, "PYTHON", NULL) {
		log_text("Python Runtime disabled: ");
		log_text(message);
		ENDLOG;
	}
	if(PyErr_Occurred()) {
		MUXPy_ReportError(-1, 1);
	}
	Py_XDECREF(maindict);
	Py_XDECREF(sysmod);
	Py_XDECREF(muxmod);
}

void MUXPy_Init(void)
{

	PyObject *sys_path, *muxpy_path, *mainmod;

	if(MUXPy_State != 0)
		return;

	Py_SetProgramName("netmux");
	Py_Initialize();

	init_muxmodule();

	if((mainmod = PyImport_ImportModule("__main__")) == NULL) {
		MUXPy_Abort("Can't import '__main__' module");
		return;
	}

	if((maindict = PyModule_GetDict(mainmod)) == NULL) {
		MUXPy_Abort("Can't get module dict for '__main__'");
		return;
	}
	Py_INCREF(maindict);

	if((sysmod = PyImport_ImportModule("sys")) == NULL) {
		MUXPy_Abort("Can't import 'sys' module");
		return;
	}

	sys_path = PyObject_GetAttrString(sysmod, "path");
	if(sys_path == NULL) {
		MUXPy_Abort("Can't find sys.path to update!");
		return;
	}
	muxpy_path = PyString_FromString("muxpy-lib");
	if(muxpy_path == NULL) {
		Py_DECREF(sys_path);
		MUXPy_Abort("Can't create muxpy-lib path string");
		return;
	}
	if(PyList_Append(sys_path, muxpy_path) == -1) {
		Py_DECREF(sys_path);
		Py_DECREF(muxpy_path);
		MUXPy_Abort("Can't append muxpy-lib path string to sys.path");
		return;
	}

	if((muxmod = PyImport_ImportModule("mux")) == NULL) {
		MUXPy_Abort("Can't import 'mux' module");
		return;
	}

	STARTLOG(LOG_PROBLEMS, "PYTHON", NULL) {
		log_text("Python Runtime Started.");
		ENDLOG;
	}
	MUXPy_State = 1;
}

#define MAX_UPDATE_PER_SEC 3
#define TICK(x) ((x) / (1000000/MAX_UPDATE_PER_SEC))

static int shouldupdate()
{
	struct timeval tv;
	struct timezone tz;
	int tick;

	if(gettimeofday(&tv, &tz) < 0)
		return 0;
	if(tv.tv_sec == last_sec) {
		tick = TICK(tv.tv_usec);
		if(tick == last_run)
			return 0;
	} else
		tick = TICK(tv.tv_usec);
	last_sec = tv.tv_sec;
	last_run = tick;
	return 1;
}

void runPythonHook(char *hook)
{
	PyObject *hookobj, *arglist, *result;

	if(MUXPy_State != 1)
		return;
	hookobj = PyObject_GetAttrString(muxmod, hook);
	if(hookobj) {
		if(PyCallable_Check(hookobj)) {
			if((arglist = PyTuple_New(0)) == NULL) {
				/* Can't create/reuse empty tuple -- serious error. */
				MUXPy_Abort("Can't create empty tuple");
				return;
			}
			result = PyEval_CallObject(hookobj, arglist);
			/* XXX use result */
		} else {
			MUXPy_ReportError(-1, 1);
		}
	} else {
		/* attr not found, possibly log it ? */
		MUXPy_ReportError(-1, 1);
	}
}

void updatePython(void)
{
	if(!shouldupdate())
		return;
	runPythonHook("update");
}

void endPython(int result)
{
	MUXPy_State = 0;
	Py_Exit(result);
	/* XXX: Insert this, instead of regular exit, to all places (ugh) */
	/* NOTREACHED */
}

static PyObject *setStdout(dbref player)
{
	PyObject *stdout = MakeFakeStdout(player);

	if(stdout == NULL)
		return NULL;
	if(PySys_SetObject("stdout", stdout) == -1) {
		MUXPy_ReportError(player, 1);
		return NULL;
	}
	if(PySys_SetObject("stderr", stdout) == -1) {
		MUXPy_ReportError(player, 1);
		return NULL;
	}
	return stdout;
}

static PyObject *evalString(char *runstring)
{
	/* Run eval() on the code, and print result */
	PyObject *result = PyRun_String(runstring, Py_single_input,
									maindict, maindict);

	return result;
}

int update_whocause(void)
{
	PyObject *tmp;

	if((tmp = PyInt_FromLong((long) who)) == NULL)
		return -1;

	if(PyDict_SetItemString(maindict, "who", tmp) == -1) {
		Py_DECREF(tmp);
		return -1;
	}
	Py_DECREF(tmp);

	if((tmp = PyInt_FromLong((long) cause)) == NULL)
		return -1;
	if(PyDict_SetItemString(maindict, "cause", tmp) == -1) {
		Py_DECREF(tmp);
		return -1;
	}
	Py_DECREF(tmp);
	return 0;
}

void do_python(dbref cwho, dbref ccause, int key, char *target)
{
	PyObject *stdout, *result;
	dbref prev_cause = cause;
	dbref prev_who = who;

	if(MUXPy_State != 1)
		return;
	if(!target)
		return;
	if(*target == ',')
		target++;				/* one-letter start */
	while (*target && isspace(*target))
		target++;

	stdout = setStdout(cwho);
	if(stdout == NULL) {
		PyErr_Print();
		MUXPy_ReportError(who, 1);
		return;
	}

	cause = ccause;
	who = cwho;
	if(cause != prev_cause || who != prev_who)
		update_whocause();

	result = PyRun_String(target, Py_single_input, maindict, maindict);
	if(result == NULL) {
		if(PyErr_Occurred()) {
			PyErr_Print();
			PyErr_Clear();
		}
	}
	if(result != NULL && result != Py_None) {
		PyObject *str = PyObject_Repr(result);

		if(str != NULL) {
			raw_notify_raw(cwho, PyString_AsString(str), "\r\n");
			Py_DECREF(str);
		}
	}

	Py_XDECREF(result);
	Py_DECREF(stdout);
	cause = prev_cause;
	who = prev_who;
}

void fun_python(char *buff, char **bufc, dbref cwho, dbref ccause,
				char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char buf[LBUF_SIZE];		/* XXX: Overflow check */
	PyObject *stdout, *t;
	int eval = 1;
	char *c, *target = fargs[0];
	dbref prev_cause = cause;
	dbref prev_who = who;

	if(MUXPy_State != 1)
		return;
	if(!target)
		return;
	while (*target && isspace(*target))
		target++;
	if(!*target)
		return;

	stdout = setStdout(cwho);
	if(stdout == NULL) {
		MUXPy_ReportError(who, 1);
		return;
	}

	who = cwho;
	cause = ccause;
	if(cause != prev_cause || who != prev_who)
		update_whocause();

	if((t = evalString(target))) {
		if(t != Py_None) {
			PyObject *str;

			if((str = PyObject_Str(t))) {
				safe_str(PyString_AsString(str), buff, bufc);
				Py_DECREF(str);
			} else {
				MUXPy_ReportError(who, 1);
			}
		}
	} else {
		MUXPy_ReportError(who, 1);
	}
	Py_DECREF(stdout);
	cause = prev_cause;
	who = prev_who;
}

void fun_pythoncall(char *buff, char **bufc,
					dbref cwho, dbref ccause,
					char *fargs[], int nfargs, char *cargs[], int ncargs)
{
	char buf[LBUF_SIZE];
	char *myfargs[2];
	char *to, c;
	int i;

	/* Conveniency function that converts arguments and calls fun_python() */

	if(MUXPy_State != 1)
		return;

	if(nfargs < 1) {
		safe_str("#-1 AT LEAST ONE ARGUMENT IS REQUIRED", buff, bufc);
		return;
	}

	strcpy(buf, fargs[0]);
	to = buf + strlen(buf);
	*to++ = '(';
	for(i = 1; i < nfargs; i++) {
		char *fr = fargs[i];

		if(i > 1)
			*to++ = ',';

		*to++ = '"';
		while (*fr && (to - buf) < (LBUF_SIZE - 10)) {
			c = *fr++;
			if(c == '"')
				c = '\'';
			*to++ = c;
		}
		if((to - buf) >= (LBUF_SIZE - 10)) {
			safe_str("#-1 ARGUMENTS TOO LONG", buff, bufc);
			return;
		}
		*to++ = '"';
	}
	*to++ = ')';
	*to = 0;
	myfargs[0] = buf;
	myfargs[1] = NULL;
	fun_python(buff, bufc, cwho, ccause, myfargs, 1, cargs, ncargs);
}

/* Fake stdout, prints to player-dbref. */

staticforward PyTypeObject PyFakeStdout_Type;
typedef struct {
	PyObject_HEAD;
	dbref dbref;
} fakestdout;

static PyObject *fakestdout_write(fakestdout * self, PyObject * args)
{
	char *str;

	if(!PyArg_ParseTuple(args, "s:write", &str))
		return NULL;
	raw_notify_raw(self->dbref, str, NULL);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *MakeFakeStdout(dbref dbref)
{
	fakestdout *mo = PyObject_NEW(fakestdout, &PyFakeStdout_Type);

	if(mo == NULL)
		return NULL;
	mo->dbref = dbref;
	return (PyObject *) mo;
}

static PyMethodDef fakestdout_methods[] = {
	{"write", (PyCFunction) fakestdout_write, METH_VARARGS},
	{NULL, NULL}				/* Sentinel */
};

static PyObject *fakestdout_getattr(PyObject * self, char *name)
{
	return Py_FindMethod(fakestdout_methods, self, name);
}

static void fakestdout_dealloc(fakestdout * self)
{
	PyMem_DEL(self);
}

static PyTypeObject PyFakeStdout_Type = {

	PyObject_HEAD_INIT(&PyType_Type) 0,	/*ob_size */
	"fakestdout",				/*tp_name */
	sizeof(fakestdout),			/*tp_size */
	0,							/*tp_itemsize */
	/* methods */
	(destructor) fakestdout_dealloc,	/*tp_dealloc */
	0,							/*tp_print */
	(getattrfunc) fakestdout_getattr,	/*tp_getattr */
	0,							/*tp_setattr */
	0,							/*tp_compare */
	0,							/*tp_repr */
};

/* MUXObject */

staticforward PyTypeObject PyMUXObject_Type;

typedef struct {
	PyObject_HEAD;
	dbref dbref;
} MUXObject;

static PyObject *MUXObject_New(dbref dbref)
{
	MUXObject *mo;

	mo = PyObject_NEW(MUXObject, &PyMUXObject_Type);
	if(mo == NULL)
		return NULL;
	mo->dbref = dbref;
	return (PyObject *) mo;
}

static void MUXObject_Del(MUXObject * self)
{
	PyMem_DEL(self);
}

static PyObject *MUXObject_keys(MUXObject * self, PyObject * args)
{
	PyObject *p;
	int ca;
	char *as;
	ATTR *attr;

	if(!PyArg_ParseTuple(args, ":keys"))
		return NULL;
	p = PyList_New(0);
	PyList_Append(p, PyString_FromString("Dbref"));
	PyList_Append(p, PyString_FromString("Location"));
	for(ca = atr_head(self->dbref, &as); ca; ca = atr_next(&as)) {
		attr = atr_num(ca);
		PyList_Append(p, PyString_FromString(attr->name));
	}
	return p;
}

static PyMethodDef MUXObject_methods[] = {
	{"keys", (PyCFunction) MUXObject_keys, METH_VARARGS},
	{NULL, NULL}				/* Sentinel */
};

static PyObject *MUXObject_GetAttr(MUXObject * self, char *name)
{
	char *ptr;
	int len;
	ATTR *a;
	char *buf;
	int ao, af;					/* Attribute owner, attribute flags */
	PyObject *p;
	PyObject *v = NULL;

	if(strcasecmp(name, "location") == 0)
		return PyInt_FromLong(Location(self->dbref));

	if(strcasecmp(name, "dbref") == 0)
		return PyInt_FromLong(self->dbref);

	if(!(a = atr_str(name)))
		return Py_FindMethod(MUXObject_methods, (PyObject *) self, name);

	buf = alloc_lbuf("python_getattr");	/* XXX: Overflow check */
	atr_get_str(buf, self->dbref, a->number, &ao, &af);
	p = PyString_FromString(buf);
	free_lbuf(buf);
	return p;
}

static int MUXObject_SetAttr(MUXObject * self, char *name, PyObject * v)
{
	ATTR *atr;
	int attnum;

	atr = atr_str(name);
	if(v == NULL) {
		/* delAttr */
		if(atr) {
			atr_clr(self->dbref, atr->number);
			return 0;
		}
		PyErr_SetString(PyExc_AttributeError, "Nonexistent attribute");
		return -1;
	}
	if(!PyString_Check(v)) {
		PyErr_SetString(PyExc_ValueError,
						"Invalid value: only string accepted");
		return -1;
	}
	attnum = atr ? atr->number : mkattr(name);
	atr_add_raw(self->dbref, attnum, PyString_AsString(v));
	return 0;
}

/* Functions in MUX module */
static PyObject *muxc_getobject(PyObject * self, PyObject * args)
{
	dbref dbref;

	if(!PyArg_ParseTuple(args, "i:getobject", &dbref))
		return NULL;
	return MUXObject_New(dbref);
}

static PyObject *mux_eval(dbref dbref, char *str)
{
	char *buf = alloc_lbuf("objeval");
	char *endMarker = buf;
	PyObject *rv;

	exec(buf, &endMarker, 0, dbref, cause,
		 EV_FCHECK | EV_STRIP | EV_EVAL, &str, NULL, 0);
	*endMarker = 0;
	if(*buf)
		rv = PyString_FromString(buf);
	else {
		Py_INCREF(Py_None);
		rv = Py_None;
	}
	free_lbuf(buf);
	return rv;
}

static PyObject *muxc_muxeval(PyObject * self, PyObject * args)
{
	dbref dbref;
	char *str;

	if(!PyArg_ParseTuple(args, "is:muxeval", &dbref, &str))
		return NULL;
	return mux_eval(dbref, str);
}

static PyObject *muxc_notify(PyObject * self, PyObject * args)
{
	dbref dbref;
	char *str;

	if(!PyArg_ParseTuple(args, "is:notify", &dbref, &str))
		return NULL;
	notify(dbref, str);
	Py_INCREF(Py_None);
	return Py_None;
}

/* Initialization */ static PyTypeObject PyMUXObject_Type = {
	PyObject_HEAD_INIT(&PyType_Type) 0,	/*ob_size */
	"MUXobject",				/*tp_name */
	sizeof(MUXObject),			/*tp_basicsize */
	0,							/*tp_itemsize */
	/* methods */
	(destructor) MUXObject_Del,	/*tp_dealloc */
	0,							/*tp_print */
	(getattrfunc) MUXObject_GetAttr,	/*tp_getattr */
	(setattrfunc) MUXObject_SetAttr,	/*tp_setattr */
	0,							/*tp_compare */
	0,							/*tp_repr */
	0,							/*tp_hash (=as_number) */
};

static PyMethodDef MUXMethods[] = {
	{"getobject", muxc_getobject, METH_VARARGS},
	{"muxeval", muxc_muxeval, METH_VARARGS},
	{"notify", muxc_notify, METH_VARARGS},
	{NULL, NULL}				/* Sentinel */
};

static void init_muxmodule()
{
	PyImport_AddModule("muxc");
	Py_InitModule("muxc", MUXMethods);
}
#endif /* USE_PYTHON */
