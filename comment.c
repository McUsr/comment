/*********************************************************************** 
 Name:    
 Created: 06-05-2013
 Author:  McUsr 
 
 Usage:   
 comment, set, append, and delete Finder comments.

 Just enter "comment" for help in a Terminal window, once installed in your path.

 You can  delete a comment by issuing the command comment -s '' yourfile.
 
 DISCLAIMER
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 
 primary tests:
 asserts LITTLE_ENDIAN
 version 10.4 or newer
 
 secondary tests:
 any files operated upon must be on a hfs+ filesystem.
 
 Below is the command line to compile:

 gcc -Os -I /System/Library/Frameworks/CoreFoundation.framework/Versions/A/Headers/   -o comment comment.c -L/System/Library/Frameworks  -framework CoreFoundation -framework CoreServices

 © McUsr and put into public domain under Gnu LPGL 2.0
 It is free to use for both  Non-Commercial purposes and commercialr
 purposes, you may not sell it as it is, or the binary  as part of
 something, without giving me notice at least.
 And then you'll have to disclose  this source as I understand
 Gnu LPGL 2.0.
 
 ***********************************************************************/


#if 1 == 1
#define TESTING
#endif

#include <CoreFoundation/CoreFoundation.h>
#include "/System/Library/Frameworks/CoreServices.framework/Versions/A/Frameworks/Metadata.framework/Versions/A/Headers/MDItem.h" /* Thanks to StefanK! */
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/stat.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <libgen.h>
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>

#define USER_ERROR 2
#define INTERNAL_ERR	4
#define min(x,y) (((x)<(y)) ? (x) : (y))
#define max(x,y) (((x)>(y)) ? (x) : (y))
struct osver {
	int minor;
	int sub;
} ;

typedef struct osver osxver ;


static void macosx_ver(char *darwinversion, osxver *osxversion ) ;
static char *osversionString(void) ;
int osx_minor( void ) ;

int		pathLengths( int *max_path_len, int *max_name_len, char *argv_0 ) ;
char	*full_absolute_path( char **orig_path, size_t orig_len, size_t max_path_len ) ;
bool	validFinderFileFn(char *fn) ;
int		maxPathCompLen( char *aPath ) ;
bool	fonhfsplus(char *fname ) ;
int		cmdln_maxarg(void ) ;
char *	baseNameCheck(  char* pathToCheck) ;

static int	MY_MAX_PATH_LEN		= 0, 
			MY_MAX_NAME_LEN		= 0;

static char	*ABSOLUTE_FILENAME	= NULL,
			*THE_COMMENT		= NULL,
			DELIMITER			= '\t',
			COMMENT_DELIMITER	= ' ';

static void setFileComment(char *fn, char *newCmt ) ;
static char *getFileComment( CFURLRef existingFile ) ;
static int listComments( char *fn,u_int16_t theCmd ) ;
static int setComments( char *fn,u_int16_t theCmd ) ;
	
static void validate_file(char *file_arg, char **absfname  ) ;
static void err_with_message( int errnum, char *format, ... ) ;
static void err_msg_nofiles(void ) ;
static void usage(void) ;
static void version(void ) ;
static void copyright(void );


static struct option longopts[] = {
	{"help", no_argument, NULL, 'h'},
	{"usage", no_argument, NULL, 'u'},
	{"copyright", no_argument, NULL, 'c'},
	{"version", no_argument, NULL, 'V'},
	{"list", no_argument, NULL, 'l'},
	{"set", required_argument, NULL, 's'},
	{"append", required_argument, NULL, 'a'},
	{"prepend", required_argument,NULL, 'p'},
    {"delimiter",required_argument,NULL, 'd'},
    {"verbose",no_argument,NULL, 'v' },
	{"terse", no_argument, NULL, 't'},
	{"fullpath", no_argument, NULL, 'P'},
	{"comment_delim",required_argument , NULL, 'D'},
	{NULL, 0, NULL, 0},
};



#define CMD_LIST		0x01
#define CMD_SET			0x02
#define CMD_APPEND		0x04
#define CMD_PREPEND		0x08
/* Det som er forskjellig nå, er at jeg har annet navn for å skille bedre. */
#define OPT_VERBOSE		0x10
#define OPT_FULLPATH	0x20
#define OPT_TERSE		0x40
/*
 noe annet for delimitiere, om de er spesifiserte.
 */
#define set_verbose(x)\
do {\
if ((x & CMD_NUMBER)==CMD_NUMBER )\
;\
else\
x |= CMD_VERBOSE ;	\
} while(0)

#define copy_optarg(x,y)\
do {\
y=malloc(strlen(x)+1) ;\
if (y == NULL ) {\
perror("Error during malloc of mem for optarg");\
exit(EXIT_FAILURE);\
} else {\
y=strcpy(y,x);\
}\
} while (0)

#define clear_prev_cmd( x ) x &=~(0x0F)


