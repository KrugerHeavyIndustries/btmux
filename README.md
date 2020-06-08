Battletech MUX
==============

Battletech MUX is a [TinyMUX 1.6 based MUX server](https://en.wikipedia.org/wiki/MUSH) with special [Battletech](https://en.wikipedia.org/wiki/BattleTech)
extensions. These extensions allow site admins to configure and run a
persistent pseudo real-time warfare simulation. The rules that govern the simulation are an adaptation of the original turn based FASA board game.

To find out how to install this piece of software, see the `INSTALL` file. The
rest of this `README` details a rough history of the code, as
well as the rationale behind the license (which can be found in the file
`COPYING`).

There was, in previous decades, a wiki that was a companion resource for this codebase. It is still partially accessible via the magic of web.archive.org. [The archived btmux wiki is here](https://web.archive.org/web/20081219032921/http://docs.btmux.com/index.php/Main_Page).

Table of contents
====================

* [History](#history)
* [Credits](#credits)
* [License](#license)


History
=======

This codebase is the result of over 20 years of development in one form or
another. It started hosted on a TinyMUSE server, around 1992 or 1993. Its primary
instance powered the most popular Battletech MU* to date, 3056 MUSE. 

There after,
the code was ported to/reimplemented for TinyMUSH by the Animudiacs crew,
who ran, amongst other servers, 3034-II (which later evolved into 3035) and
Varxsis (a non-battletech universe with similar theme and technology). Somewhat
simultaneously, Markus Stenberg (aka Fingon) ported the original MUSE code
to TinyMUX, implementing a lot of ideas from the Animudiacs sites and
implementing a lot more from scratch, both official Battletech rules and
brand new ideas. The main MUX site has always been 3030 MUX.

Fingon continued development until about 1998, after which he released the
source to the public under a restricted license and retired. Development for 3030 was continued by several of the 3030 MUX Wizards, most notably Thomas Wouters (Focus) and Cord Awtry (Spectre).

In 1999, a new Battletech MUX using the restricted license source opened up
under the banner of 'Exile: Exodus', with development efforts lead by Null
(Kevin Stevens). Exile took an aggressive approach to development and many
modifications, rewrites, and additions were made to the now ancient
source. While some parallel development was done in coordination with
the staff of 3030, the two source trees remained largely seperate due to different licensing.

After some requests from people running their own spinoffs from the original
'public' release by Fingon, Fingon, Spectre, and Focus wrote a new licence:
the 3030 MUX Artistic License, and released the actual 3030 MUX source tree
under it. The MUX source then moved to SourceForge for its main development.

Limited by the licensing on the old release, the Exile source tree was
unable to release its source. Attempts were made to contact Fingon to get the
old tree re-licensed, but over the span of several years nothing was heard
from him.

In late 2004 and early 2005, Null began to port his changes to the Exile
base to the open-sourced 3030 branch.

The torch was then picked up and carried by a development team
headed up by Kelvin McCorvin, composed of many of the people in the
docs/CREDITS file. With Hag as lead programmer overseeing a capable team of
developers, the codebase grew in both scope and quality.

Credits
=======

Much credit is due to many. Unfortunately due to the
long and storied history of the source code as well as many of the included ideas borrowed from other MUSEs, MUSHes and MUXes that inspired this MUX code, it presents an impossible task.

Anyone who worked on TinyMUSE, TinyMUSH and TinyMUX,
both with and without Battletech extensions, as well as those who
built and ran the Battletech MUSE, MUSH and MUX worlds out there is deserving of credit. If that is you, and you want to be listed in a docs/CREDITS file, let us know.

License
=======

This code is distributed under the 3030 MUX Artistic License, a modified
Artistic License, which you should find in a file 'LICENSE' accompanying the
actual source code. The changes to the Artistic License are minimal:
sections 3.b, 3.c, 4.b and 7 are scrapped, and the rest of the sections were
renumbered to fill the gaps. No other changes were made. 

The net result of
these changes is that if you want to host a game with a modified version of
this source, you HAVE TO make that source available. Either by publically
putting it up a website, or by sending it back to the original authors (a
patch submission will do.) You do not, however, have to wait for
the authors to incorporate the changes before you use them; making them
available is enough.

The rest of the Artistic License restrictions are the same: You cannot
charge money for this source, though you can charge money for supporting
this source. MUX 'softcode' that you use in a game is considered part of the
configuration/scripting of the MUX, and does not fall under the 3030 MUX
Artistic License: you can do anything you like with it, including selling
it.

The original authors of the code retain their copyright. The source is not
yours. You are given permission to use it. Actual authors are listed in the
files themselves. When attribution in the 'src/' directory is missing,
assume David Passmore. When attribution in src/hcode or any other directory
is missing, assume Markus Stenberg. 


