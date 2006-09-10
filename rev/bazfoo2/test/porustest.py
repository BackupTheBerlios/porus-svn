
import usb
import cmd, shlex
import sys
import zlib,binascii,struct
import random,time

endpointTypeNames={
    usb.ENDPOINT_TYPE_BULK:'Bulk',
    usb.ENDPOINT_TYPE_CONTROL:'Control',
    usb.ENDPOINT_TYPE_INTERRUPT:'Interrupt',
    usb.ENDPOINT_TYPE_ISOCHRONOUS:'Isochronous'
}

porusLongStatCodes={
    0:'Idle',
    1:'Receiving Bulk OUT data',
    2:'Calculating Bulk OUT CRC',
    3:'Generating Bulk IN test data / calculating CRC',
    4:'Transmitting Bulk In test data'
}

porusShortStatCodes={
    0:'IDLE',
    1:'BORX',
    2:'BOCC',
    3:'BICC',
    4:'BITX',
    5:'TIMO',
    6:'EROR'
}

indentLevel=0

def iprint(s):
    print indentLevel*' '+s

def toUns(l):
    def w(n):
	if n<0: n+=256
	return n
    return map(w,l)

def tupleToStr(t):
    s=''
    for i in t:
	if i<0: i+=256
	s+=chr(i)
    return s

def toUns32(i):
    return struct.unpack('!L',struct.pack('!L',i))[0]
    
def uhex32(n):
    return binascii.hexlify(struct.pack('!L',n))

def rndstring(bc):
    a=''
    while bc:
	a+=chr(random.randint(0,255))
	bc-=1
    return a

def getdevs():
    """Returns a flat array of configs on all busses -- ie an 
    array of tuples of (bus,dev,conf).  Array is guaranteed to be 
    sorted by bus and dev.  Used for giving numbers to configs"""
    rtn=[]
    for b in usb.busses():
	for d in b.devices:
	    for c in d.configurations:
		rtn.append((b,d,c))
    return rtn

def porusSTAT(devh):
    buf=devh.controlMsg(0xC1, 5, 1)[0]
    if buf<0: buf+=256
    return buf

def porusRCRC(devh):
    buf=toUns(devh.controlMsg(0xC1, 6, 4))
    var=buf[0]<<24|buf[1]<<16|buf[2]<<8|buf[3]
    return var

def porusBLKO(devh,length):
    devh.controlMsg(0x41,4,[],length)

def porusBLKI(devh,length):
    devh.controlMsg(0x41,3,[],length)

def porusTMOI(devh,length,c):
    devh.controlMsg(0x41,7,[c],length)

def getDeviceClassName(devcls):
    names={0:'interface',
    	9:'hub',
	0xdc:'diagnostic',
	0xe0:'wireless',
	0xef:'misc',
	0xff:'vendor'}
    if names.has_key(devcls):
	return names[devcls]
    else:
	return '%02X'%devcls

def getInterfaceClassName(ifcls):
    names={
    	1:'audio',
	2:'cdc control',
	3:'hid',
	5:'physical',
	6:'image',
	7:'printer',
	8:'mass storage',
	9:'hub',
	10:'cdc data',
	11:'smartcard',
	13:'content security',
	14:'video',
	0xdc:'diagnostic',
	0xe0:'wireless',
	0xef:'misc',
	0xff:'vendor'}
    if names.has_key(ifcls):
	return names[ifcls]
    else:
	return '%02X'%ifcls

def shortlist(devs):
    """Lists devices on busses in short form with numbers"""
    lastbus=None
    lastdev=None
    index=0
    for t in devs:
	if lastbus!=t[0]:
	    lastbus=t[0]
	    print
	    print "-------- bus %s --------"%(lastbus.location)
	    print
	d=t[1]
	c=t[2]
	print "% 2d: mfr [%04X] pid [%04X]"%(index,d.idVendor,d.idProduct),
	if d.deviceClass:
	    clsname=getDeviceClassName(d.deviceClass)
	    print "class %d (%s)"%(d.deviceClass,clsname),
	print "-- config %d"%c.value
	for ia in c.interfaces:
	    for i in ia:
		clsname=getInterfaceClassName(i.interfaceClass)
		print "    iface %d: class [%s] subclass [%d] -- %d endpoints"%(
			i.interfaceNumber,clsname,i.interfaceSubClass,
			len(i.endpoints))
	index+=1
    print