#define be_verbose(x) ((x & CMD_VERBOSE) == CMD_VERBOSE)

int main( int argc, char *argv[] )
{
	assert(__LITTLE_ENDIAN__);		
	bool			redir			= (bool)(!isatty(STDIN_FILENO)),
	multiple		= false;
	int	cmd_err		= EXIT_SUCCESS,
		minor_ver	= osx_minor();
	u_int16_t 		optype = 0;
	
	if (minor_ver < 4 )
		err_with_message(EXIT_FAILURE, "comment: This program requires Mac OS X 10.4 or newer. You have (10.%d).\n", minor_ver,0);
	/* char *pwd=getenv("PWD"); */
	if (argc < 2 && redir == false ) 
		err_msg_nofiles() ;
	else if ( argc < 2 && redir == true )  
		optype = (OPT_VERBOSE | CMD_LIST ) ;// we'll list files from stdin.
	else {
		extern int optind ;
    	int	ch;
		/* We are only giving feedback in the list mode really. */
    	while ((ch = getopt_long(argc, argv, "hucVlvtPs:a:p:d:D:", longopts,NULL )) != -1) {
    		switch (ch) {
    			case 'h':
    				usage() ;
    				exit(EXIT_SUCCESS) ;
    			case 'u':
    				usage() ;
    				exit(EXIT_SUCCESS) ;
    			case 'c':
    				copyright();
    				exit(EXIT_SUCCESS) ;
    			case 'V':
    				version() ;
    				exit(EXIT_SUCCESS);
    			case 'l':
					clear_prev_cmd(optype);
					optype |= CMD_LIST;
    				break;
				case 's':
					clear_prev_cmd(optype) ;
					optype |= CMD_SET ;
					copy_optarg(optarg,THE_COMMENT);
    				break;
    			case 'a':
					clear_prev_cmd(optype) ;
					optype |= CMD_APPEND;
					copy_optarg(optarg,THE_COMMENT);
    				break;
				case 'p':
					clear_prev_cmd(optype) ;
					optype |= CMD_PREPEND ;
					copy_optarg(optarg,THE_COMMENT);
    				break;
				case 't': 
					optype |= OPT_TERSE ;
    				break;	
    			case 'v':
    				optype |= OPT_VERBOSE ;
    				break;
    			case 'P':
    				optype |= OPT_FULLPATH ;
    				break;
				case 'd':
					DELIMITER=optarg[0] ;
					break;
				case 'D':
					COMMENT_DELIMITER=optarg[0] ;
					break;
    		}
    	}
		
		if (!(optype & 0x0F)) optype |= CMD_LIST ; // may need default operation.
	}
		// final qualification of exec pattern after parse
	if (optind<(argc-1) || redir == true ) 
		multiple = true;
	else if (optind<argc) 
		multiple = false ;
	else
		err_msg_nofiles() ;
	// configuring for the correct operation.
	
	static int (*doit)(char *arg, u_int16_t cmd ) = NULL ; // generic function
	
	u_int16_t main_cmd= (optype & (~(0xF0))) ;
	
	if (main_cmd == CMD_LIST )  // TODO: figure the convention.
		doit= &listComments;
	else if ( main_cmd & (CMD_SET | CMD_APPEND | CMD_PREPEND ))
		doit= &setComments;
	else
		err_with_message( INTERNAL_ERR, "comment: can't happen! no valid command from main\n" ) ;
	   
	setbuf(stdout,NULL) ;
	// gathering any "static" information we may need.
	if (pathLengths(&MY_MAX_PATH_LEN, &MY_MAX_NAME_LEN, argv[0])<0) 
		err_with_message( INTERNAL_ERR, "comment: couldn't get system information for path lenghts.\n" ) ;

	size_t slen;
	if (multiple == false ) {
		
		if ((slen=strlen(argv[optind])) >= MY_MAX_PATH_LEN ) 
			err_with_message( USER_ERROR, "comment: The pathname is longer than allowed by the system.\n" ) ;
		else {
			validate_file(argv[optind],&ABSOLUTE_FILENAME ) ;
			cmd_err = doit(argv[optind],optype );
			
			exit(cmd_err) ;
		}
	} else if (multiple == true ) {
		int final_code;
		if (redir == true ) {
			char *buf = malloc( (size_t) MY_MAX_PATH_LEN+1 );
			if (buf == NULL ) 
				err_with_message( INTERNAL_ERR, "comment: main: Couldn't allocate memory for buf.\n" ) ;
			
			while((buf=fgets(buf,MY_MAX_PATH_LEN,stdin))!=NULL) {
				buf[strlen(buf)-1] = '\0'  ;
				if ( buf ) {
					validate_file(buf,&ABSOLUTE_FILENAME ) ;
					cmd_err = doit(buf,optype );
					final_code = (cmd_err == INTERNAL_ERR ) ?  INTERNAL_ERR  : EXIT_SUCCESS  ;
				}
			}
			free(buf) ;
			buf=NULL ;
			exit(final_code) ;
		} else {
			int last_arg = argc -1 ;
			
			while ( optind <= last_arg ) {
				validate_file(argv[optind],&ABSOLUTE_FILENAME ) ;
				cmd_err = doit(argv[optind++],optype );
				final_code = (cmd_err == INTERNAL_ERR ) ?  INTERNAL_ERR  : EXIT_SUCCESS  ;
			}
			exit(final_code) ;
		}
	}
	
	return 0;
}

