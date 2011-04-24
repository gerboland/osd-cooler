//!!!!!!!!!!!!!!
//
// FIXME:
// WARNING: 1. to be fixed: memory leaking,
//             check for strdup malloc fopen etc.
//          2. infinite loop upon network error
//
//!!!!!!!!!!!!!!
/*
 * wget - retrieve a file using HTTP or FTP
 *
 * Chip Rosenthal Covad Communications <chip@laserlink.net>
 *
 * butchered to get us a wget client API. 
 *     Neuros Technology LLC --- 2007-06-08 <mgao@neuros>
 *
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

//#define OSD_DBG_MSG
#include "nc-err.h"
#include "nc-type.h"

struct host_info {
	char *host;
	int port;
	char *path;
	int is_ftp;
	char *user;
};

static FILE *open_socket(struct sockaddr_in *s_in);
static char *gethdr(char *buf, size_t bufsiz, FILE *fp, int *istrunc);
static int ftpcmd(char *s1, char *s2, FILE *fp, char *buf);

/* Globals (can be accessed from signal handlers */
static off_t filesize = 0;		/* content-length of the file */
static int chunked = 0;			/* chunked transfer encoding */
#ifdef CONFIG_FEATURE_WGET_STATUSBAR
static void progressmeter(int flag);
static char *curfile;			/* Name of current file being transferred. */
static struct timeval start;	/* Time a transfer started. */
static volatile unsigned long statbytes = 0; /* Number of bytes transferred so far. */
/* For progressmeter() -- number of seconds before xfer considered "stalled" */
static const int STALLTIME = 5;
#endif

static int safe_strtoul(char *arg, unsigned long* value)
{
	char *endptr;
	int errno_save = errno;

	assert(arg!=NULL);
	errno = 0;
	*value = strtoul(arg, &endptr, 0);
	if (errno != 0 || *endptr!='\0' || endptr==arg) {
		return 1;
	}
	errno = errno_save;
	return 0;
}

/* Find out if the last character of a string matches the one given Don't
 * underrun the buffer if the string length is 0.  Also avoids a possible
 * space-hogging inline of strlen() per usage.
 */
static char * last_char_is(const char *s, int c)
{
	char *sret = (char *)s;
	if (sret) {
		sret = strrchr(sret, c);
		if(sret != NULL && *(sret+1) != 0)
			sret = NULL;
	}
	return sret;
}
/*
static char *concat_path_file(const char *path, const char *filename)
{
	char *lc;

	if (!path)
		path = "";
	lc = last_char_is(path, '/');
	while (*filename == '/')
		filename++;
	return bb_xasprintf("%s%s%s", path, (lc==NULL ? "/" : ""), filename);
}
*/
static void chomp(char *s)
{
	char *lc = last_char_is(s, '\n');

	if(lc)
		*lc = 0;
}

static struct hostent *xgethostbyname(const char *name)
{
	struct hostent *retval;

	DBGLOG("get host name: %s\n", name);
	if ((retval = gethostbyname(name)) == NULL)
	{
		WARNLOG("unable to get host by name: %s\n", name);
	}
	return retval;
}

/* Return network byte ordered port number for a service.
 * If "port" is a number use it as the port.
 * If "port" is a name it is looked up in /etc/services, if it isnt found return
 * default_port
 */
static unsigned short bb_lookup_port(const char *port, const char *protocol, unsigned short default_port)
{
	unsigned short port_nr = htons(default_port);

	if (port) 
	{
		char *endptr;
		int old_errno;
		long port_long;

		/* Since this is a lib function, we're not allowed to reset errno to 0.
		 * Doing so could break an app that is deferring checking of errno. */
		old_errno = errno;
		errno = 0;
		port_long = strtol(port, &endptr, 10);
		if (errno != 0 || *endptr!='\0' || endptr==port || port_long < 0 || port_long > 65535) {
			struct servent *tserv = getservbyname(port, protocol);
			if (tserv) {
				port_nr = tserv->s_port;
			}
		} else {
			port_nr = htons(port_long);
		}
		errno = old_errno;
	}
	return port_nr;
}

static void bb_lookup_host(struct sockaddr_in *s_in, const char *host)
{
	struct hostent *he;

	memset(s_in, 0, sizeof(struct sockaddr_in));
	s_in->sin_family = AF_INET;
	he = xgethostbyname(host);
	memcpy(&(s_in->sin_addr), he->h_addr_list[0], he->h_length);
}

