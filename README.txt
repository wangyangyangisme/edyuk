
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

                 Edyuk, (C) 2006-2008 Luc Bruant a.k.a fullmetalcoder,
                       is a free and open source software.

      You may use, distribute and copy Edyuk under the terms of GNU GPL v 3
      (General Public License, see the file GPL.txt, distributed along, for
      more informations). Note that it implies the following :

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

   I	) Yet another IDE ?
   II	) Requirements
   III	) Pluginize your world
   IV	) Team & Contributions
   V	) HOW TO ???
   VI	) Disclaimer


I ) Yet another IDE ?

 Quite a stupid question indeed, but it needs an answer! Yes, Edyuk is "Yet
another Integrated Developpment Environnement", it does not bring revolutionnar
concepts but try to make an intelligent use of all those that already exists!

 Edyuk is an Integrated Developement Environment built with Qt4 and meant to
provide a light, fast and stable environment for rapid application development
in C++/Qt4. Thanks to plugins (see III) its scope can hopefully be extended to
any possible programming related task (e.g version control, issue tracking,
management of other project formats, support for other languages/toolkits...)


II ) Requirements

	* Qt 4.3.0 or higher : http://trolltech.com/qt
	
	Note : Edyuk builds with both Qt 4.4 and 4.3 but a Qt 4.4 build will NOT
	work in a Qt 4.3 environment and a Qt 4.3 build will not work as well in
	a Qt 4.4 environment as it would in a Qt 4.3 one (assistant integration
	broken). Please use Edyuk with the same version of Qt it was compiled
	against to avoid troubles...
	
	* A C++ compiler (preferabily GCC/MinGW though VC++ should work).
	Warning : gcc 3.4.5 has been reported to create crashing executables under
	Linux 64 bit. If you are under Linux, prefer gcc 4.x over 3.x
	
	* Some patience to wait during the awfully long compilation (a couple of
	minutes, depending on your hardware...)


III ) Pluginize your world

 Edyuk is meant to be extended in many ways. Actually, without the plugins
packed by default, it is merely a Kate-like text editor, which is already
a good sign of modularity since it implies that syntax definitions (aka
languages definitions, highlighting rules or whatever you like to call it...)
are built at run-time from human-readable XMl files whose format is self-
explanative and (well?) documented.

!!! You should skip this part if you're not interested in contributing to Edyuk !!!

 Besides, Edyuk implements a lightweight plugin system which is said to be
"component-oriented". This term refers to the fact that plugins provide
a set of "components" know to Edyuk as a pair of string (type and id). These
are loaded appropriately at run-time. Supported components include :

	* QLanguageDefinition : a mean to use hard-coded syntax definitions instead
	of the default generic ones (or to bring support for new generic format...)
	
	* QCompletionEngine : the base class for code completion.
	
	* QCodeParser : the base class for code parsers, in charge of class browsing,
	and as such, of half of the completion backends.
	
	* QProjectParser : the base class for project parsers, in charge of loading
	projects, that is turning a file into a custom tree structure, based on the
	QProjectModel2 framework.
	
	* qmdiPerspective : the base class for perspectives (did I forgot to mention
	that Edyuk GUI is perspective-based and makes use of dynamic menus? ;) )
	
	* QBuilder : the base class for builders. A builder is basically an convertion
	tool. Builders are chained together by the build engine to allow projects to
	be compiled and run. For instance, the default plugin provides a builder for
	qmake (which turns *.pro files into Makefiles) and another which processes
	GNU Makefiles.
	
	* QDebuger : the base class for debugers. A debuger provides graphical debugging
	facilities for the binaries built from a project. It should rely on threading
	to prevent the UI from freezing and to allow o the fly breakpoint placement.
	Its UI is mostly composed of actions which are placed in menus/toolbars using
	qmdilib (QDebuger inherits qmdiClient)


IV ) Team & Contributions

The team is currently composed of :

	- Luc Bruant aka fullmetalcoder : founder and most active coder until now

