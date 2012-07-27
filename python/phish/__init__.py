# Python wrapper on PHISH library via ctypes

import types
from ctypes import *
from cPickle import dumps,loads

# Storage for the loaded PHISH dynamic library.
_library = None

# Any time we wrap a callback function, we store it here to ensure that it
# never goes out-of-scope.
_callbacks = []

# data type defs from src/phish.h

RAW = 0
CHAR = 1
INT8 = 2
INT16 = 3
INT32 = 4
INT64 = 5
UINT8 = 6
UINT16 = 7
UINT32 = 8
UINT64 = 9
FLOAT = 10
DOUBLE = 11
STRING = 12
INT8_ARRAY = 13
INT16_ARRAY = 14
INT32_ARRAY = 15
INT64_ARRAY = 16
UINT8_ARRAY = 17
UINT16_ARRAY = 18
UINT32_ARRAY = 19
UINT64_ARRAY = 20
FLOAT_ARRAY = 21
DOUBLE_ARRAY = 22
PICKLE = 23

# max port setting from src/phish.cpp

def init(args):
  for index,argument in enumerate(args):
    if argument == "--phish-backend":
      backend = args[index+1]

      # Load the PHISH runtime library ...
      import ctypes.util
      global _library
      _library = None
      if _library is None:
        if ctypes.util.find_library("phish-%s" % backend):
          _library = ctypes.CDLL(ctypes.util.find_library("phish-%s" % backend))
      if _library is None:
        _library = ctypes.CDLL("libphish-%s.so" % backend)
      if _library is None:
        _library = ctypes.CDLL("libphish-%s.dylib" % backend)
      if _library is None:
        _library = ctypes.CDLL("libphish-%s.dll" % backend)
      if _library is None:
        raise Exception("Unable to load %s backend." % backend)

  # phish_timer() returns a double
  _library.phish_timer.restype = c_double

  argc = pointer(c_int(len(args)))

  argv = POINTER(POINTER(c_char_p))()
  argv.contents = pointer((c_char_p * len(args))(*args))
  _library.phish_init(argc, argv)
  return [argv.contents[i] for i in range(argc.contents.value)]

def exit():
  _library.phish_exit()

def input(iport,datumfunc,donefunc,reqflag):
  if datumfunc is not None:
    datumfunc = CFUNCTYPE(None, c_int)(datumfunc)
    _callbacks.append(datumfunc)

  if donefunc is not None:
    donefunc = CFUNCTYPE(None)(donefunc)
    _callbacks.append(donefunc)

  _library.phish_input(iport, datumfunc, donefunc, reqflag)

def output(iport):
  _library.phish_output(iport)

def check():
  _library.phish_check()

def callback(alldonefunc,abortfunc):
  if alldonefunc is not None:
    alldonefunc = CFUNCTYPE(None)(alldonefunc)
    _callbacks.append(alldonefunc)

  if abortfunc is not None:
    abortfunc = CFUNCTYPE(None, POINTER(c_int))(abortfunc)
    _callbacks.append(abortfunc)

  _library.phish_callback(alldonefunc, abortfunc)

def close(iport):
  _library.phish_close(iport)

# loop, probe, recv functions with callbacks

def loop():
  _library.phish_loop()

def probe(probefunc):
  probefunc = CFUNCTYPE(None)(probefunc)
  _callbacks.append(probefunc)

  _library.phish_probe(probefunc)

def recv():
  return _library.phish_recv()

# send functions

def send(iport):
  _library.phish_send(iport)

# pickle the key so can hash any Python object
# this means Python and non-Python minnows will not send
# same key (e.g. a string) to the same minnow

def send_key(iport,key):
  cobj = dumps(key,1)
  _library.phish_send_key(iport,cobj,len(cobj))

def send_direct(iport,receiver):
  _library.phish_send_direct(iport,receiver)

# error check that value is between begin (inclusive) and end (exclusive)

def check_range(begin,value,end):
  if value < begin or value >= end:
    raise Exception("Value must be in range [%s, %s)" % (begin, end))

# pack functions with range checking on int and uint values

def repack():
  _library.phish_repack()

# cstr is a char ptr to Python object as if it were string
# but user-specified len is passed to pack_raw(), not string length

def pack_raw(obj,len):
  cstr = c_char_p(obj)
  _library.phish_pack_raw(cstr,len)

