/*
 * Fast Infoset serialization interface.
 */

#ifndef BTECH_FI_SERIALIZABLE_HH
#define BTECH_FI_SERIALIZABLE_HH

namespace BTech {
namespace FI {

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
