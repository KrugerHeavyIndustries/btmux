#! /bin/sh

# Run autoreconf with our options.

AUTOMAKE="automake --foreign"

export AUTOMAKE

autoreconf
