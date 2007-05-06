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

class AssertionFailureException : public Exception {
}; // AssertionFailureException

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

class IOException : public Exception {
}; // IOException

} // namespace FI
} // namespace BTech

#endif /* !BTECH_FI_EXCEPTION_HH */
