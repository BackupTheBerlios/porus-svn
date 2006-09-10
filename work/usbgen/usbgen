#!/usr/bin/env python

# usbgen.py -- USB enumeration code generator

# PORUS
# Portable USB Stack
#
# (c) 2004-2006 Texas Instruments Inc.

# This file is part of PORUS.  You can redistribute and/or modify
# it under the terms of the Common Public License as published by
# IBM Corporation; either version 1.0 of the License, or
# (at your option) any later version.
#
# PORUS is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# Common Public License for more details.
#
# You should have received a copy of the Common Public License
# along with PORUS.  It may also be available at the following URL:
# 
#    http://www.opensource.org/licenses/cpl1.0.txt
# 
# If you cannot obtain a copy of the License, please contact the 
# Data Acquisition Products Applications Department at Texas 
# Instruments Inc.

VERSION='0.1.0'

import datetime,textwrap,logging,pprint

debug=logging.debug
warn=logging.warning
error=logging.error

#logging.getLogger().setLevel(logging.DEBUG)

DESCTYPE_DEVICE=1
DESCTYPE_CONFIGURATION=2
DESCTYPE_STRING=3
DESCTYPE_INTERFACE=4
DESCTYPE_ENDPOINT=5

LANGID_EN_US=0x0409
LANGID_EN_UK=0x0809

# These are valid for the VC5509/09A/07
VALID_NONISO_PACKET_SIZES=[8,16,32,64]

ENDPOINT_TYPES={
	'bulk':'USB_EPTYPE_BULK',
	'interrupt':'USB_EPTYPE_INTERRUPT',
	'isochronous':'USB_EPTYPE_ISOCHRONOUS'
}

default_device={
	'dataFormat':'u8',
	'ctlWriteBufLen':32,
	'usbRelease':'2.0',
	'classCode':0,
	'subclassCode':0,
	'protocolCode':0,
	'maxCtlPacketSize':64,
	'vendorID':None,
	'productID':0,
	'devRelease':0,
	'manufacturerDesc':'',
	'productDesc':'',
	'serialNumber':'',
	'config':None
	# assigned opts:
	# numConfigs - number of configurations
	# descriptor - descriptor array
	# headerName - name of header file
	# sourceName - name of source file
	# configFile - name of config file
}

default_config={
	'name':'',
	'selfPowered':1,
	'remoteWakeup':0,
	'interface':None
	# assigned opts:
	# numIfaces - number of interfaces
	# value - used for bConfigurationValue; starts from 1
	# descriptor - descriptor array, including iface & endpoint arrays
	# features - number appearing in the features array in the code
	# maxPower - default 500 only if selfPowered is true
}

default_iface={
	'classCode':0xff,
	'subclassCode':0xff,
	'protocolCode':0xff,
	'desc':'',
	'endpoint':tuple()
	# assigned opts:
	# numEndpoints - number of endpoints
	# number - used for bInterfaceNumber; starts from 0
	# descriptor - descriptor array, including ep arrays
}

default_ep={
	'dir':None,
	'number':None,
	'type':'bulk',
	'maxPacketSize':None,
	'pollingInterval':1,
	'sendTimeout':0
	# assigned opts:
	# descriptor - descriptor array
	# symbol - symbolic name for structs etc.
	# syncType - default 'none' only if iso
	# usageType - default 'data' only if iso
}

strings=[]
serialNumberIndex=-1
dataFormat="u8"
usb_data_t=''

# ========================================================================
# Format converters & misc
# ========================================================================