static int xconnect(struct sockaddr_in *s_addr)
{
	int s = socket(AF_INET, SOCK_STREAM, 0);
	if (connect(s, (struct sockaddr *)s_addr, sizeof(struct sockaddr_in)) < 0)
	{
		close(s);
		WARNLOG("Unable to connect to remote host (%s)",
				inet_ntoa(s_addr->sin_addr));
	}
	return s;
}

static int parse_url(char *url, struct host_info *h)
{
	char *cp, *sp, *up, *pp;
	int ret = -1;

	if (strncmp(url, "http://", 7) == 0) 
	{
		DBGLOG("http url.\n");
		h->port = bb_lookup_port("http", "tcp", 80);
		h->host = url + 7;
		h->is_ftp = 0;
	} 
	else if (strncmp(url, "ftp://", 6) == 0) 
	{
		DBGLOG("ftp url.\n");
		h->port = bb_lookup_port("ftp", "tfp", 21);
		h->host = url + 6;
		h->is_ftp = 1;
	} 
	else
	{
		WARNLOG("not an http or ftp url: %s", url);
		goto bail;
	}

	sp = strchr(h->host, '/');
	if (sp) 
	{
		*sp++ = '\0';
		h->path = sp;
	} 
	else h->path = strdup(" ");

	up = strrchr(h->host, '@');
	if (up != NULL) 
	{
		h->user = h->host;
		*up++ = '\0';
		h->host = up;
	} 
	else h->user = NULL;

	pp = h->host;

#ifdef CONFIG_FEATURE_WGET_IP6_LITERAL
	if (h->host[0] == '[') 
	{
		char *ep;

		ep = h->host + 1;
		while (*ep == ':' || isxdigit (*ep))
			ep++;
		if (*ep == ']') 
		{
			h->host++;
			*ep = '\0';
			pp = ep + 1;
		}
	}
#endif

	cp = strchr(pp, ':');
	if (cp != NULL) 
	{
		*cp++ = '\0';
		h->port = htons(atoi(cp));
	}
	ret = 0;
	DBGLOG("url successfully parsed.\n");
 bail:
	return ret;
}


/* Read NMEMB elements of SIZE bytes into PTR from STREAM.  Returns the
 * number of elements read, and a short count if an eof or non-interrupt
 * error is encountered.  */
static size_t safe_fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	size_t ret = 0;

	do {
		clearerr(stream);
		ret += fread((char *)ptr + (ret * size), size, nmemb - ret, stream);
	} while (ret < nmemb && ferror(stream) && errno == EINTR);

	return ret;
}

/* Read a line or SIZE - 1 bytes into S, whichever is less, from STREAM.
 * Returns S, or NULL if an eof or non-interrupt error is encountered.  */
static char *safe_fgets(char *s, int size, FILE *stream)
{
	char *ret;

	do {
		clearerr(stream);
		ret = fgets(s, size, stream);
	} while (ret == NULL && ferror(stream) && errno == EINTR);

	return ret;
}


#ifdef CONFIG_FEATURE_WGET_AUTHENTICATION
/*
 *  Base64-encode character string
 *  oops... isn't something similar in uuencode.c?
 *  XXX: It would be better to use already existing code
 */
