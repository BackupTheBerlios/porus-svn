
#ifndef GUARD_5509a_i2c_h
#define GUARD_5509a_i2c_h

#define I2CREG(ofs)             IOREG(I2C_BASE+ofs)

#define I2COAR                  I2CREG(0)
#define I2CIMR                  I2CREG(1)
// assuming this is right ...
#define I2CIER                  I2CIMR

#define I2CSTR                  I2CREG(2)
#define I2CSTR_BB               (1<<12)
#define I2CSTR_RSFULL           (1<<11)
#define I2CSTR_XSMT             (1<<10)
#define I2CSTR_AAS              (1<< 9)
#define I2CSTR_AD0              (1<< 8)
#define I2CSTR_XRDY             (1<< 4)
#define I2CSTR_RRDY             (1<< 3)
#define I2CSTR_ARDY             (1<< 2)
#define I2CSTR_NACK             (1<< 1)
#define I2CSTR_AL               (1<< 0)

#define I2CCLKL                 I2CREG(3)
#define I2CCLKH                 I2CREG(4)
#define I2CCNT                  I2CREG(5)
#define I2CDRR                  I2CREG(6)
#define I2CSAR                  I2CREG(7)
#define I2CDXR                  I2CREG(8)

#define I2CMDR                  I2CREG(9)
#define I2CMDR_FREE             (1<<14)
#define I2CMDR_STT              (1<<13)
#define I2CMDR_STP              (1<<11)
#define I2CMDR_MST              (1<<10)
#define I2CMDR_TRX              (1<<9)
#define I2CMDR_XA               (1<<8)
#define I2CMDR_RM               (1<<7)
#define I2CMDR_DLB              (1<<6)
#define I2CMDR_IRS              (1<<5)
#define I2CMDR_STB              (1<<4)
#define I2CMDR_FDF              (1<<3)
#define I2CMDR_BC               (7)

#define I2CIVR                  I2CREG(10)
#define I2CPSC                  I2CREG(12)
#define I2CMDR2                 I2CREG(15)

#endif