def u16len(l):
    return ((l+(l&1))//2)

def intToBCDU8(n):
    d0=n%10
    d1=n//10
    return (d1<<4)|d0

def encodeUSBRelease(rel):
    l=rel.split('.')
    if len(l)<2:
	warn("USB release has only major number; assuming 0 for minor")
	l.append('0')
    major=int(l[0])
    if (major>99) or (major<0):
	error("USB major release number must be [0,99]")
    minor=int(l[1])
    if (minor>99) or (minor<0):
	error("USB minor release number must be [0,99]")
    return [intToBCDU8(minor),intToBCDU8(major)]

def intToU8(i):
    return [i&0xff]

def intToU16(i):
    return [i&0xff,(i>>8)&0xff]

def intToBCD16(i):
    d0=i%10
    d1=(i//10)%10
    d2=(i//100)%10
    d3=i//1000
    i2=(d3<<12)|(d2<<8)|(d1<<4)|d0
    return intToU16(i2)

def checkBool(s):
    if isinstance(s,str):
	return s.lower() in ['true','yes','1']
    elif isinstance(s,int):
	if s: return True
	else: return False
    else:
	return False

# ========================================================================
# Initial tree processing
# ========================================================================

def setDefaults(d,defs):
    if isinstance(d,tuple):
	debug("setting defaults for all")
	d3=tuple()
	for d2 in d:
	    d3=d3+(setDefaults(d2,defs),)
	return d3
    for key in d.keys():
	if not key in defs.keys():
	    warn("Unknown option %s, ignoring"%key)
	    del d[key]
    d2=defs.copy()
    d2.update(d)
    d=d2
    for key,val in d.items():
	debug("let's look at key [%s]"%str(key))
	if val is None:
	    error("Option %s not provided, and it has no default"%key)
	if isinstance(val,dict) or isinstance(val,tuple):
	    if key=='config':
		debug("this is a cf:"+str(val))
		d2=setDefaults(val,default_config)
	    elif key=='interface':
		debug("this is a if:"+str(val))
		d2=setDefaults(val,default_iface)
	    elif key=='endpoint':
		debug("this is an ep:"+str(val))
		d2=setDefaults(val,default_ep)
	    else:
		# shouldn't ever get here
		error("Unknown block type"+key)
	    debug("done with subdefs: "+str(d2))
	    d[key]=d2
    return d

def setTreeDefaults(t):
    return setDefaults(t,default_device)

def assignNumbers(t):
    #debug("assignNumbers: "+str(t))
    cl=t['config']
    if not isinstance(cl,tuple):
	t['config']=(cl,)
	cl=(cl,)
    t['numConfigs']=len(cl)
    i=1
    for config in cl:
	#debug(str(config))
	config['value']=i
	i+=1
	il=config['interface']
	if not isinstance(il,tuple):
	    config['interface']=(il,)
	    il=(il,)
	config['numIfaces']=len(il)
	features=0
	if checkBool(config['selfPowered']):
	    features=1
	if checkBool(config['remoteWakeup']):
	    features=features|2
	config['features']=features
	iv=0
	for iface in il:
	    iface['number']=iv
	    iv+=1
	    el=iface['endpoint']
	    if not isinstance(el,tuple):
		iface['endpoint']=(el,)
		el=(el,)
	    iface['numEndpoints']=len(el)
	    for ep in el:
		ep['symbol']='ep'+ep['dir']+str(ep['number'])

# ========================================================================
# Byte array generation
# ========================================================================

def genDeviceArray(opts):
    global serialNumberIndex
    a=[18,DESCTYPE_DEVICE]
    a+=encodeUSBRelease(opts['usbRelease'])
    a+=intToU8(opts['classCode'])
    a+=intToU8(opts['subclassCode'])
    a+=intToU8(opts['protocolCode'])
    a+=intToU8(opts['maxCtlPacketSize'])
    a+=intToU16(opts['vendorID'])
    a+=intToU16(opts['productID'])
    a+=intToBCD16(opts['devRelease'])
    a+=mkString(opts['manufacturerDesc'])
    a+=mkString(opts['productDesc'])
    sni=mkString(opts['serialNumber'])[0]
    a+=[sni]
    if sni!=0:
	serialNumberIndex=sni
    a+=intToU8(opts['numConfigs'])
    opts['descriptor']=a

pow2=[1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768]

def genEPArray(o):
    debug(str(o))
    a=[7,DESCTYPE_ENDPOINT]
    isIn=(o['dir']=='in')
    if isIn: adr=0x80
    else: adr=0
    adr|=o['number']&15
    a+=[adr]
    s=o['type']
    att=0
    if s=='control':
	pass
    elif s=='bulk':
	att|=2
    elif s=='isochronous':
	att|=1
    elif s=='interrupt':
	att|=3
    else:
	error('%s: unknown transfer type %s'%(name,s))
    if s=='isochronous':
	if not o.has_key('syncType'):
	    o['syncType']='none'
	if not o.has_key('usageType'):
	    o['usageType']='data'
	s=o['syncType']
	if s=='none':
	    pass
	elif s=='async':
	    att|=4
	elif s=='adaptive':
	    att|=8
	elif s=='sync':
	    att|=12
	else:
	    error('%s: unknown synchronisation type %s'%(name,s))
	s=o['usageType']
	if s=='data':
	    pass
	elif s=='feedback':
	    att|=0x10
	elif s=='implicit':
	    att|=0x20
	else:
	    error('%s: unknown usage type %s'%(name,s))
    else:
	if o.has_key('syncType'):
	    warn('%s: not isochronous, so option syncType ignored'%o['symbol'])
	if o.has_key('usageType'):
	    warn('%s: not isochronous, so option usageType ignored'%o['symbol'])
    a+=[att]
    i=o['maxPacketSize']
    if o['type']=='isochronous' and i>1023:
	error('%s: maxPacketSize can be no greater than 1023 for isochronous packets'%name)
    elif not i in VALID_NONISO_PACKET_SIZES:
	error('%s: maxPacketSize must be in %s'%(name,str(VALID_NONISO_PACKET_SIZES)))
    a+=intToU16(i)
    i=o['pollingInterval']
    if i<0:
	error('%s: negative polling interval?! umm .. no.'%name)
    if i<1:
	error('%s: polling interval may not be zero'%name)
    if o['type']=='isochronous':
	if (i>32768):
	    error('%s: for isochronous endpoints, polling interval must be power of 2 <= 32768'%name)
	i2=0
	for p in pow2:
	    if i==p: break
	    i2+=1
	if i2>=len(pow2):
	    error('%s: for isochronous endpoints, polling interval must be power of 2 <= 32768'%name)
	i=i2
    else:
	if i>255:
	    error('%s: polling interval must be <= 255'%name)
    a+=[i]
    o['descriptor']=a

def genIfaceArray(opts):
    for ep in opts['endpoint']:
	genEPArray(ep)
    a=[9,DESCTYPE_INTERFACE]
    a+=intToU8(opts['number'])
    a+=[0]
    a+=intToU8(opts['numEndpoints'])
    a+=intToU8(opts['classCode'])
    a+=intToU8(opts['subclassCode'])
    a+=intToU8(opts['protocolCode'])
    a+=mkString(opts['desc'])
    for ep in opts['endpoint']:
	a+=ep['descriptor']
    opts['descriptor']=a

def genConfigArray(o):
    totalLen=0
    for iface in o['interface']:
	genIfaceArray(iface)
	totalLen+=len(iface['descriptor'])
    a=[9,DESCTYPE_CONFIGURATION]
    a+=intToU16(totalLen+9)
    a+=intToU8(o['numIfaces'])
    a+=intToU8(o['value'])
    a+=mkString(o['name'])
    b=0x80
    selfPowered=False
    if checkBool(o['selfPowered']):
	selfPowered=True
	b|=0x40
    if checkBool(o['remoteWakeup']):
	b|=0x20
    a+=[b]
    if not selfPowered:
	if not o.has_key('maxPower'):
	    mp=500
	    warn('config %d: maxPower not set, but device is bus powered; using 500 mA'%o['value'])
	else:
	    mp=o['maxPower']
	if mp>500:
	    error('%s: maxPower too large: max is 500'%name)
	if mp<0:
	    error('%s: maxPower is negative!'%name)
	if mp==0:
	    warn('config %d: maxPower is zero, but device is bus powered'%o['value'])
	a+=[mp//2]
    if selfPowered:
	if o.has_key('maxPower'):
	    warn("config %d: device is not bus powered; maxPower option ignored"%o['value'])
	a+=[0]
    for iface in o['interface']:
	a+=iface['descriptor']
    o['descriptor']=a

def genConfigArrays(o):
    for config in o['config']:
	genConfigArray(config)

# ========================================================================
# Code generation utilities
# ========================================================================

def genCByteArrayConstantU8(name,bytes):
    l=len(bytes)
    s="static const usb_data_t %s[%d]={\n"%(name,len(bytes)+2)
    bytes=[(l>>8)&0xff,l&0xff]+bytes
    i=0
    a=[]
    for b in bytes:
	a.append("0x%02.2X"%b)
	i+=1
	if i%8==0:
	    s+='\t'+', '.join(a)
	    if i<(l+2):
		s+=',\n'
	    a=[]
    if len(a)>0:
	s+='\t'+', '.join(a)
    s+='\n};'
    return s

def genCByteArrayConstantU16(name,bytes):
    debug('gen u16: '+str(bytes))
    l=len(bytes)
    l16=u16len(len(bytes))+1
    s="static const usb_data_t %s[%d]={\n"%(name,l16)
    b2=[]
    i=0
    while i<l:
	if (i+1)<l:
	    b2.append((bytes[i]<<8)|bytes[i+1])
	else:
	    b2.append(bytes[i]<<8)
	i+=2
    #bytes=[((l&0xff)<<8)|((l>>8)&0xff)]+b2
    bytes=[l]+b2
    i=0
    a=[]
    for b in bytes:
	a.append("0x%04.4X"%b)
	i+=1
	if i%4==0:
	    s+='\t'+', '.join(a)
	    if i<l16:
		s+=',\n'
	    a=[]
    if len(a)>0:
	s+='\t'+', '.join(a)
    s+='\n};'
    return s

def genCByteArrayConstant(name,bytes):
    global dataFormat
    if dataFormat=='u8': return genCByteArrayConstantU8(name,bytes)
    elif dataFormat=='u16': return genCByteArrayConstantU16(name,bytes)
    else: return ''

def genPointerArray(name,items,typ='static const usb_data_t'):
    out='%s *%s[%d]={\n'%(typ,name,len(items))
    l=', '.join(items)
    tw=textwrap.TextWrapper(initial_indent='\t',subsequent_indent='\t')
    out+=tw.fill(l)
    out+='\n};'
    return out

# ========================================================================
# String descriptors
# ========================================================================

def genDescString(name,text):
    a=[len(text)*2+2,DESCTYPE_STRING]
    for c in text:
	a.append(ord(c))
	a.append(0)
    return genCByteArrayConstant(name,a)

def genLangTable(name,langs=[LANGID_EN_US]):
    """langs is an array of language codes"""
    a=[len(langs)+2,DESCTYPE_STRING]
    for l in langs:
	a.append(l&0xff)
	a.append((l>>8)&0xff)
    return genCByteArrayConstant(name,a)

def mkString(s):
    global strings
    if s=='': return [0]
    i=0
    for s2 in strings:
	if s2==s: break
	i+=1
    if i>=len(strings):
	strings.append(s)
    #debug("made string [%s] index %d"%(s,i+1))
    return [i+1,]

def genStringDescs(opts):
    global strings
    if len(strings)==0:
	return '',0
    i=1;
    out=''
    strtbl=['langtbl']
    for s in strings:
	out+='/* "%s" */\n'%s
	strname='string'+str(i)
	out+=genDescString(strname,s)+'\n\n'
	strtbl.append(strname)
	i+=1
    out+=genLangTable('langtbl')+'\n\n'
    out+=genPointerArray('string_descs',strtbl)
    return out,len(strings)+1

# ========================================================================
# Main code generation
# ========================================================================

def printHeader(config):
    global dataFormat,usb_data_t
    if config.has_key("serialNumber"):
	sn=True
    else:
	sn=False
    if usb_data_t=='unsigned short':
	usbMemLen='((l)>>1)'
    else:
	usbMemLen='(l)'
    substs={'version':VERSION,'date':datetime.datetime.now().ctime(),
    	'usb_data_t':usb_data_t, 'configFile':config['configFile'],
	'maxCtlPacketSize':config['maxCtlPacketSize'],
	'ctlWriteBufLen':config['ctlWriteBufLen'],
	'usbMemLen':usbMemLen,
	'bufLenSize':1} ### FIXME: This needs to be configurable per target
    print """
#ifndef GUARD_USB_DESC_GENERATED_H
#define GUARD_USB_DESC_GENERATED_H

/*
   *** DO NOT EDIT THIS FILE ***
   This is an automatically generated file.
   Any edits you make will be lost if the file is regenerated.
   *** DO NOT EDIT THIS FILE ***
*/

/*
   Generated by usbdescgen %(version)s
   from %(configFile)s on %(date)s
*/

typedef %(usb_data_t)s usb_data_t;

#define USB_BUF_LEN_SIZE %(bufLenSize)d
#define USB_CTL_PACKET_SIZE %(maxCtlPacketSize)d
#define USB_CTL_WRITE_BUF_SIZE %(ctlWriteBufLen)d
#define usb_mem_len(l) %(usbMemLen)s
#define usb_buf_len(buf) (buf[0])
#define usb_buf_set_len(buf,len) buf[0]=len
#define usb_buf_data(buf) (buf+1)

#include "usbtypes.h"

void usb_get_device_desc(usb_data_t **bytes, int *len);
int usb_get_config_desc(unsigned int index, usb_data_t **bytes, int *len);
int usb_get_string_desc(unsigned int index, unsigned short langid, usb_data_t **bytes, int *len);
int usb_have_config(unsigned int config);
int usb_have_iface(unsigned int config, unsigned int iface);
usb_endpoint_t *usb_get_ep(unsigned int config, unsigned int ep);
usb_endpoint_t *usb_get_first_ep(unsigned int config);
/* bit 0: self powered; bit 1: remote wakeup */
int usb_config_features(unsigned int config);"""%substs
    if sn:
	print """void usb_set_serial_number(usb_data_t *bytes);"""
    print """
#endif
"""

def genEPStruct(opts,nextlink):
    global ENDPOINT_TYPES
    number=opts['number']
    if not opts['type'] in ENDPOINT_TYPES:
	error("Unknown transfer type %s in endpoint %d"%(opts['type'],number))
    else:
	typ=ENDPOINT_TYPES[opts['type']]
    epname=opts['symbol']
    datastruct=epname+'_data';
    if opts['dir']=='in':
	number+=16
    substs={'name':epname,'epid':number,'eptype':typ,
    	'pktsize':opts['maxPacketSize'],
	'datastruct':datastruct,
	'sendtimeout':opts['sendTimeout'],
	'nextlink':nextlink}
    return """usb_endpoint_data_t %(datastruct)s;

static const usb_endpoint_t %(name)s={
	%(epid)s,
	%(eptype)s,
	%(pktsize)s,
	&%(datastruct)s,
	%(sendtimeout)s,
	(usb_endpoint_t *)(%(nextlink)s)
};"""%substs

def genEPConfigStructs(opts):
    cf=opts['config'][0]
    epStructs=[]
    epArray=['0']*32
    nextlink='0'
    for iface in cf['interface']:
	for ep in reversed(iface['endpoint']):
	    epStructs.append(genEPStruct(ep,nextlink))
	    epn=ep['number']
	    if ep['dir']=='in':
		epn+=16
	    epArray[epn]='&'+ep['symbol']
	    nextlink='&'+ep['symbol']
    s='\n\n'.join(epStructs)
    s+='\n\n'+genPointerArray('endpoints',epArray,'static const usb_endpoint_t')
    s+='\n\nstatic const usb_endpoint_t *first_endpoint='+nextlink+';'
    return s

def genConfigDescs(opts):
    configConsts=[]
    configFeatures=[]
    ifaceCounts=[]
    for config in opts['config']:
	configName='config'+str(config['value'])
	configConsts.append(genCByteArrayConstant(configName,config['descriptor']))
	configFeatures.append(str(config['features']))
	ifaceCounts.append(str(config['numIfaces']))
    s='\n\n'.join(configConsts)
    s+='\n\nstatic const unsigned int iface_counts[%d]={\n\t'%opts['numConfigs']
    s+=','.join(ifaceCounts)
    s+='\n};\n\nstatic const unsigned int config_features[%d]={\n\t'%opts['numConfigs']
    s+=','.join(configFeatures)
    s+='\n};'
    return s,opts['numConfigs']

def genDeviceDesc(opts):
    return genCByteArrayConstant('device_desc',opts['descriptor'])

def printSource(opts):
    global strings, serialNumberIndex, usb_data_t
    devDesc=genDeviceDesc(opts)
    configDescs,configDescCount=genConfigDescs(opts)
    stringDescs,stringDescCount=genStringDescs(opts)
    epConfigs=genEPConfigStructs(opts)
    if dataFormat=='u16':
	dataOffset=1
	ucw=u16len(opts['ctlWriteBufLen'])
    else:
	dataOffset=2
	ucw=opts['ctlWriteBufLen']
    substs={'version':VERSION,'date':datetime.datetime.now().ctime(),
    	'configDescCount':configDescCount,
	'stringDescCount':stringDescCount,
	'dataOffset':dataOffset,
	'langid':'%04.4x'%LANGID_EN_US,
	'usb_data_t':usb_data_t,
	'headerName':opts['headerName'],
	'configFile':opts['configFile'],
	'ctlWriteBufUnits':ucw}
    if serialNumberIndex!=-1:
	substs['serialNumberIndex']=serialNumberIndex
    print """
/*
   *** DO NOT EDIT THIS FILE ***
   This is an automatically generated file.
   Any edits you make will be lost if the file is regenerated.
   *** DO NOT EDIT THIS FILE ***
*/

/*
   Generated by usbdescgen %(version)s
   from %(configFile)s on %(date)s
*/

#include "%(headerName)s"

#define CONFIG_DESC_COUNT %(configDescCount)d
#define STRING_DESC_COUNT %(stringDescCount)d
#define ONLY_LANG_ID 0x%(langid)s"""%substs
    if serialNumberIndex!=-1:
	print """
#define SERIAL_NUMBER_INDEX %(serialNumberIndex)d"""%substs
    print
    print stringDescs
    print
    if serialNumberIndex!=-1:
	print """static usb_data_t *serial_number_string_ptr=(usb_data_t *)string%(serialNumberIndex)d;
"""%substs
    print configDescs
    print
    print devDesc
    print
    print epConfigs
    print """
usb_data_t usb_ctl_write_data[%(ctlWriteBufUnits)d];

static int get_len(usb_data_t *bytes)
{"""%substs
    if dataFormat=="u16":
	print """\treturn (int)(*bytes);"""
    else:
	print """\treturn (int)((bytes[0]<<8)|bytes[1]);"""
    print """}

void usb_get_device_desc(usb_data_t **bytes, int *len)
{
	*bytes=(usb_data_t *)(device_desc+%(dataOffset)d);
	*len=get_len((usb_data_t *)device_desc);
}

int usb_get_config_desc(unsigned int index, usb_data_t **bytes, int *len)
{
	usb_data_t *data;

	if (index>=CONFIG_DESC_COUNT) return -1;
	data=(usb_data_t *)config1;
	*len=get_len(data);
	*bytes=data+%(dataOffset)d;
	return 0;
}

usb_endpoint_t *usb_get_ep(unsigned int config, unsigned int ep)
{
	if (!usb_have_config(config)) return 0;
	if (ep>31) return 0;
	return (usb_endpoint_t *)(endpoints[ep]);
}

usb_endpoint_t *usb_get_first_ep(unsigned int config)
{
	if (!usb_have_config(config)) return 0;
	return (usb_endpoint_t *)first_endpoint;
}

int usb_have_config(unsigned int config)
{
	if (config>CONFIG_DESC_COUNT) return 0;
	return 1;
}

int usb_config_features(unsigned int config)
{
	if (!usb_have_config(config)) return 0;
	return config_features[config-1];
}

int usb_have_iface(unsigned int config, unsigned int iface)
{
	if (!usb_have_config(config)) return 0;
	return (iface>iface_counts[config-1])?0:1;
}"""%substs
    if stringDescCount==0: return
    print """
int usb_get_string_desc(unsigned int index, unsigned short langid, usb_data_t **bytes, int *len)
{
	usb_data_t *data;"""%substs
    if serialNumberIndex!=-1:
	print """
	if (index==SERIAL_NUMBER_INDEX) {
		data=(usb_data_t *)serial_number_string_ptr;
	} else if (!index) {
		data=(usb_data_t *)langtbl;""",
    else:
	print """
	if (!index) {
		data=(usb_data_t *)langtbl;""",
    print """
	} else {
		if (index>=STRING_DESC_COUNT) return -1;
		if (langid!=ONLY_LANG_ID) return -1;
		data=(usb_data_t *)string_descs[index];
	}
	*len=get_len((usb_data_t *)data);
	*bytes=(usb_data_t *)(data+%(dataOffset)d);
	return 0;
}"""%substs
    if serialNumberIndex!=-1:
	print """
void usb_set_serial_number(usb_data_t *bytes)
{
	serial_number_string_ptr=bytes;
}
"""

# ========================================================================
# Command-line interface
# ========================================================================

from optparse import OptionParser
import sys,cnfparse

p=OptionParser(usage="%prog [options] configfile")
p.add_option('-o','--output',default='usbconfig',
	help='Basename of output files')
p.add_option('--header',
	help="Name for header file; overrides -o")
p.add_option('--source',
	help="Name for source file; overrides -o")
p.add_option('-H',dest="header_stdout",action="store_true",
	help="Print generated header on stdout")
p.add_option('-S',dest="source_stdout",action="store_true",
	help="Print generated source on stdout")

(opts,args)=p.parse_args()

if len(args)!=1:
    p.print_help()
    sys.exit(1)

if opts.header is None:
    opts.header=opts.output+'.h'
if opts.source is None:
    opts.source=opts.output+'.c'

stdoutOnly=opts.header_stdout or opts.source_stdout

tree=cnfparse.parseFile(args[0])
tree=setTreeDefaults(tree)
dataFormat=tree['dataFormat']
assignNumbers(tree)
tree['headerName']=opts.header
tree['sourceName']=opts.source
tree['configFile']=args[0]
genConfigArrays(tree)
genDeviceArray(tree)

if dataFormat=="u16":
    usb_data_t="unsigned short"
elif dataFormat=="u8":
    usb_data_t="unsigned char"
else:
    error("Unknown data format"+dataFormat)

if opts.header_stdout:
    printHeader(tree)
    if not opts.source_stdout:
	sys.exit(0)
elif not stdoutOnly:
    f=open(opts.header,'w')
    sys.stdout=f
    printHeader(tree)
    sys.stdout=sys.__stdout__
    f.close()

if opts.source_stdout:
    printSource(tree)
elif not stdoutOnly:
    f=open(opts.source,'w')
    sys.stdout=f
    printSource(tree)
    sys.stdout=sys.__stdout__
    f.close()