/*!
    @function
    @abstract   Sets the Finder/Spotlight comment of a single valid file.
	@discussion It writes no output to stderr, as a confirmation.
				It checks notoriously for errors.
	@param      (char *fn : the filename)
				(u_int_16t) theCmd
				(bool multiple) This variable is for knowing that we have more than one
				when obtaining length data for the comment during setting, ignored for
				this function.
 
 @result     Returns 0 if everything went ok, an error code  if it didn't.
*/

#define MAX_COMBINED_NAME_AND_COMMENT_LENGTH_OF_UTF16_CHARS 1012

static int setComments( char *fn,u_int16_t theCmd ) {
	static  bool first_time = true ;
	int ret = EXIT_FAILURE ;
	
	
	static CFIndex newCommentLength= 0 ;
	if ( first_time  == true ) {
		CFStringRef cfNewComment=CFStringCreateWithCString(NULL ,THE_COMMENT,
															  kCFStringEncodingUTF8);
		newCommentLength = CFStringGetLength(cfNewComment);
		CFRelease(cfNewComment) ;
		first_time = false ;
	}
	CFIndex totalLen = newCommentLength ;
	char *theOldComment=NULL ;
	if (theCmd & (CMD_APPEND | CMD_PREPEND) ) {
		CFIndex oldCommentLength = 0 ;
		CFStringRef thePath=NULL ;
		thePath = CFStringCreateWithCString(NULL ,fn,
											kCFStringEncodingUTF8);
		CFURLRef myFile = NULL ;
		myFile =  CFURLCreateWithFileSystemPath (NULL,thePath,
												 kCFURLPOSIXPathStyle,FALSE );
		CFRelease(thePath) ;
		theOldComment = getFileComment(myFile ) ;
		if (theOldComment != NULL  || !(theCmd & OPT_TERSE)) {
			
			if (theOldComment != NULL ) {
				CFStringRef cfOldComment = CFStringCreateWithCString(NULL,theOldComment
																		   ,kCFStringEncodingUTF8);
				oldCommentLength = CFStringGetLength(cfOldComment) ;
				CFRelease(cfOldComment) ;
				totalLen += oldCommentLength + 1 ; // for the delimiter.
			}
			
		}
		CFRelease(myFile) ;
	}
	char *thebasename=baseNameCheck( fn);
	size_t utf8baseNameLen = strlen(thebasename);
	CFStringRef cfBasename=NULL ;
	cfBasename = CFStringCreateWithCString(NULL ,thebasename,
										kCFStringEncodingUTF8);

		// TODO: basename when decding the length in the .DS_store file!
	CFIndex baseNameLen = CFStringGetLength(cfBasename) ;
	CFRelease(cfBasename) ;
	if (utf8baseNameLen > baseNameLen) // utf8=greater, then we know a buffer was allocated and thebasename wasn't used.
		free(thebasename) ;

	totalLen+=baseNameLen ;
	if ((theCmd & (CMD_APPEND |CMD_PREPEND ) && ( theOldComment != NULL || (!(theCmd & OPT_TERSE))))|| (theCmd & CMD_SET) ) {
		// the second last test for validity, the combined length of the comment and the filename in UINT16 chars.		
		if (totalLen >= MAX_COMBINED_NAME_AND_COMMENT_LENGTH_OF_UTF16_CHARS ) {
			free(THE_COMMENT) ;
			free(ABSOLUTE_FILENAME) ;
			if (theCmd & (CMD_APPEND |CMD_PREPEND ))
				err_with_message(EXIT_FAILURE,"comment: setComments: the total size of the file %s, the old comment %s and the new comment %s is to big to handle. Bailing out.\n",fn,theOldComment,THE_COMMENT) ;
			else 
			err_with_message(EXIT_FAILURE,"comment: setComments: the total size of the file %s and the comment %s is to big to handle. Bailing out.\n",fn,THE_COMMENT) ;
		}
	
		size_t fullcmtSize ;
		char *newCmt=NULL ;
		if (theCmd & (CMD_APPEND |CMD_PREPEND )) {
			if (theOldComment != NULL)  {
				fullcmtSize = strlen(theOldComment) + strlen(THE_COMMENT) + 2 ;
				// IF the new comment to be appended/prepended starts/ends with delimiter, we adjust.
				if (THE_COMMENT[strlen(THE_COMMENT)-1] == COMMENT_DELIMITER && (theCmd & CMD_PREPEND ) )
					fullcmtSize-=1;
				else if (THE_COMMENT[0] == COMMENT_DELIMITER && (theCmd & CMD_APPEND ) ) 
					fullcmtSize-=1;
			} else if (!(theCmd & OPT_TERSE))
				fullcmtSize = strlen(THE_COMMENT) + 1 ;
			newCmt = malloc(fullcmtSize) ;
			if (newCmt == NULL) {
				free(THE_COMMENT) ;
				free(ABSOLUTE_FILENAME) ;
				err_with_message( INTERNAL_ERR, "comment: setComments: Couldn't allocate memory for new comment! Bailing out.\n" ) ;
			}
		
			if (theCmd & CMD_APPEND ) { // TODO: check i debugger
				if ( theOldComment != NULL ) {
					newCmt = strcpy(newCmt,theOldComment );
					if ((THE_COMMENT[0] != COMMENT_DELIMITER ) && theOldComment[strlen(theOldComment)-1] != COMMENT_DELIMITER )
						newCmt = strcat(newCmt,&COMMENT_DELIMITER );
					newCmt = strcat(newCmt,THE_COMMENT);
				} else if (!(theCmd & OPT_TERSE )) {
					newCmt = strcpy(newCmt,THE_COMMENT);
				} else
					err_with_message( INTERNAL_ERR, "comment: setComments: Can't happen terse, old comment == NULL, in the process... Bailing out.\n" ) ;
			} else if ( theCmd & CMD_PREPEND ) {		
				if ( theOldComment != NULL ) {
					newCmt = strcpy(newCmt,THE_COMMENT);
					if (THE_COMMENT[strlen(THE_COMMENT)-1] != COMMENT_DELIMITER && theOldComment[0] != COMMENT_DELIMITER)
						newCmt = strcat(newCmt,&COMMENT_DELIMITER );
					newCmt = strcat(newCmt,theOldComment );
				} else if (!(theCmd & OPT_TERSE )) {
					newCmt = strcpy(newCmt,THE_COMMENT);
				} else
					err_with_message( INTERNAL_ERR, "comment: setComments: Can't happen terse, old comment == NULL, in the process... Bailing out.\n" ) ;
			}		
		} else { // set
			fullcmtSize = strlen(THE_COMMENT) + 1 ;
			newCmt = malloc(fullcmtSize) ;
			if (newCmt == NULL) {
				free(THE_COMMENT) ;
				free(ABSOLUTE_FILENAME) ;
				err_with_message( INTERNAL_ERR, "comment: setComments: Couldn't allocate memory for new comment! Bailing out.\n" ) ;
			}
			newCmt = strcpy(newCmt,THE_COMMENT);
		}
		setFileComment( fn, newCmt ) ;
		free(newCmt) ;
	}
	free(ABSOLUTE_FILENAME) ;

	return ret ;
}

