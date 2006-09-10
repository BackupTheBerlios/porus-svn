
# setup.py -- setup / install script for usbgen

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

from distutils.core import setup
setup(name='usbgen',
      version=VERSION,
      scripts=['usbgen']
      )
