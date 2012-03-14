# Python wrapper on PHISH library via ctypes

import types
from ctypes import *
from cPickle import dumps,loads

# same data type defs as in src/phish.h

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

MAXPORT = 16

# load PHISH C++ library

try:
  lib = CDLL("_phish.so")
except:
  try:
    lib = CDLL("libphish-zmq.dylib")
  except:
    raise StandardError,"Could not load PHISH dynamic library"

# function defs
# one-to-one match to functions in src/phish.h

def init(args):
  narg = len(args)
  cargs = (c_char_p*narg)(*args)
  n = lib.phish_init_python(narg,cargs)
  return args[n:]

def exit():
  lib.phish_exit()

# clunky: using named callback for each port
# Tim says to use closures to generate callback function code
    
def input(iport,datumfunc,donefunc,reqflag):
  global datum0_caller,done0_caller
  global datum1_caller,done1_caller
  if iport == 0:
    datum0_caller = datumfunc
    done0_caller = donefunc
    if datumfunc and donefunc:
      lib.phish_input(iport,datum0_def,done0_def,reqflag)
    elif datumfunc:
      lib.phish_input(iport,datum0_def,None,reqflag)
    elif donefunc:
      lib.phish_input(iport,None,done0_def,reqflag)
    else:
      lib.phish_input(iport,None,None,reqflag)
  if iport == 1:
    datum1_caller = datumfunc
    done1_caller = donefunc
    if datumfunc and donefunc:
      lib.phish_input(iport,datum1_def,done1_def,reqflag)
    elif datumfunc:
      lib.phish_input(iport,datum1_def,None,reqflag)
    elif donefunc:
      lib.phish_input(iport,None,done1_def,reqflag)
    else:
      lib.phish_input(iport,None,None,reqflag)
    
def output(iport):
  lib.phish_output(iport)

def check():
  lib.phish_check()

def done(donefunc):
  global alldone_caller
  alldone_caller = donefunc
  if donefunc: lib.phish_done(alldone_def)
  else: lib.phish_done(None)

def alldone_callback():
  alldone_caller()

def close(iport):
  lib.phish_close(iport)

def loop():
  lib.phish_loop()

def probe(probefunc):
  global probe_caller
  probe_caller = probefunc
  lib.phish_probe(probe_def)

def probe_callback():
  probe_caller()

def datum0_callback(nvalues):
  datum0_caller(nvalues)

def done0_callback():
  done0_caller()

def datum1_callback(nvalues):
  datum1_caller(nvalues)

def done1_callback():
  done1_caller()

def recv():
  return lib.phish_recv()
  
def send(iport):
  lib.phish_send(iport)

# pickle the key so can hash any Python object
  
def send_key(iport,key):
  cobj = dumps(key,1)
  lib.phish_send_key(iport,cobj,len(cobj))

def send_direct(iport,receiver):
  lib.phish_send_direct(iport,receiver)

def reset_receiver(iport,receiver):
  lib.phish_reset_receiver(iport,receiver)
  
def pack_datum(ptr,len):
  lib.phish_pack_datum(ptr,len)

# cstr will be a char ptr to Python object as it were string
# but pass user-specified len to pack_raw(), not string length

def pack_raw(obj,len):
  cstr = c_char_p(obj)
  lib.phish_pack_raw(cstr,len)

def check_range(begin, value, end):
  if value < begin or value >= end:
    raise Exception("Value must be in range [%s, %s)" % (begin, end))

def pack_char(value):
  check_range(0, value, 256)
  lib.phish_pack_char(c_char(value))

def pack_int8(value):
  check_range(-128, value, 128)
  lib.phish_pack_int8(c_int8(value))

def pack_int16(value):
  check_range(-32768, value, 32768)
  lib.phish_pack_int16(c_int16(value))

def pack_int32(value):
  check_range(-2147483648, value, 2147483648)
  lib.phish_pack_int32(c_int32(value))

def pack_int64(value):
  check_range(-9223372036854775808, value, 9223372036854775808)
  lib.phish_pack_int64(c_int64(value))

def pack_uint8(value):
  check_range(0, value, 256)
  lib.phish_pack_uint8(c_uint8(value))