def pack_char(value):
  check_range(0,value,256)
  _library.phish_pack_char(c_char(value))

def pack_int8(value):
  check_range(-128,value,128)
  _library.phish_pack_int8(c_int8(value))

def pack_int16(value):
  check_range(-32768,value,32768)
  _library.phish_pack_int16(c_int16(value))

def pack_int32(value):
  check_range(-2147483648,value,2147483648)
  _library.phish_pack_int32(c_int32(value))

def pack_int64(value):
  check_range(-9223372036854775808,value,9223372036854775808)
  _library.phish_pack_int64(c_int64(value))

def pack_uint8(value):
  check_range(0,value,256)
  _library.phish_pack_uint8(c_uint8(value))

def pack_uint16(value):
  check_range(0,value,65536)
  _library.phish_pack_uint16(c_uint16(value))

def pack_uint32(value):
  check_range(0,value,4294967296)
  _library.phish_pack_uint32(c_uint32(value))

def pack_uint64(value):
  check_range(0,value,18446744073709551616)
  _library.phish_pack_uint64(c_uint64(value))

def pack_float(value):
  _library.phish_pack_float(c_float(value))

def pack_double(value):
  _library.phish_pack_double(c_double(value))

def pack_string(str):
  cstr = c_char_p(str)
  _library.phish_pack_string(cstr)

def pack_int8_array(vec):
  n = len(vec)
  ptr = (c_int8*n)()
  for i in xrange(n):
    check_range(-128,vec[i],128)
    ptr[i] = vec[i]
  _library.phish_pack_int8_array(ptr,n)

def pack_int16_array(vec):
  n = len(vec)
  ptr = (c_int16*n)()
  for i in xrange(n):
    check_range(-32768,vec[i],32768)
    ptr[i] = vec[i]
  _library.phish_pack_int16_array(ptr,n)

def pack_int32_array(vec):
  n = len(vec)
  ptr = (c_int32*n)()
  for i in xrange(n):
    check_range(-2147483648,vec[i],2147483648)
    ptr[i] = vec[i]
  _library.phish_pack_int32_array(ptr,n)

def pack_int64_array(vec):
  n = len(vec)
  ptr = (c_int64*n)()
  for i in xrange(n):
    check_range(-9223372036854775808,vec[i],9223372036854775808)
    ptr[i] = vec[i]
  _library.phish_pack_int64_array(ptr,n)

def pack_uint8_array(vec):
  n = len(vec)
  ptr = (c_uint8*n)()
  for i in xrange(n):
    check_range(0,vec[i],256)
    ptr[i] = vec[i]
  _library.phish_pack_uint8_array(ptr,n)

def pack_uint16_array(vec):
  n = len(vec)
  ptr = (c_uint16*n)()
  for i in xrange(n):
    check_range(0,vec[i],65536)
    ptr[i] = vec[i]
  _library.phish_pack_uint16_array(ptr,n)

def pack_uint32_array(vec):
  n = len(vec)
  ptr = (c_uint32*n)()
  for i in xrange(n):
    check_range(0,vec[i],4294967296)
    ptr[i] = vec[i]
  _library.phish_pack_uint32_array(ptr,n)

def pack_uint64_array(vec):
  n = len(vec)
  ptr = (c_uint64*n)()
  for i in xrange(n):
    check_range(0,vec[i],18446744073709551616)
    ptr[i] = vec[i]
  _library.phish_pack_uint64_array(ptr,n)

def pack_float_array(vec):
  n = len(vec)
  ptr = (c_float*n)()
  for i in xrange(n): ptr[i] = vec[i]
  _library.phish_pack_float_array(ptr,n)

def pack_double_array(vec):
  n = len(vec)
  ptr = (c_double*n)()
  for i in xrange(n): ptr[i] = vec[i]
  _library.phish_pack_double_array(ptr,n)

# pickle an aribitrary Python object, to convert it to a string of bytes

def pack_pickle(obj):
  cobj = dumps(obj,1)
  _library.phish_pack_pickle(cobj,len(cobj))

# unpack based on data type

