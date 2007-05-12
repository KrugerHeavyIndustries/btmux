/*
 * Fast Infoset serialization interface.
 */

#ifndef BTECH_FI_SERIALIZABLE_HH
#define BTECH_FI_SERIALIZABLE_HH

namespace BTech {
namespace FI {

// Section 5.5: Fast Infoset numbers bits from 1 (MSB) to 8 (LSB).
#define FI_BIT_1 0x80
#define FI_BIT_2 0x40
#define FI_BIT_3 0x20
#define FI_BIT_4 0x10
#define FI_BIT_5 0x08
#define FI_BIT_6 0x04
#define FI_BIT_7 0x02
#define FI_BIT_8 0x01

#define FI_BITS(a,b,c,d,e,f,g,h) \
	(  ((0||(a+0)) << 7) | ((0||(b+0)) << 6) \
	 | ((0||(c+0)) << 5) | ((0||(d+0)) << 4) \
	 | ((0||(e+0)) << 3) | ((0||(f+0)) << 2) \
	 | ((0||(g+0)) << 1) | ((0||(h+0))     ))

// Forward declarations for Encoder/Decoder.
class Encoder;
class Decoder;

// Abstract base class for objects supporting serialization via an
// Encoder/Decoder.
class Serializable {
protected:
	virtual ~Serializable () {}

public:
	virtual void write (Encoder& encoder) const = 0;
	virtual bool read (Decoder& decoder) = 0;
}; // class Serializable

} // namespace FI
} // namespace BTech

#endif /* !BTECH_FI_SERIALIZABLE_HH */
