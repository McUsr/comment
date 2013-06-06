comment
=======

Set, Append, Prepend, and delete Finder comments from the commandline, works from 10.4 onwards
comment: version 1.0 Copyright © 2013 McUsr and put into Public Domain
         under Gnu LPGL 2.0

comment: List files with Finder comments  or comments files from commandline
         that aren't locked or read only. Either by setting or resetting a 
		 full new comment, or by prepending or appending and existing comment.
         For OS X 10.6 and later. Copyright © 2013 McUsr. GnU GPL 2.0.
		 Filenames maybe be read from stdin.
	
Usage: comment [options] [1 to n ..file arguments or from stdin
	    specified by posix path, one file on each line.]
		Any multi wordcomment need to be either enclosed in single double ticks
		or have the spaces withing the comment escaped by \ in order to differ
		between the comment and any subsequent file arguments.
		   
Options
-------
   comment [-huCvlsapdvtPD]
   comment [ --help,--usage,--copyright,--version,--list,--set,--append,
             --prepend,--delimiter,--verbose, --terse,--fullpath,-comment-delim ]
Details
-------
 	-l [file 1..en] list files and their comments. Respecting any delimiter.
	   if verbose is given, then files without comments will also be listed .
       (--list).
 	-P like -l but the absolute path of the file is returned.
	-s [comment] [file1..filen] sets/resets a comment for one or more files.
       An empty comment are allowed, and will delete any old ones. (--set).
	-a [comment] [file1..filen] appends a comment to one or more files.
	   A file without a comment, will get their comment field set. if strict
	   option isn't specified.(--append).
	-p [comment] [file1..filen] prepends  a comment to one or more files.
	   A file without a comment, will get their comment field set. If strict
	   option isn't specified. (--prepend).
	-v [-l] file1..filen] list both files with and without comments. (--verbose).
	-t [ap] [comment] [file1..filen] Sees to that only  files with an existing
	   comment will have text added to their comment.(--terse).
    -d [delimiter] [file1..filen] specifies a delimter to split the filenames
	   from the comments.(--delimiter).
	-P [filei 1..n] like -l on its own,  but outputs the full absolute paths of
       the files. It works with other options that output filenames.
	-D [comment delimiter] [ap] [file 1..n] lets you specifier other than space
       to differ between the comment, and the new appended/prepended part.
	   
Examples of usage:
------------------

	ls |comment -ld :
	Lists all files with a comment to stdout as a colon separated list.
	ls |comment -vld -
	Lists all files  comment or not to stdout as a colon separated list.
	-No comment, still colon.
	comment -sp 'Threadstone ' *
	prepends any files with Finder comments in the current directory with
	'Threadstone'.	
	comment -a " DONE $(date)" *
	Appends or sets a comment like 'DONE man jun  3 23:00:18 CEST 2013' into
	all files in the current working directory.