def printLongEndpointInfo(ep):
    global endpointTypeNames
    if ep.address&0x80:
	epdir='IN'
    else:
	epdir='OUT'
    iprint("%s endpoint %d"%(epdir,ep.address&15))
    iprint("  Type: %s"%endpointTypeNames[ep.type])
    iprint("  Interval: %d"%ep.interval)
    iprint("  Maximum packet size: %d"%ep.maxPacketSize)

def printLongInterfaceInfo(iface):
    global indentLevel
    iprint("Interface %d, setting %d"%(iface.interfaceNumber,iface.alternateSetting))
    if iface.interfaceClass:
	clsname=getInterfaceClassName(iface.interfaceClass)
	iprint("  Class: %02X (%s)"%(iface.interfaceClass,clsname))
	iprint("  Subclass: %02X"%iface.interfaceSubClass)
	iprint("  Protocol: %02X"%iface.interfaceProtocol)
    iprint("  Endpoints: %d"%len(iface.endpoints))
    indentLevel+=2
    for e in iface.endpoints:
	print
	printLongEndpointInfo(e)
    indentLevel-=2

def getDescString(devh,i,length=40):
    try:
	return devh.getString(i,length)
    except:
	return '<error>'

def printLongInfo(devrec,prefix=''):
    "devrec = (bus,dev,conf,devh)"
    global indentLevel
    bus=devrec[0]
    dev=devrec[1]
    devh=devrec[3]
    conf=devrec[2]
    print
    iprint("Device on bus [%s], configuration %d"%(bus.location,conf.value))
    print
    iprint("Device information")
    iprint("  Vendor %04X, product %04X"%(dev.idVendor,dev.idProduct))
    iprint("  Complies with: USB %s"%dev.usbVersion)
    iprint("  Manufacturer: [%s]"%getDescString(devh,dev.iManufacturer))
    iprint("  Product: [%s]"%getDescString(devh,dev.iProduct))
    iprint("  Version: [%s]"%dev.deviceVersion)
    iprint("  Serial: [%s]"%getDescString(devh,dev.iSerialNumber))
    clsname=getDeviceClassName(dev.deviceClass)
    iprint("  Class: %02X (%s)"%(dev.deviceClass,clsname))
    iprint("  Subclass: %02X"%dev.deviceSubClass)
    iprint("  Protocol: %d"%dev.deviceProtocol)
    iprint("  File name: [%s]"%dev.filename)
    iprint("  Maximum control packet size: %d bytes"%dev.maxPacketSize)
    print
    iprint("Configuration information")
    iprint("  Description: [%s]"%getDescString(devh,conf.iConfiguration))
    if conf.selfPowered:
	iprint("  Uses bus power: no")
    else:
	iprint("  Uses bus power: yes, up to %d mA"%conf.maxPower)
    if conf.remoteWakeup:
	s='yes'
    else:
	s='no'
    iprint("  Remote wakeup capable: "+s)
    iprint("  Interfaces: %d"%len(conf.interfaces))
    print
    indentLevel+=2
    for i in conf.interfaces:
	for setting in i:
	    printLongInterfaceInfo(setting)
    print
    indentLevel-=2

