/*
 * Exceptions.
 */

#ifndef BTECH_FI_EXCEPTION_HH
#define BTECH_FI_EXCEPTION_HH

#include <exception>

namespace BTech {
namespace FI {

class Exception : public std::exception {
}; // Exception

class OutOfMemoryException : public Exception {
}; // OutOfMemoryException

class IndexOutOfBoundsException : public Exception {
}; // IndexOutOfBoundsException

class InvalidArgumentException : public Exception {
}; // InvalidArgumentException

class IllegalStateException : public Exception {
}; // IllegalStateException

class UnsupportedOperationException : public Exception {
}; // UnsupportedOperationException

} // namespace FI
} // namespace BTech

#endif /* !BTECH_FI_EXCEPTION_HH */