static void sig_alrm(int signo ) {
	err_with_message( INTERNAL_ERR, "comment: setFileComment: The operation timed out! Bailing out.\n" ) ;
}

static void setFileComment(char *fn, char *newCmt ) {
	static bool first_time=true ;
	static char *commandLineFormatString=NULL;
	static size_t cmdLineFmtStrLen = 0 ;
	static int kern_arg_max = 0 ;
	
	if (first_time) {
		kern_arg_max = cmdln_maxarg() ;
		commandLineFormatString=strdup("osascript -e \"tell application \\\"Finder\\\" to 	set the comment of (POSIX file \\\"%s\\\" as alias) to \\\"%s\\\"\" >/dev/null");
		if (commandLineFormatString == NULL) 
			err_with_message( INTERNAL_ERR, "comment: setFileComment: Couldn't allocate memory for the formatstring for the command line! Bailing out.\n" ) ;
		cmdLineFmtStrLen = strlen(commandLineFormatString) ;
		
		if (signal(SIGALRM, sig_alrm) == SIG_ERR) 
			err_with_message( INTERNAL_ERR, "comment: setFileComment: Couldn't install signal handler for SIGALARM. Bailing out.\n"); 
		
		first_time = false ;
	}
	size_t cmdlnSize= cmdLineFmtStrLen + strlen(fn)+ strlen(newCmt) - 4 ;
	if ( cmdlnSize >= kern_arg_max)
		err_with_message( INTERNAL_ERR, "comment: setFileComment: The combined size of osascript commandline bigger than kern_arg_max. Bailing out.\n" ) ;
	static char *cmdBuffer=NULL ;
	cmdBuffer = malloc(cmdlnSize+1) ;
	if (cmdBuffer == NULL) 
		err_with_message( INTERNAL_ERR, "comment: setFileComment: Couldn't allocate mem for cmdBuffer. Bailing out.\n");
	/* TODO : signal handling */				  
	snprintf(cmdBuffer,cmdlnSize+1,commandLineFormatString,fn,newCmt ) ;
	unsigned int myalarm=alarm((unsigned int)1) ;
	// close(STDOUT_FILENO);
	system(cmdBuffer) ;
	myalarm=alarm((unsigned int)0) ;
	// int sout = open("/dev/tty",O_RDONLY );
	free(cmdBuffer) ;
}