People that take part in Edyuk development for a while :

	- Brian M Workman : joined the team around begining of spring 2007 if I
	remember well, (correct me if I'm wrong) and did important work on Edyuk
	configuration facilities for both dialog and core.
	
	- Juergen P Messerer : joined the team around the end of spring 2007, bringing
	ideas and code samples along. His first contribution is the draggable tab bar.
	Although I completely changed the implementation he proposed the concept and
	the motivation comes from him.
	
	- Christophe Duelli aka caduel : joined the team in august 2007 and started
	hacking right away after having submitted a very useful issues list.

Other contributions (apologies to the forgotten ones... a single mail would fix this) :
 
	- Elcuco ranted an incredible number of time but most of his opinions, though
	a bit acid hopefully lead to fixes and improvements
	
	- Jeremy Magland did some really useful testings under Windows which allowed
	me to track down all bugs whitout having a windows box.
	
	- Donaldinos and Jens Idelberger did some testing/debugging under Mac which
	allowed me to fix bugs encountered on this platform without having one to do
	the tests myself.
	
	- Shriraman Sharma fed me back about all scripting issues. He gave me links and
	ideas that fianlly lead me to the shiny new build script.
	
	- Kent Kristensen gave me a long wishlist for QCodeEdit 2 and you can already see
	some new features that come from there
	
	- Thomas Keller provided a lot of useful feedback about Mac integration and UI
	design, and a couple of reworked ui files as well.

I may have forgotten some people. I hope they'll forgive me and contact me so that
I give them proper credit here.

 If you feel like getting involved read STANDARDS.txt TODO.txt and contact the team
through the mailing list (edyuk-devel@lists.sf.net) or directly one of its members


V ) HOW TO ???

 * How to build Edyuk ?

   - Check out the requirements (II)...
   
   - Make sure that your PATH variable point to the location where your compiler
   resides (this is OK by default under Unix but rarely under Windows)

   - Open a shell or any command line tool

   - Execute the classical command or enjoy the power of shell script.
   The shell script has many neat features :
	* auto-resolution of qmake command
	* Qt version check (>= 4.3)
	* compile output formatting
	* Several pretty useful switches making your life easier (run ./build -h)
	
	/!\ In case you have troubles running the build script (e.g under BSD) please
	report it so that it may be fixed...
	
	/!\ The script appears to fail detecting an error in make so if you fail to
	run Edyuk because of missing binaries consider using the god old qmake && make

	Unix :
	$ ./build [--install]
	
	Others (or if build fails) :
	$ qmake
	$ make
	$ qmake [just to make sure install target is created properly]
	$ make install [*nix only, requires admin rights via su, sudo, whatever]

   - Run the app by double cliking on it or the run the folowing commands
	Unix :
    $ ./edyuk
    
	Windows :
	$ edyuk

 * How to report bugs/compilation failure/whatsoever may be wrong ?
   You may reach the team members through :

		* Sf.net task tracker : http://sourceforge.net/tracker/?group_id=168260
		You'll need a Sf.net account to use this one
		
		* WebIssues-based tracker : http://edyuk.org/webissues
		See http://webissues.mimec.org for more details (the URL
		above is that to be given to the WebIssues client)
		
		login : anonymous
		password : anonymous
		
		* Edyuk Devel mailing list : edyuk-devel@lists.sf.net
		
		* QtCentre.org related thread (in Qt software section)
		
		* directly by mail (see at the bottom...)

 * How to get bleeding-edge Edyuk ?
    Install a Subversion client and perform an anonymous checkout of :
		https://edyuk.svn.sf.net/svnroot/edyuk/trunk
		
	For instance, under Linux this can be done using the following command line :
	
	$ svn co https://edyuk.svn.sf.net/svnroot/edyuk/trunk edyuk
	
	For a list of Subversion clients, visit http://subversion.tigris.org

 * How to be informed of what's going on ?
    The following media will allow you to be informed of Edyuk's progress :
		
		* Edyuk homepage : http://edyuk.org
		
		* Edyuk commits feed (provided by CIA.vc) : http://cia.vc/stats/project/edyuk/.rss
		
		* Edyuk Devel mailing list : edyuk-devel@lists.sf.net

 * How to get some documentation ?
   You may have noticed that the doc folder is empty. Don't worry this is on
   purpose, to reduce the size of packages (yet if asked, this may change).
   All you need is Doxygen (http://doxygen.org) :
   $ doxygen
   
   And the doc folder will miraculously get populated :)
   Please note that apidox are still work in progress and due to a lack of time they are the
   part of Edyuk that get the less attention...

 * How to create your own plugin ?

   Have a look at the code of the plugins shipped with this package if you
   can't wait for a proper manual... ;-)

 If you have questions feel free to ask them :

	* fullmetalcoder				<fullmetalcoder@hotmail.fr>
	* edyuk devel mailing list		<edyuk-devel@lists.sf.net>
	* QtCentre forum				<http://qtcentre.org/forum>


VI ) Disclaimer

 This software is released in the hope that some people may find a use for it, which does not
 imply it may happen... In other words it comes with NO WARRANTY OF ANY KIND and neither the author
 nor any of the contributor may be held liable for the damages it could cause to your computer,
 math teacher, fish or whoever comes in too close contact with it...