static char *base64enc(unsigned char *p, char *buf, int len) {

	char al[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
		    "0123456789+/";
		char *s = buf;

	while(*p) {
				if (s >= buf+len-4)
					WARNLOG("buffer overflow");
		*(s++) = al[(*p >> 2) & 0x3F];
		*(s++) = al[((*p << 4) & 0x30) | ((*(p+1) >> 4) & 0x0F)];
		*s = *(s+1) = '=';
		*(s+2) = 0;
		if (! *(++p)) break;
		*(s++) = al[((*p << 2) & 0x3C) | ((*(p+1) >> 6) & 0x03)];
		if (! *(++p)) break;
		*(s++) = al[*(p++) & 0x3F];
	}

		return buf;
}
#endif

#define WGET_OPT_CONTINUE	1
#define WGET_OPT_QUIET	2
#define WGET_OPT_PASSIVE	4
#define WGET_OPT_OUTNAME	8
#define WGET_OPT_HEADER	16
#define WGET_OPT_PREFIX	32
#define WGET_OPT_PROXY	64

/*
        -c      continue retrieval of aborted transfers
        -q      quiet mode - do not print
        -P      Set directory prefix to DIR
        -O      save to filename ('-' for stdout)
        -Y      use proxy ('on' or 'off')
*/

#define INTERNAL_BUFLEN 512
typedef struct
{
	struct host_info target;
	FILE *sfp;                /* socket to web/ftp server          */
	FILE *dfp;                /* socket to ftp server (data)       */
	int got_clen;             /* got content-length: from server   */
	int use_proxy;            /* Use proxies if env vars are set   */
	//char *proxy_flag = "on";	/* Use proxies if env vars are set  */

	char buf[INTERNAL_BUFLEN];/* internal buffer.                  */
	int buf_bytes;            /* number of bytes in buffer.        */
	int buf_offset;           /* internal buffer offset.           */
} wget_handle_t;

void * CoolWgetOpen(const char * url, int option)
{
	struct host_info server;
	wget_handle_t * hdl;
	int bytes;

	int try=5, status;
#if 0
	int port;
	char *proxy = 0;
	char *dir_prefix=NULL;
#endif
	char *s, buf[512];
	//struct stat sbuf;
	char extra_headers[1024];
	//char *extra_headers_ptr = extra_headers;
	int extra_headers_left = sizeof(extra_headers);

	struct sockaddr_in s_in;

	int do_continue = 0;		/* continue a prev transfer (-c)    */
	long beg_range = 0L;		/*   range at which continue begins */
	
	int quiet_flag = FALSE;		/* Be verry, verry quiet...	    */

	char * cp_url = NULL;

	hdl = (wget_handle_t *)malloc(sizeof(wget_handle_t));
	if (!hdl)
	{
		WARNLOG("unable to create wget handle.\n");
		goto bail;
	}
	memset(hdl, 0, sizeof(wget_handle_t));

	cp_url = strdup(url);
	DBGLOG("url = %s\n", cp_url);
	if (parse_url((char*)cp_url, &hdl->target)) goto bail;

	server.host = hdl->target.host;
	server.port = hdl->target.port;

	/*
	 * Use the proxy if necessary.
	 */
/*
	if (hdl->use_proxy) {
		proxy = getenv(hdl->target.is_ftp ? "ftp_proxy" : "http_proxy");
		if (proxy && *proxy) {
			parse_url(bb_xstrdup(proxy), &server);
		} else {
			hdl->use_proxy = 0;
		}
	}
*/
	/* Guess an output filename */
/*
	if (!fname_out) {
		// Dirty hack. Needed because bb_get_last_path_component
		// will destroy trailing / by storing '\0' in last byte!
		if(*hdl->target.path && hdl->target.path[strlen(hdl->target.path)-1]!='/') {
			fname_out =
#ifdef CONFIG_FEATURE_WGET_STATUSBAR
				curfile =
#endif
				bb_get_last_path_component(hdl->target.path);
		}
		if (fname_out==NULL || strlen(fname_out)<1) {
			fname_out =
#ifdef CONFIG_FEATURE_WGET_STATUSBAR
				curfile =
#endif
				"index.html";
		}
		if (dir_prefix != NULL)
			fname_out = concat_path_file(dir_prefix, fname_out);
#ifdef CONFIG_FEATURE_WGET_STATUSBAR
	} else {
		curfile = bb_get_last_path_component(fname_out);
#endif
	}
	if (do_continue && !fname_out)
		WARNLOG("cannot specify continue (-c) without a filename (-O)");
*/

	/* We want to do exactly _one_ DNS lookup, since some
	 * sites (i.e. ftp.us.debian.org) use round-robin DNS
	 * and we want to connect to only one IP... */
	bb_lookup_host(&s_in, server.host);
	DBGLOG("host looked up.\n");
	s_in.sin_port = server.port;
	if (quiet_flag==FALSE) 
	{
		fprintf(stdout, "Connecting to %s[%s]:%d\n",
				server.host, inet_ntoa(s_in.sin_addr), ntohs(server.port));
	}

	//if (hdl->use_proxy || !hdl->target.is_ftp) 
	{
		/*
		 *  HTTP session
		 */
		do {
			hdl->got_clen = chunked = 0;

			if (! --try)
				WARNLOG("too many redirections");

			/*
			 * Open socket to http server
			 */
			if (hdl->sfp) fclose(hdl->sfp);
			hdl->sfp = open_socket(&s_in);

			/*
			 * Send HTTP request.
			 */
			if (hdl->use_proxy) {
				const char *format = "GET %stp://%s:%d/%s HTTP/1.1\r\n";
#ifdef CONFIG_FEATURE_WGET_IP6_LITERAL
				if (strchr (hdl->target.host, ':'))
					format = "GET %stp://[%s]:%d/%s HTTP/1.1\r\n";
#endif
				fprintf(hdl->sfp, format,
					hdl->target.is_ftp ? "f" : "ht", hdl->target.host,
					ntohs(hdl->target.port), hdl->target.path);
			} else {
				fprintf(hdl->sfp, "GET /%s HTTP/1.1\r\n", hdl->target.path);
			}

			fprintf(hdl->sfp, "Host: %s\r\nUser-Agent: Wget\r\n", hdl->target.host);

#ifdef CONFIG_FEATURE_WGET_AUTHENTICATION
			if (hdl->target.user) {
				fprintf(hdl->sfp, "Authorization: Basic %s\r\n",
					base64enc((unsigned char*)hdl->target.user, buf, sizeof(buf)));
			}
			if (hdl->use_proxy && server.user) {
				fprintf(hdl->sfp, "Proxy-Authorization: Basic %s\r\n",
					base64enc((unsigned char*)server.user, buf, sizeof(buf)));
			}
#endif

			if (do_continue)
				fprintf(hdl->sfp, "Range: bytes=%ld-\r\n", beg_range);
			if(extra_headers_left < sizeof(extra_headers))
				fputs(extra_headers,hdl->sfp);
			fprintf(hdl->sfp,"Connection: close\r\n\r\n");

			/*
			* Retrieve HTTP response line and check for "200" status code.
			*/
read_response:
			if (fgets(buf, sizeof(buf), hdl->sfp) == NULL)
				WARNLOG("no response from server");

			for (s = buf ; *s != '\0' && !isspace(*s) ; ++s)
			;
			for ( ; isspace(*s) ; ++s)
			;
			switch (status = atoi(s)) {
				case 0:
				case 100:
					while (gethdr(buf, sizeof(buf), hdl->sfp, &bytes) != NULL);
					goto read_response;
				case 200:
					//if (do_continue && output != stdout)
					//	output = freopen(fname_out, "w", output);
					do_continue = 0;
					break;
				case 300:	/* redirection */
				case 301:
				case 302:
				case 303:
					break;
				case 206:
					if (do_continue)
						break;
					/*FALLTHRU*/
				default:
					chomp(buf);
					WARNLOG("server returned error %d: %s", atoi(s), buf);
			}

			/*
			 * Retrieve HTTP headers.
			 */
			while ((s = gethdr(buf, sizeof(buf), hdl->sfp, &bytes)) != NULL) {
				if (strcasecmp(buf, "content-length") == 0) {
					unsigned long value;
					if (safe_strtoul(s, &value)) {
						WARNLOG("content-length %s is garbage", s);
					}
					filesize = value;
					hdl->got_clen = 1;
					continue;
				}
				if (strcasecmp(buf, "transfer-encoding") == 0) {
					if (strcasecmp(s, "chunked") == 0) {
						chunked = hdl->got_clen = 1;
					} else {
					WARNLOG("server wants to do %s transfer encoding", s);
					}
				}
				if (strcasecmp(buf, "location") == 0) {
					if (s[0] == '/')
						hdl->target.path = strdup(s+1);
					else {
						parse_url(strdup(s), &hdl->target);
						if (hdl->use_proxy == 0) {
							server.host = hdl->target.host;
							server.port = hdl->target.port;
						}
						bb_lookup_host(&s_in, server.host);
						s_in.sin_port = server.port;
						break;
					}
				}
			}
		} while(status >= 300);

		hdl->dfp = hdl->sfp;
	}
#if 0
	else
	{
		/*
		 *  FTP session
		 */
		if (! hdl->target.user)
			hdl->target.user = bb_xstrdup("anonymous:busybox@");

		hdl->sfp = open_socket(&s_in);
		if (ftpcmd(NULL, NULL, hdl->sfp, buf) != 220)
			WARNLOG("%s", buf+4);

		/*
		 * Splitting username:password pair,
		 * trying to log in
		 */
		s = strchr(hdl->target.user, ':');
		if (s)
			*(s++) = '\0';
		switch(ftpcmd("USER ", hdl->target.user, hdl->sfp, buf)) {
			case 230:
				break;
			case 331:
				if (ftpcmd("PASS ", s, hdl->sfp, buf) == 230)
					break;
				/* FALLTHRU (failed login) */
			default:
				WARNLOG("ftp login: %s", buf+4);
		}

		ftpcmd("CDUP", NULL, hdl->sfp, buf);
		ftpcmd("TYPE I", NULL, hdl->sfp, buf);

		/*
		 * Querying file size
		 */
		if (ftpcmd("SIZE /", hdl->target.path, hdl->sfp, buf) == 213) {
			unsigned long value;
			if (safe_strtoul(buf+4, &value)) {
				WARNLOG("SIZE value is garbage");
			}
			filesize = value;
			hdl->got_clen = 1;
		}

		/*
		 * Entering passive mode
		 */
		if (ftpcmd("PASV", NULL, hdl->sfp, buf) !=  227)
			WARNLOG("PASV: %s", buf+4);
		s = strrchr(buf, ',');
		*s = 0;
		port = atoi(s+1);
		s = strrchr(buf, ',');
		port += atoi(s+1) * 256;
		s_in.sin_port = htons(port);
		hdl->dfp = open_socket(&s_in);

		if (do_continue) {
			sprintf(buf, "REST %ld", beg_range);
			if (ftpcmd(buf, NULL, hdl->sfp, buf) != 350) {
				if (output != stdout)
					output = freopen(fname_out, "w", output);
				do_continue = 0;
			} else
				filesize -= beg_range;
		}

		if (ftpcmd("RETR /", hdl->target.path, hdl->sfp, buf) > 150)
			WARNLOG("RETR: %s", buf+4);

	}
#endif

	if (chunked) 
	{
		fgets(buf, sizeof(buf), hdl->dfp);
		filesize = strtol(buf, (char **) NULL, 16);
	}

#ifdef CONFIG_FEATURE_WGET_STATUSBAR
	if (quiet_flag==FALSE)
		progressmeter(-1);
#endif

	hdl->buf_offset = 0;
	hdl->buf_bytes = 0;

	return hdl;
 bail:
	if (hdl) 
	{
		free(hdl);
	}
	if (cp_url) free(cp_url);
	return NULL;
}

void CoolWgetClose(void * handle)
{
	wget_handle_t * hdl = (wget_handle_t*)handle;
	char buf[512];

#ifdef CONFIG_FEATURE_WGET_STATUSBAR
	if (quiet_flag==FALSE)
		progressmeter(1);
#endif

	if ((hdl->use_proxy == 0) && hdl->target.is_ftp) 
	{
		fclose(hdl->dfp);
		if (ftpcmd(NULL, NULL, hdl->sfp, buf) != 226)
			WARNLOG("ftp error: %s", buf+4);
		ftpcmd("QUIT", NULL, hdl->sfp, buf);
	}

	if (handle) free(handle);
}

int CoolWget(void * handle, void * outbuf, int buflen)
{
	wget_handle_t * hdl = (wget_handle_t*)handle;
	int bytes = 0;
	int offset = 0;
	char buf[512];

	/*
	 * Retrieve file
	 */
	do 
	{
	retry:
		if (hdl->buf_bytes >= buflen)
		{
			DBGLOG("data all contained in internal buffer.\n");
			memcpy(outbuf+offset, hdl->buf+hdl->buf_offset, buflen);
			hdl->buf_offset += buflen;
			hdl->buf_bytes -= buflen;
			return offset+buflen;
		}
		else if (hdl->buf_bytes)
		{
			DBGLOG("data partially contained in internal buffer.\n");
			memcpy(outbuf+offset, hdl->buf+hdl->buf_offset, hdl->buf_bytes);
			buflen -= hdl->buf_bytes;
			offset += hdl->buf_bytes;

			hdl->buf_offset = 0;
			hdl->buf_bytes = 0;
		}
		DBGLOG("empty internal buffer.\n");
		while (buflen >= INTERNAL_BUFLEN)
		{
			DBGLOG("read directly into buffer.\n");
			if ((filesize > 0 || !hdl->got_clen) && 
				(bytes = safe_fread(outbuf+offset, 1, 
									((chunked || hdl->got_clen) && (filesize < buflen)) ? 
									filesize : buflen, hdl->dfp)) > 0) 
			{
#ifdef CONFIG_FEATURE_WGET_STATUSBAR
				statbytes+=bytes;
#endif
				if (hdl->got_clen) 
				{
					filesize -= bytes;
				}				
				offset += bytes;
				buflen -= bytes;
				DBGLOG("read directly into buffer, bytes read = %d.\n", bytes);
			}
			else break;
		}

		if (!buflen) break;

		if (buflen < INTERNAL_BUFLEN)
		{
			DBGLOG("read into internal buffer.\n");
			if ((filesize > 0 || !hdl->got_clen) && 
				(bytes = safe_fread(hdl->buf+hdl->buf_offset, 1, 
									((chunked || hdl->got_clen) && (filesize < INTERNAL_BUFLEN)) ? 
									filesize : INTERNAL_BUFLEN, hdl->dfp)) > 0) 
			{
#ifdef CONFIG_FEATURE_WGET_STATUSBAR
				statbytes+=bytes;
#endif
				if (hdl->got_clen) 
				{
					filesize -= bytes;
				}				
				hdl->buf_bytes += bytes;
				DBGLOG("read into internal buffer, bytes read = %d.\n", bytes);
				goto retry;
			}
		}

		if (chunked) 
		{
			safe_fgets(buf, sizeof(buf), hdl->dfp); /* This is a newline */
			safe_fgets(buf, sizeof(buf), hdl->dfp);
			filesize = strtol(buf, (char **) NULL, 16);
			if (filesize==0) 
			{
				chunked = 0; /* all done! */
				DBGLOG("all done.\n");
				break;
			}
		}
		
		if (bytes == 0 && ferror(hdl->dfp)) 
		{
			WARNLOG("network read error");
			break;
		}
	} while (chunked);

	return offset;
}


/** Get file from given url.
 *
 *
 */
int CoolWgetFile(const char * url, int option, const char * fname)
{
	return 0;
}


FILE *open_socket(struct sockaddr_in *s_in)
{
	FILE *fp;

	fp = fdopen(xconnect(s_in), "r+");
	if (fp == NULL)
		WARNLOG("fdopen()");

	return fp;
}


char *gethdr(char *buf, size_t bufsiz, FILE *fp, int *istrunc)
{
	char *s, *hdrval;
	int c;

	*istrunc = 0;

	/* retrieve header line */
	if (fgets(buf, bufsiz, fp) == NULL)
		return NULL;

	/* see if we are at the end of the headers */
	for (s = buf ; *s == '\r' ; ++s)
		;
	if (s[0] == '\n')
		return NULL;

	/* convert the header name to lower case */
	for (s = buf ; isalnum(*s) || *s == '-' ; ++s)
		*s = tolower(*s);

	/* verify we are at the end of the header name */
	if (*s != ':')
		WARNLOG("bad header line: %s", buf);

	/* locate the start of the header value */
	for (*s++ = '\0' ; *s == ' ' || *s == '\t' ; ++s)
		;
	hdrval = s;

	/* locate the end of header */
	while (*s != '\0' && *s != '\r' && *s != '\n')
		++s;

	/* end of header found */
	if (*s != '\0') {
		*s = '\0';
		return hdrval;
	}

	/* Rats!  The buffer isn't big enough to hold the entire header value. */
	while (c = getc(fp), c != EOF && c != '\n')
		;
	*istrunc = 1;
	return hdrval;
}

static int ftpcmd(char *s1, char *s2, FILE *fp, char *buf)
{
	if (s1) {
		if (!s2) s2="";
		fprintf(fp, "%s%s\r\n", s1, s2);
		fflush(fp);
	}

	do {
		char *buf_ptr;

		if (fgets(buf, 510, fp) == NULL) {
			WARNLOG("fgets()");
		}
		buf_ptr = strstr(buf, "\r\n");
		if (buf_ptr) {
			*buf_ptr = '\0';
		}
	} while (! isdigit(buf[0]) || buf[3] != ' ');

	return atoi(buf);
}

#ifdef CONFIG_FEATURE_WGET_STATUSBAR
/* Stuff below is from BSD rcp util.c, as added to openshh.
 * Original copyright notice is retained at the end of this file.
 *
 */


static int
getttywidth(void)
{
	int width=0;
	get_terminal_width_height(0, &width, NULL);
	return (width);
}

static void
updateprogressmeter(int ignore)
{
	int save_errno = errno;

	progressmeter(0);
	errno = save_errno;
}

static void
alarmtimer(int wait)
{
	struct itimerval itv;

	itv.it_value.tv_sec = wait;
	itv.it_value.tv_usec = 0;
	itv.it_interval = itv.it_value;
	setitimer(ITIMER_REAL, &itv, NULL);
}


static void
progressmeter(int flag)
{
	static const char prefixes[] = " KMGTP";
	static struct timeval lastupdate;
	static off_t lastsize, totalsize;
	struct timeval now, td, wait;
	off_t cursize, abbrevsize;
	double elapsed;
	int ratio, barlength, i, remaining;
	char buf[256];

	if (flag == -1) {
		(void) gettimeofday(&start, (struct timezone *) 0);
		lastupdate = start;
		lastsize = 0;
		totalsize = filesize; /* as filesize changes.. */
	}

	(void) gettimeofday(&now, (struct timezone *) 0);
	cursize = statbytes;
	if (totalsize != 0 && !chunked) {
		ratio = 100.0 * cursize / totalsize;
		ratio = MAX(ratio, 0);
		ratio = MIN(ratio, 100);
	} else
		ratio = 100;

	snprintf(buf, sizeof(buf), "\r%-20.20s %3d%% ", curfile, ratio);

	barlength = getttywidth() - 51;
	if (barlength > 0) {
		i = barlength * ratio / 100;
		snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
			 "|%.*s%*s|", i,
			 "*****************************************************************************"
			 "*****************************************************************************",
			 barlength - i, "");
	}
	i = 0;
	abbrevsize = cursize;
	while (abbrevsize >= 100000 && i < sizeof(prefixes)) {
		i++;
		abbrevsize >>= 10;
	}
	snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), " %5d %c%c ",
	     (int) abbrevsize, prefixes[i], prefixes[i] == ' ' ? ' ' :
		 'B');

	timersub(&now, &lastupdate, &wait);
	if (cursize > lastsize) {
		lastupdate = now;
		lastsize = cursize;
		if (wait.tv_sec >= STALLTIME) {
			start.tv_sec += wait.tv_sec;
			start.tv_usec += wait.tv_usec;
		}
		wait.tv_sec = 0;
	}
	timersub(&now, &start, &td);
	elapsed = td.tv_sec + (td.tv_usec / 1000000.0);

	if (wait.tv_sec >= STALLTIME) {
		snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
			 " - stalled -");
	} else if (statbytes <= 0 || elapsed <= 0.0 || cursize > totalsize || chunked) {
		snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
			 "   --:-- ETA");
	} else {
		remaining = (int) (totalsize / (statbytes / elapsed) - elapsed);
		i = remaining / 3600;
		if (i)
			snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
				 "%2d:", i);
		else
			snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
				 "   ");
		i = remaining % 3600;
		snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
			 "%02d:%02d ETA", i / 60, i % 60);
	}
	write(STDERR_FILENO, buf, strlen(buf));

	if (flag == -1) {
		struct sigaction sa;
		sa.sa_handler = updateprogressmeter;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = SA_RESTART;
		sigaction(SIGALRM, &sa, NULL);
		alarmtimer(1);
	} else if (flag == 1) {
		alarmtimer(0);
		statbytes = 0;
		putc('\n', stderr);
	}
}
#endif


/* Original copyright notice which applies to the CONFIG_FEATURE_WGET_STATUSBAR stuff,
 * much of which was blatantly stolen from openssh.  */

/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. <BSD Advertising Clause omitted per the July 22, 1999 licensing change
 *		ftp://ftp.cs.berkeley.edu/pub/4bsd/README.Impt.License.Change>
 *
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	$Id: wget.c,v 1.1.1.1 2006/06/07 15:27:18 jsantiago Exp $
 */