class usbsh(cmd.Cmd):
    def __init__(self,msg):
	self.devs=None
	self.devh=None # handle of open device
	self.curdev=None # (bus,dev,conf) of open device
	self.devn=None # index of open device
	cmd.Cmd.__init__(self,msg)

    def help_ls(self):
	print """ls [dev]

With no argument, scan the USB busses and list the devices on them.

Each device is given a number, which can be used to open ('open') or 
get information on ('info') the device.  If the device has more than 
one configuration, the device is assigned a different number for each 
configuration.

If a device is open, the busses are not scanned: the list is reused.
"""

    def help_open(self):
	print """open <dev>

Open the device given.  Devices are identified by number.  To see 
available device numbers, use ls."""

    def help_close(self):
	print """close

Closes the currently open device, if any."""

    def help_info(self):
	print """info [dev]

Prints complete information on the open device, or on the device 
specified."""

    def help_tx(self):
	print """tx <ep> < <path>

Transmit the file at <path> to the endpoint <ep> in its entirety."""

    def help_rx(self):
	print """rx [-n] [-t timeout] <ep> [path]
	
Receive data from <ep> and append it to <path>, stopping when a 
short packet is received.  If -n is given, erase <path> first.

If [path] is omitted, the data is displayed on screen in hex-dump format.

-t is used to specify a timeout in seconds.  The default is three seconds."""

    def help_stat(self):
	print """stat

Queries a PORUS test device with STAT.  Prints the numeric and text result."""

    def help_rcrc(self):
	print """rcrc
	
Queries a PORUS test device with RCRC.  Prints the hex result."""

    def help_blki(self):
	print """blki <len>

Performs the PORUS BLKI test with <len> bytes.  Prints status messages."""

    def help_blko(self):
	print """blko <len>
	
Performs the PORUS BLKO test with <len> bytes.  Prints status messages."""

    def help_quit(self):
	print """q, quit

Quit.  If a device is open, closes it first."""

    def help_q(self):
	help_quit()

    def help_wvar(self):
	print """wvar <value>

Performs WVAR on a PORUS test device, writing <value> to the device 
variable."""

    def help_rvar(self):
	print """rvar <value>

Performs RVAR on a PORUS test device.  Prints the result in hex."""

    def do_wvar(self,args):
	if self.devh is None:
	    print "No device is open"
	    return 0
	args=shlex.split(args)
	if len(args)<1:
	    print "Need a value to write"
	    return 0
	try:
	    var=int(args[0])
	except ValueError:
	    print "Value must be an integer"
	    return 0
	try:
	    self.devh.controlMsg(0x41, 1, [], var)
	    print "Wrote %02X"%var
	except:
	    print "Error:", sys.exc_info()[0]

    def do_rvar(self,args):
	if self.devh is None:
	    print "No device is open"
	    return 0
	try:
	    buf=toUns(self.devh.controlMsg(0xC1, 2, 2))
	    var=buf[0]<<8|buf[1]
	    print "Received %04X (%d)"%(var,var)
	except:
	    print "Error:", sys.exc_info()[1]

    def do_stat(self,args):
	if self.devh is None:
	    print "No device is open"
	    return 0
	try:
	    buf=porusSTAT(self.devh)
	    if not porusShortStatCodes.has_key(buf):
		print "Status: %d (unknown)"%buf
	    else:
		print "Status: %s (%s)"%(porusShortStatCodes[buf],porusLongStatCodes[buf])
	except:
	    print "Error:", sys.exc_info()[1]

    def do_rcrc(self,args):
	if self.devh is None:
	    print "No device is open"
	    return 0
	try:
	    var=porusRCRC(self.devh)
	    print "CRC",uhex32(var)
	except:
	    print "Error:", sys.exc_info()[1]

    def getEP(self,epn):
	if self.devh is None: return None
	eps=self.curdev[2].interfaces[0][0].endpoints
	for e in eps:
	    if e.address==epn:
		return e
	return None

    def writeWithShort(self,epn,buf):
	if self.devh is None: return None
	ep=self.getEP(epn)
	if ep is None: return None
	self.devh.bulkWrite(epn,buf)
	if len(buf)%ep.maxPacketSize==0:
	    self.devh.bulkWrite(epn,'')

    def rxChain(self,epn):
	if self.devh is None: return None
	ep=self.getEP(epn)
	if ep is None: return None
	buf=''
	while 1:
	    tbuf=tupleToStr(self.devh.bulkRead(0x80|epn,ep.maxPacketSize))
	    buf+=tbuf
	    if len(tbuf)<ep.maxPacketSize:
		break
	return buf

    def do_blko(self,args):
	if self.devh is None:
	    print "No device is open"
	    return 0
	args=shlex.split(args)
	if len(args)<1:
	    print "Need a number of bytes"
	    return 0
	try:
	    bc=int(args[0])
	except ValueError:
	    print "Number of bytes must be an integer"
	    return 0
	try:
	    print "Sending BLKO .."
	    porusBLKO(self.devh,bc)
	    print "Generating %d random bytes .."%bc
	    rndbuf=rndstring(bc)
	    print "Calculating CRC .."
	    crc=toUns32(zlib.crc32(rndbuf))
	    print "CRC is %s"%uhex32(crc)
	    print "Sending to peripheral .."
	    self.writeWithShort(1,rndbuf)
	    print "Waiting for STAT .."
	    while (porusSTAT(self.devh)):
		pass
	    print "STAT is idle.  Getting CRC .."
	    pcrc=porusRCRC(self.devh)
	    print "Peripheral CRC is %s"%uhex32(pcrc)
	    if long(crc)==pcrc:
		print "Match -- test OK!"
	    else:
		print "oops."
	except:
	    print "Error:", sys.exc_info()[1]
	return 0

    def do_blki(self,args):
	if self.devh is None:
	    print "No device is open"
	    return 0
	args=shlex.split(args)
	if len(args)<1:
	    print "Need a number of bytes"
	    return 0
	try:
	    bc=int(args[0])
	except ValueError:
	    print "Number of bytes must be an integer"
	    return 0
	try:
	    print "Sending BLKI .."
	    porusBLKI(self.devh,bc)
	    print "Waiting for BITX (or IDLE).."
	    while porusSTAT(self.devh)==3: pass
	    print "Got BITX, now receiving .."
	    rndbuf=self.rxChain(1)
	    print "Received %d bytes"%len(rndbuf)
	    print binascii.hexlify(rndbuf)
	    print "Calculating CRC .."
	    crc=toUns32(zlib.crc32(rndbuf))
	    print "CRC is %s"%uhex32(crc)
	    print "Getting peripheral's CRC .."
	    pcrc=porusRCRC(self.devh)
	    print "Peripheral CRC is %s"%uhex32(pcrc)
	    if long(crc)==pcrc:
		print "Match -- test OK!"
	    else:
		print "oops."
	except:
	    print "Error:", sys.exc_info()[1]
	    #raise
	return 0

    def do_tmoi(self,args):
	if self.devh is None:
	    print "No device is open"
	    return 0
	args=shlex.split(args)
	if len(args)<1:
	    print "Need a number of bytes"
	    return 0
	try:
	    bc=int(args[0])
	except ValueError:
	    print "Number of bytes must be an integer"
	    return 0
	try:
	    print "Sending TMOI .."
	    porusTMOI(self.devh,bc,1)
	    t=time.time()
	    while 1:
		s=porusSTAT(self.devh)
		print "Got %s after %f seconds .."%(porusShortStatCodes[s],time.time()-t)
		if s==0 or s==5:
		    break
		if s==6:
		    return
		time.sleep(0.2)
	    print "Finished in about %f seconds."%(time.time()-t)
	except:
	    print "Error:", sys.exc_info()[1]
	return 0

    def do_ls(self,args):
	if self.devh is None:
	    self.devs=getdevs()
	shortlist(self.devs)

    def do_open(self,args):
	if self.devs is None:
	    print "Do ls first, then pick a device"
	    return 0
	args=shlex.split(args)
	if len(args)<1:
	    if self.devh is None:
		print "No device is open"
	    else:
		print "Device %d is open"%self.devn
	    return 0
	try:
	    dn=int(args[0])
	except ValueError:
	    print "Device number must be an integer"
	    return 0
	if dn>len(self.devs) or dn<0:
	    print "Device number is out of range"
	    return 0
	self.curdev=self.devs[dn]
	self.devh=self.curdev[1].open()
	self.devn=dn
	#self.devh.setConfiguration(self.curdev[2])
	self.devh.claimInterface(self.curdev[2].interfaces[0][0])
	#self.devh.setAltInterface(0)

    def do_close(self,args):
	if self.devh is None:
	    print "No device is open"
	    return 0
	del self.devh
	self.devh=None
	self.curdev=None
	self.devn=None

    def do_info(self,args):
	args=shlex.split(args)
	if len(args)<1:
	    if self.devh is None:
		print "No device is open; specify a device"
	    else:
		printLongInfo(self.curdev+(self.devh,))
	    return 0
	if self.devs is None:
	    print "Do ls first, then pick a device"
	    return 0
	try:
	    dn=int(args[0])
	except ValueError:
	    print "Device number must be an integer"
	    return 0
	if dn>len(self.devs) or dn<0:
	    print "Device number is out of range"
	    return 0
	devrec=self.devs[dn]
	devh=devrec[1].open()
	printLongInfo(devrec+(devh,))
	del devh

    def do_q(self,args):
	return self.do_quit(args)

    def do_quit(self,args):
	if self.devh is not None:
	    self.do_close('')
	return 1

if __name__=='__main__':
    s=usbsh("USB Shell")
    s.prompt="usb> "
    s.cmdloop()