def unpack():
  buf = c_char_p()
  len = c_int32()
  type = _library.phish_unpack(byref(buf),byref(len))

  # return raw buf ptr and user-specified len
  # caller can do a ctypes cast to any POINTER

  if type == RAW:
    return type,buf,len.value

  if type == CHAR:
    ptr = cast(buf,POINTER(c_char))
    return type,ptr[0],len.value
  if type == INT8:
    ptr = cast(buf,POINTER(c_int8))
    return type,ptr[0],len.value
  if type == INT16:
    ptr = cast(buf,POINTER(c_int16))
    return type,ptr[0],len.value
  if type == INT32:
    ptr = cast(buf,POINTER(c_int32))
    return type,ptr[0],len.value
  if type == INT64:
    ptr = cast(buf,POINTER(c_int64))
    return type,ptr[0],len.value
  if type == UINT8:
    ptr = cast(buf,POINTER(c_uint8))
    return type,ptr[0],len.value
  if type == UINT16:
    ptr = cast(buf,POINTER(c_uint16))
    return type,ptr[0],len.value
  if type == UINT32:
    ptr = cast(buf,POINTER(c_uint32))
    return type,ptr[0],len.value
  if type == UINT64:
    ptr = cast(buf,POINTER(c_uint64))
    return type,ptr[0],len.value
  if type == FLOAT:
    ptr = cast(buf,POINTER(c_float))
    return type,ptr[0],len.value
  if type == DOUBLE:
    ptr = cast(buf,POINTER(c_double))
    return type,ptr[0],1
  if type == STRING:
    return type,buf.value,len.value
  if type == INT8_ARRAY:
    ptr = cast(buf,POINTER(c_int8))
    vec = len.value * [0]
    for i in xrange(len.value): vec[i] = ptr[i]
    return type,vec,len.value
  if type == INT16_ARRAY:
    ptr = cast(buf,POINTER(c_int16))
    vec = len.value * [0]
    for i in xrange(len.value): vec[i] = ptr[i]
    return type,vec,len.value
  if type == INT32_ARRAY:
    ptr = cast(buf,POINTER(c_int32))
    vec = len.value * [0]
    for i in xrange(len.value): vec[i] = ptr[i]
    return type,vec,len.value
  if type == INT64_ARRAY:
    ptr = cast(buf,POINTER(c_int64))
    vec = len.value * [0]
    for i in xrange(len.value): vec[i] = ptr[i]
    return type,vec,len.value
  if type == UINT8_ARRAY:
    ptr = cast(buf,POINTER(c_uint8))
    vec = len.value * [0]
    for i in xrange(len.value): vec[i] = ptr[i]
    return type,vec,len.value
  if type == UINT16_ARRAY:
    ptr = cast(buf,POINTER(c_uint16))
    vec = len.value * [0]
    for i in xrange(len.value): vec[i] = ptr[i]
    return type,vec,len.value
  if type == UINT32_ARRAY:
    ptr = cast(buf,POINTER(c_uint32))
    vec = len.value * [0]
    for i in xrange(len.value): vec[i] = ptr[i]
    return type,vec,len.value
  if type == UINT64_ARRAY:
    ptr = cast(buf,POINTER(c_uint64))
    vec = len.value * [0]
    for i in xrange(len.value): vec[i] = ptr[i]
    return type,vec,len.value
  if type == FLOAT_ARRAY:
    ptr = cast(buf,POINTER(c_float))
    vec = len.value * [0.0]
    for i in xrange(len.value): vec[i] = ptr[i]
    return type,vec,len.value
  if type == DOUBLE_ARRAY:
    ptr = cast(buf,POINTER(c_double))
    vec = len.value * [0.0]
    for i in xrange(len.value): vec[i] = ptr[i]
    return type,vec,len.value

  # unpickle the string of bytes back into a Python object
  # return object and length of byte string

  if type == PICKLE:
    ptr = cast(buf,POINTER(c_char))
    obj = loads(ptr[:len.value])
    return type,obj,len.value

def datum(flag):
  return _library.phish_datum(flag)

# queue functions

def queue():
  return _library.phish_queue()

def dequeue(n):
  return _library.phish_dequeue(n)

def nqueue():
  return _library.phish_nqueue()

# query/set functions

def query(str,flag1,flag2):
  return _library.phish_query(str,flag1,flag2)

def set(str,flag1,flag2):
  _library.phish_set(str,flag1,flag2)

# error functions

def error(str):
  _library.phish_error(str)

def warn(str):
  _library.phish_warn(str)

def abort():
  _library.phish_abort()

# timer function

def timer():
  return _library.phish_timer()

