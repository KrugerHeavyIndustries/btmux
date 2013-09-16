#! /bin/sh

#
# Downloads data files to the game directory, then optionally 
#

SVNROOT="svn://svn.code.sf.net/p/btonline-btech/code"

# XXX: Terminate on errors.  This is sorta a lazy hack, to avoid having to
# check exit status and recover on each command.
set -e

# Check for prototype game directory.
if test ! -d game; then
	echo "Downloading game directory from the Subversion repository..."
	svn co "${SVNROOT}/game/trunk" game
fi

# Check for prototype maps directory.
if test ! -d game/maps; then
	echo "Downloading game/maps from the Subversion repository..."
	svn co "${SVNROOT}/maps/trunk" game/maps
fi

# Check for prototype text directory.
if test ! -d game/text; then
	echo "Downloading game/text from the Subversion repository..."
	svn co "${SVNROOT}/text/trunk" game/text
fi

# Check for prototype mechs directory.
if test ! -d game/mechs; then
	echo "Downloading game/mechs from the Subversion repository..."
	svn co "${SVNROOT}/mechs/trunk" game/mechs
fi

# Check if, for some bizarre reason, we still don't have a game directory.
if test ! -d game; then
	echo "No game directory. Please acquire one from http://sourceforge.net/projects/btonline-btech."
	exit 1
fi

# Okay, go ahead and install stuff.
if test ! -d game.run; then
	echo "No game.run directory, installing data files from game."
	cp -pPR game game.run || exit 1
	chmod -R u+w game.run || exit 1
fi