static char *getFileComment( CFURLRef existingFile ) {
	static char *theComment=NULL ;
	
	MDItemRef item = MDItemCreateWithURL(kCFAllocatorDefault, existingFile);
	CFStringRef comment = MDItemCopyAttribute(item, kMDItemFinderComment);
	CFRelease(item) ;
	if (comment != NULL ) {
		
		CFIndex cslen = CFStringGetLength(comment) *8 ;
		if (cslen != 0) {
			theComment = malloc((size_t)cslen ) ;
			if (theComment == NULL ) {
				perror("comment: getFileComment, couldn't malloc for theComment");
				CFRelease(comment) ;
				CFRelease(existingFile) ;
				free(ABSOLUTE_FILENAME) ;
				exit(INTERNAL_ERR);
			}
			if (CFStringGetCString (comment,theComment,cslen,kCFStringEncodingUTF8 ) == false) {
				perror("comment: getFileComment, couldn't convert comment to C-string");
				free(theComment);
				CFRelease(comment) ;
				CFRelease(existingFile) ;
				free(ABSOLUTE_FILENAME) ;
				exit(INTERNAL_ERR);
			}
			CFRelease(comment) ;
			size_t slen = strlen(theComment) ;
			theComment = realloc(theComment, (slen+1));
			if (theComment == NULL ) {
				perror("comment: getFileComment, couldn't realloc memory for theComment");
				CFRelease(existingFile) ;
				free(ABSOLUTE_FILENAME) ;
				exit(INTERNAL_ERR);
			}
			return theComment ;
		} else {
			CFRelease(comment) ;
			return NULL ;
		} 		
	} else 
		return NULL ;
}

/*!
    @function
    @abstract   Lists the Finder/Spotlight comment of a single valid file
				to standard output.
    @discussion If the verbose option is set, then a file without a comment
				is listed to standard output as well.
				If the full path option is set, then the full path is returned
				as in contrast to just the given argument for a filename.
    @param      (char *fn : the filename)
				(u_int_16t) theCmd
				(bool multiple) This variable is for knowing that we have more than one
				when obtaining length data for the comment during setting, ignored for
				this function.
    @result     Returns 0 if everything went ok, an error code  if it didn't.
 
	TODO: test for non-existant files.
*/

static int listComments( char *fn,u_int16_t theCmd ) {
	int ret = EXIT_FAILURE ;
	
	CFStringRef thePath=NULL ;
	thePath = CFStringCreateWithCString(NULL ,fn,
											   kCFStringEncodingUTF8);

	CFURLRef myFile = NULL ;
	myFile =  CFURLCreateWithFileSystemPath (NULL,thePath,
											 kCFURLPOSIXPathStyle,FALSE );
	CFRelease(thePath) ;
	char *theComment=NULL ;
	theComment = getFileComment(myFile ) ; // isn't retained when new stack! so I don't free it!
	CFRelease(myFile) ;
	if (theComment != NULL ) {
		if ( (theCmd & OPT_FULLPATH ) == OPT_FULLPATH )
			fprintf(stderr,"%s%c%s\n",ABSOLUTE_FILENAME,DELIMITER,theComment );
		else 
			fprintf(stderr,"%s%c%s\n",fn,DELIMITER,theComment );
		free(theComment);
		ret = EXIT_SUCCESS ;
	} else if ( (theCmd & OPT_VERBOSE ) == OPT_VERBOSE ) {
		if ( (theCmd & OPT_FULLPATH ) == OPT_FULLPATH )
			fprintf(stderr,"%s\n",ABSOLUTE_FILENAME );
		else 
			fprintf(stderr,"%s\n",fn );
	} 
	free(ABSOLUTE_FILENAME) ;
	return ret ;	
}

