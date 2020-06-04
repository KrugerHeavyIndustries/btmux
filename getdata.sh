#! /bin/sh

#
# Downloads data files to the game directory, then optionally 

GITORG="https://github.com/KrugerHeavyIndustries"

# XXX: Terminate on errors.  This is sorta a lazy hack, to avoid having to
# check exit status and recover on each command.
set -e

# Check for prototype game directory.
if test ! -d game; then
	echo "Downloading game directory from the git repository..."
  mkdir -p game
	curl -L "${GITORG}/btmux-game/tarball/master" | tar xv -C game --strip-component 1
fi

# Check for prototype maps directory.
if test ! -d game/maps; then
	echo "Downloading game/maps from the git repository..."
  mkdir -p game/maps
	curl -L "${GITORG}/btmux-maps/tarball/master" | tar xv -C game/maps --strip-component 1
fi

# Check for prototype text directory.
if test ! -d game/text; then
	echo "Downloading game/text from the git repository..."
  mkdir -p game/text
	curl -L "${GITORG}/btmux-text/tarball/master" | tar xv -C game/text --strip-component 1
fi

# Check for prototype mechs directory.
if test ! -d game/mechs; then
	echo "Downloading game/mechs from the git repository..."
  mkdir -p game/mechs
	curl -L "${GITORG}/btmux-mechs/tarball/master" | tar xv -C game/mechs --strip-component 1
fi

# Check if, for some bizarre reason, we still don't have a game directory.
if test ! -d game; then
	echo "No game directory. Please acquire one from http://github.com/KrugerHeavyIndustries/btmux."
	exit 1
fi

# Okay, go ahead and install stuff.
if test ! -d game.run; then
	echo "No game.run directory, installing data files from game."
	cp -pPR game game.run || exit 1
	chmod -R u+w game.run || exit 1
fi