def pack_uint16(value):
  check_range(0, value, 65536)
  lib.phish_pack_uint16(c_uint16(value))

def pack_uint32(value):
  check_range(0, value, 4294967296)
  lib.phish_pack_uint32(c_uint32(value))

def pack_uint64(value):
  check_range(0, value, 18446744073709551616)
  lib.phish_pack_uint64(c_uint64(value))

def pack_float(value):
  lib.phish_pack_float(c_float(value))

def pack_double(value):
  lib.phish_pack_double(c_double(value))

def pack_string(str):
  cstr = c_char_p(str)
  lib.phish_pack_string(cstr)

def pack_int8_array(vec):
  n = len(vec)
  ptr = (c_int8*n)()
  for i in xrange(n): ptr[i] = vec[i]
  lib.phish_pack_int8_array(ptr,n)

def pack_int16_array(vec):
  n = len(vec)
  ptr = (c_int16*n)()
  for i in xrange(n): ptr[i] = vec[i]
  lib.phish_pack_int16_array(ptr,n)

def pack_int32_array(vec):
  n = len(vec)
  ptr = (c_int32*n)()
  for i in xrange(n): ptr[i] = vec[i]
  lib.phish_pack_int32_array(ptr,n)

def pack_int64_array(vec):
  n = len(vec)
  ptr = (c_int64*n)()
  for i in xrange(n): ptr[i] = vec[i]
  lib.phish_pack_int64_array(ptr,n)

def pack_uint8_array(vec):
  n = len(vec)
  ptr = (c_uint8*n)()
  for i in xrange(n): ptr[i] = vec[i]
  lib.phish_pack_uint8_array(ptr,n)

def pack_uint16_array(vec):
  n = len(vec)
  ptr = (c_uint16*n)()
  for i in xrange(n): ptr[i] = vec[i]
  lib.phish_pack_uint16_array(ptr,n)

def pack_uint32_array(vec):
  n = len(vec)
  ptr = (c_uint32*n)()
  for i in xrange(n): ptr[i] = vec[i]
  lib.phish_pack_uint32_array(ptr,n)

def pack_uint64_array(vec):
  n = len(vec)
  ptr = (c_uint64*n)()
  for i in xrange(n): ptr[i] = vec[i]
  lib.phish_pack_uint64_array(ptr,n)

def pack_float_array(vec):
  n = len(vec)
  ptr = (c_float*n)()
  for i in xrange(n): ptr[i] = vec[i]
  lib.phish_pack_float_array(ptr,n)

def pack_double_array(vec):
  n = len(vec)
  ptr = (c_double*n)()
  for i in xrange(n): ptr[i] = vec[i]
  lib.phish_pack_double_array(ptr,n)

# pickle the Python object which converts it to a string of bytes
  
def pack_pickle(obj):
  cobj = dumps(obj,1)
  lib.phish_pack_pickle(cobj,len(cobj))

def unpack():
  buf = c_char_p()
  len = c_int()
  type = lib.phish_unpack(byref(buf),byref(len))
  
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
  
def datum():
  buf = c_char_p()
  len = c_int()
  iport = lib.phish_datum(byref(buf),byref(len))
  return iport,buf,len.value

def query(str,flag1,flag2):
  cstr = c_char_p(str)
  return lib.phish_query(cstr,flag1,flag2)

def error(str):
  lib.phish_error(str)

def warn(str):
  lib.phish_warn(str)

def timer():
  return lib.phish_timer()

# define other PHISH module variables

# callback functions

ALLDONEFUNC = CFUNCTYPE(c_void_p)
alldone_def = ALLDONEFUNC(alldone_callback)
    
DATUMFUNC = CFUNCTYPE(c_void_p,c_int)
datum0_def = DATUMFUNC(datum0_callback)
datum1_def = DATUMFUNC(datum1_callback)
DONEFUNC = CFUNCTYPE(c_void_p)
done0_def = DONEFUNC(done0_callback)
done1_def = DONEFUNC(done1_callback)
  
PROBEFUNC = CFUNCTYPE(c_void_p)
probe_def = PROBEFUNC(probe_callback)