static void validate_file(char *file_arg, char **absfname )
{
	char *tf=strdup(file_arg) ;
	if (tf == NULL) 
		err_with_message( INTERNAL_ERR, "comment: validate_file: couldn't strdup string for absolute path. Bailing out.\n") ;
	
	tf = full_absolute_path( &tf, strlen(tf), (size_t) MY_MAX_PATH_LEN ) ;
	if (strlen(tf) >= MY_MAX_PATH_LEN) 
		err_with_message(USER_ERROR,"comment: validate_file: the absolute pathname for %s is too long for the OS. Bailing out.\n",tf);
	
	if (maxPathCompLen(tf) >= MY_MAX_NAME_LEN )
		err_with_message(USER_ERROR,"comment: validate_file: one or more of the parts int the filename %s is longer than OS X can handle. Bailing out.\n,tf");
	
	if (fonhfsplus(tf) == false ) 
			err_with_message(USER_ERROR,"comment: validate_file: the file %s is not on a HFS+ filesystem. Bailing out.\n",tf);
	
	if (validFinderFileFn(tf) == false ) 
		err_with_message(USER_ERROR,"comment: validate_file: the file %s is not on a valid name or type for a Finder file. Bailing out.\n",tf);
	*absfname = tf ;
}	
static void err_with_message( int errnum, char *format, ... ) {
	int save_errno=errno;
	va_list args ;
	va_start(args,format) ;
	vfprintf(stderr, format, args) ;
	va_end(args );
	if (errnum == INTERNAL_ERR)
		fprintf(stderr,"%s\n",strerror(save_errno));
	exit(errnum) ;
}
static void err_msg_nofiles(void ) 
{
	fprintf(stderr,"comment: there are no files to operate upon.\n") ;
	usage() ;
	exit(USER_ERROR) ;
	
	
}
static void usage(void) {
	fprintf(stderr,"comment: version 1.0 Copyright © 2013 McUsr and put into Public Domain\n");
	fprintf(stderr,"         under Gnu LPGL 2.0\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"comment: List files with Finder comments  or comments files from commandline\n");
	fprintf(stderr,"         that aren't locked or read only. Either by setting or resetting a \n");
	fprintf(stderr,"		 full new comment, or by prepending or appending and existing comment.\n");
	fprintf(stderr,"         For OS X 10.6 and later. Copyright © 2013 McUsr. GnU GPL 2.0.\n");
	fprintf(stderr,"		 Filenames maybe be read from stdin.\n");
	fprintf(stderr,"	\n");
	fprintf(stderr,"Usage: comment [options]  [color] [1 to n ..file arguments or from stdin\n");
	fprintf(stderr,"	    specified by posix path, one file on each line.]\n");
	fprintf(stderr,"		Any multi wordcomment need to be either enclosed in single double ticks\n");
	fprintf(stderr,"		or have the spaces withing the comment escaped by \\ in order to differ\n");
	fprintf(stderr,"		between the comment and any subsequent file arguments.\n");
	fprintf(stderr,"		   \n");
	fprintf(stderr,"Options\n");
	fprintf(stderr,"-------\n");
	fprintf(stderr,"   comment [-huCvlsapdvtPD]\n");
	fprintf(stderr,"   comment [ --help,--usage,--copyright,--version,--list,--set,--append,\n");
	fprintf(stderr,"             --prepend,--delimiter,--verbose, --terse,--fullpath,-comment-delim ]\n");
	fprintf(stderr,"Details\n");
	fprintf(stderr,"-------\n");
	fprintf(stderr," 	-l [file 1..en] list files and their comments. Respecting any delimiter.\n");
	fprintf(stderr,"	   if verbose is given, then files without comments will also be listed .\n");
	fprintf(stderr,"       (--list).\n");
	fprintf(stderr," 	-P like -l but the absolute path of the file is returned.\n");
	fprintf(stderr,"	-s [comment] [file1..filen] sets/resets a comment for one or more files.\n");
	fprintf(stderr,"       An empty comment are allowed, and will delete any old ones. (--set).\n");
	fprintf(stderr,"	-a [comment] [file1..filen] appends a comment to one or more files.\n");
	fprintf(stderr,"	   A file without a comment, will get their comment field set. if strict\n");
	fprintf(stderr,"	   option isn't specified.(--append).\n");
	fprintf(stderr,"	-p [comment] [file1..filen] prepends  a comment to one or more files.\n");
	fprintf(stderr,"	   A file without a comment, will get their comment field set. If strict\n");
	fprintf(stderr,"	   option isn't specified. (--prepend).\n");
	fprintf(stderr,"	-v [-l] file1..filen] list both files with and without comments. (--verbose).\n");
	fprintf(stderr,"	-t [ap] [comment] [file1..filen] Sees to that only  files with an existing\n");
	fprintf(stderr,"	   comment will have text added to their comment.(--terse).\n");
	fprintf(stderr,"    -d [delimiter] [file1..filen] specifies a delimter to split the filenames\n");
	fprintf(stderr,"	   from the comments.(--delimiter).\n");
	fprintf(stderr,"	-P [filei 1..n] like -l on its own,  but outputs the full absolute paths of\n");
	fprintf(stderr,"       the files. It works with other options that output filenames.\n");
	fprintf(stderr,"	-D [comment delimiter] [ap] [file 1..n] lets you specifier other than space\n");
	fprintf(stderr,"       to differ between the comment, and the new appended/prepended part.\n");
	fprintf(stderr,"	   \n");
	fprintf(stderr,"Examples of usage:\n");
	fprintf(stderr,"------------------\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"	ls |comment -ld :\n");
	fprintf(stderr,"	Lists all files with a comment to stdout as a colon separated list.\n");
	fprintf(stderr,"	ls |comment -vld -\n");
	fprintf(stderr,"	Lists all files  comment or not to stdout as a colon separated list.\n");
	fprintf(stderr,"	-No comment, still colon.\n");
	fprintf(stderr,"	comment -sp 'Threadstone ' *\n");
	fprintf(stderr,"	prepends any files with Finder comments in the current directory with\n");
	fprintf(stderr,"	'Threadstone'.	\n");
	fprintf(stderr,"	comment -a \" DONE $(date)\" *\n");
	fprintf(stderr,"	Appends or sets a comment like 'DONE man jun  3 23:00:18 CEST 2013' into\n");
	fprintf(stderr,"	all files in the current working directory.\n");
}

static void version(void) {
	fprintf(stderr,"comment version 1.0 © 2013 Copyright McUsr and put into Public Domain under GNU LPGL.\n") ;
}

static void copyright(void) {
	version() ;
}

int osx_minor( void ) {
	osxver foundver;
  	char *osverstr= NULL ;
    osverstr=osversionString() ;
    macosx_ver(osverstr, &foundver ) ;
	free(osverstr);
	return foundver.minor ;
}



static char *osversionString(void) {
	int mib[2];
	size_t len;
	char *kernelVersion=NULL;
	mib[0] = CTL_KERN;
	mib[1] = KERN_OSRELEASE;
	
	if (sysctl(mib, 2, NULL, &len, NULL, 0) < 0 ) {
		fprintf(stderr,"%s: Error during sysctl probe call!\n",__PRETTY_FUNCTION__ );
		fflush(stdout);
		exit(INTERNAL_ERR) ;
	}
	kernelVersion = malloc(len );
	if (kernelVersion == NULL ) {
		fprintf(stderr,"%s: Error during malloc!\n",__PRETTY_FUNCTION__ );
		fflush(stdout);
		exit(INTERNAL_ERR) ;
	}
	if (sysctl(mib, 2, kernelVersion, &len, NULL, 0) < 0 ) {
		fprintf(stderr,"%s: Error during sysctl get verstring call!\n",__PRETTY_FUNCTION__ );
		fflush(stdout);
		exit(INTERNAL_ERR) ;
	}
	return kernelVersion ;
}

static void macosx_ver(char *darwinversion, osxver *osxversion ) {
    /*
	 From the book Mac Os X and IOS Internals:
	 In version 10.1.1, Darwin (the core OS) was renumbered from v1.4.1 to 5.1,
	 and since then has followed the OS X numbers consistently by being four
	 numbers ahead of the minor version, and aligning its own minor with the
	 sub-version.
	 */
	char firstelm[2]= {0,0},secElm[2]={0,0};
	
	if (strlen(darwinversion) < 5 ) {
		fprintf(stderr,"%s: %s Can't possibly be a version string. Exiting\n",__PRETTY_FUNCTION__,darwinversion);
		fflush(stdout);
		exit(USER_ERROR);
	}
	char *s=darwinversion,*t=firstelm,*curdot=strchr(darwinversion,'.' );
	
	while ( s != curdot )
		*t++ = *s++;
	t=secElm ;
	curdot=strchr(++s,'.' );
	while ( s != curdot )
		*t++ = *s++;
	int maj=0, min=0;
	maj= (int)strtol(firstelm, (char **)NULL, 10);
	if ( maj == 0 && errno == EINVAL ) {
		fprintf(stderr,"%s Error during conversion of version string\n",__PRETTY_FUNCTION__);
		fflush(stdout);
		exit(INTERNAL_ERR);
	}
	
	min=(int)strtol(secElm, (char **)NULL, 10);
	
	if ( min  == 0 && errno == EINVAL ) {
		fprintf(stderr,"%s: Error during conversion of version string\n",__PRETTY_FUNCTION__);
		fflush(stdout);
		exit(INTERNAL_ERR);
	}
	osxversion->minor=maj-4;
	osxversion->sub=min;
}


int pathLengths( int *max_path_len, int *max_name_len, char *argv_0 ) {
	int fd = open(argv_0,O_RDONLY);
	if (fd == -1) return -1 ;
	if (((*max_path_len = fpathconf( fd, _PC_PATH_MAX ))) && 
		(*max_name_len= fpathconf( fd, _PC_NAME_MAX ))) {
		close(fd);
		return 0;
	}
    close(fd);
	return -1 ;
}
char *full_absolute_path( char **orig_path, size_t orig_len, size_t max_path_len )
{
	char *new_ptr = malloc(max_path_len+1);
	if (new_ptr == NULL ) {
		perror("full_absolute_path couldn't allocate mem for realpath");
		exit(INTERNAL_ERR);
	}
	new_ptr=realpath(*orig_path, new_ptr);
	size_t newlen;
	if ((newlen=strlen(new_ptr))!= orig_len) {
		free(*orig_path);
		*orig_path = new_ptr ;
		*orig_path = realloc(*orig_path,newlen+1) ;
		if (*orig_path == NULL) {
			perror("full_absolute_path couldn't realloc mem for using realpath");
			exit(INTERNAL_ERR);
		}
	} else {
		free(new_ptr);
		new_ptr = NULL ;
	}
	return *orig_path ;
}

bool validFinderFileFn(char *fn) {
	struct stat buf;
		// removes those char's from the end of a file name for that we have 
		// gotten a "pretty ls- listing as input.
		// we -don't handle long ls listings very well...
	if ( fn[strlen(fn)-1] == '@' ) {
		fn[strlen(fn)-1]= '\0' ;
	}
	if (  fn[strlen(fn)-1] == '*' ) {
		fn[strlen(fn)-1]= '\0' ;
	}
	if (strlen(fn) == 2 && (!strcmp( fn,".." ))) return true ;
		// Check that we aren't dealing with a dot-file.
	if ( fn[0] != '.' || ( fn[0] == '.' && strlen(fn) > 1 && ( fn[1] == '.' ||  fn[1] == '/')))  {
			// dotfiles starts with a '.' but if the next char is another '.' or '/' then its a relative path.
		if (lstat(fn, &buf) < 0) return false ;
		
		if (S_ISREG(buf.st_mode))
			return true ;
		else if (S_ISDIR(buf.st_mode))
			return true ;
		else 
			return false;
	} else {
		return false;
	}
}


#define PATH_DELIM '/'
int maxPathCompLen( char *aPath ) {
	char *prev_delim =strchr(aPath, PATH_DELIM) ;
	int path_len = strlen(aPath), max_comp_len = 0 ;
	
	if (prev_delim == NULL) {
		max_comp_len = path_len ;
	} else{
		
		char *next_delim = NULL ;
		if (max_comp_len = prev_delim - aPath )
			--max_comp_len;	
		while ((next_delim=strchr((prev_delim+1), PATH_DELIM))!= NULL) {
			max_comp_len = max(max_comp_len,(next_delim-prev_delim - 1)) ;
			prev_delim = next_delim ;
			if ((*(prev_delim+1)) == '\0') 
				break ;
		}
		if ((*(prev_delim+1)) != '\0') { // may have a rest without any component.
			max_comp_len = max(max_comp_len,strlen(prev_delim+1)) ;
		}
	}
	return max_comp_len ;
}
#define HFSPLUS 0x11
bool fonhfsplus(char *fname ) {
	struct statfs stat;
	char buf[FILENAME_MAX+1];
	strncpy(buf,fname,FILENAME_MAX);
	
    if (!statfs(buf, &stat)) {
		if (stat.f_type == HFSPLUS ) 
			return true;
		else 
			return false;
    } else {
		fprintf(stderr,"%s: problems call fstat on %s : the file may not exist\n",__PRETTY_FUNCTION__,buf);
        perror("fonhfsplus");
		exit(EXIT_FAILURE);
    }
	
}

int cmdln_maxarg(void ) {
	int				mib[2];
	int				kern_arg_max=0;
	size_t			sz_kern_arg_max = sizeof(int) ;
	mib[0] = CTL_KERN;
	mib[1] = KERN_ARGMAX;
	if (sysctl(mib, 2, &kern_arg_max, &sz_kern_arg_max, NULL, 0) < 0 ) {
		fprintf(stderr,"%s: Error during sysctl get KERN_ARGMAX call!\n",__PRETTY_FUNCTION__ );
		fflush(stdout);
		exit(INTERNAL_ERR );
	}
	return kern_arg_max ;
}

char * baseNameCheck( char* pathToCheck)
{
 	/*  The basename() function returns the last component from the pathname
	 pointed to by path, deleting any trailing `/' characters.  If path consists
	 entirely of`/' characters, a pointer to the string "/" is returned.
	 If path is a null pointer or the empty string, a pointer to the string "."
	 is returned.
     */
	char aSlash[]="/";
	size_t slen=strlen(pathToCheck) ;
	bool endingSlash=false ;
	if (pathToCheck[slen-1] == '/') 
     	endingSlash = true ;
	
	char *basePtr= basename(pathToCheck) ;
	if ( basePtr == NULL ) {
     	perror("baseNameCheck: I couldn't get a valid basename back");
     	exit(INTERNAL_ERR);
		
	} else if (!strcmp(basePtr,".")) {
		perror("baseNameCheck: Illgal basename >.<");
     	exit(INTERNAL_ERR);
	} else if (!strcmp(basePtr,"/")) {
		perror("baseNameCheck: Illgal basename >/<");
     	exit(INTERNAL_ERR);
	}
	if (endingSlash ) {
     	basePtr=realloc(basePtr,strlen(basePtr)+2);
     	strcat(basePtr,aSlash) ;
	}
	return basePtr ;
}


