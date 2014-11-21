/*      brokensym.c
 *
 *	Copyright 2011 Bob Parker <rlp1938@gmail.com>
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *	MA 02110-1301, USA.
*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>


int filecount;

void *memmem(const void *haystack, size_t haystacklen,
             const void *needle, size_t needlelen);

struct filedata {
    char *from; // start of file content
    char *to;   // last byte of data + 1
};

void help_print(int forced);
void recursedir(char *headdir);
static void dorealpath(char *givenpath, char *resolvedpath);


char *helptext = "\n\tUsage: brokensym [option] [search_dir]\n"
  "\n\tOptions:\n"
  "\t-h outputs this help message.\n"
  "\n\tDESCRIPTION"
  "\tbrokensym recurses search_dir writing any broken symlinks found\n"
  "\tto stdout. Any other errors found will be output to stderr.\n"
  "\tNB the default search_dir is the users home directory."
  ;

int main(int argc, char **argv)
{
	int opt;
	struct stat sb;
    char topdir[PATH_MAX];

	// initialise defaults
    strcpy(topdir, getenv("HOME"));

	while((opt = getopt(argc, argv, ":h")) != -1) {
		switch(opt){
		case 'h':
			help_print(0);
		break;
		case ':':
			fprintf(stderr, "Option %c requires an argument\n",optopt);
			help_print(1);
		break;
		case '?':
			fprintf(stderr, "Illegal option: %c\n",optopt);
			help_print(1);
		break;
		} //switch()
	}//while()
	// now process the non-option arguments

	// 1.Check that argv[???] exists.
	if ((topdir)) {
		strcpy(topdir, topdir);	// default is $HOME
		// Convert relative path to absolute if needed.
		if (topdir[0] != '/') dorealpath(argv[optind], topdir);
	}

	// 2. Check that the dir exists.
	if ((stat(topdir, &sb)) == -1){
		perror(topdir);
		help_print(EXIT_FAILURE);
	}

	// 3. Check that this is a dir
	if (!(S_ISDIR(sb.st_mode))) {
		fprintf(stderr, "Not a directory: %s\n", topdir);
		help_print(EXIT_FAILURE);
	}

	/*
	 * This is what this program does.
	 * It recursively searches the input dir and checks any
	 * symlinks found. If the link is broken, it's listed on
	 * stdout.
	 * Any other errors are output to stderr using perror().
	 *
	 */

	// Recurse
	recursedir(topdir);

	return 0;
} // main()

void help_print(int forced){
    fputs(helptext, stderr);
    exit(forced);
} // help_print()

void recursedir(char *headdir)
{
	/* open the dir at headdir and process according to file type.
	*/
	DIR *dirp;
	struct dirent *de;

	dirp = opendir(headdir);
	if (!(dirp)) {
		perror(headdir);
		exit(EXIT_FAILURE);
	}
	/*
	struct dirent {
		ino_t          d_ino;       // inode number
        off_t          d_off;       // not an offset; see NOTES
        unsigned short d_reclen;    // length of this record
        unsigned char  d_type;      // type of file; not supported
                                    // by all filesystem types
        char           d_name[256]; // filename
	};

	DT_BLK      This is a block device.
    DT_CHR      This is a character device.
    DT_DIR      This is a directory.
    DT_FIFO     This is a named pipe (FIFO).
    DT_LNK      This is a symbolic link.
    DT_REG      This is a regular file.
    DT_SOCK     This is a UNIX domain socket.
    DT_UNKNOWN  The file type is unknown.

	*/
	while((de = readdir(dirp))) {
		if (strcmp(de->d_name, "..") == 0) continue;
		if (strcmp(de->d_name, ".") == 0) continue;
		switch(de->d_type) {
			char newdir[FILENAME_MAX];
			struct stat sb;
			// Nothing to do for these.
			case DT_BLK:
			case DT_CHR:
			case DT_FIFO:
			case DT_SOCK:
			case DT_REG:
			break;
			case DT_LNK:
			strcpy(newdir, headdir);
			strcat(newdir, "/");
			strcat(newdir, de->d_name);
			if (stat(newdir, &sb) == -1) {
				switch(errno) {
					case ENOENT:
					fprintf(stdout, "%s\n", newdir);
					break;
					default:
					perror(newdir);
					break;
				}
			break;
			}
			case DT_DIR:
			// recurse using this pathname.
			strcpy(newdir, headdir);
			strcat(newdir, "/");
			strcat(newdir, de->d_name);
			recursedir(newdir);
			break;
			// Just report the error but do nothing else.
			case DT_UNKNOWN:
			fprintf(stderr, "Unknown type:\n%s/%s\n\n", headdir,
					de->d_name);
			break;
		} // switch()
	} // while
	closedir(dirp);
} // recursedir()

void dorealpath(char *givenpath, char *resolvedpath)
{	// realpath() witherror handling.
	if(!(realpath(givenpath, resolvedpath))) {
		perror("realpath()");
		exit(EXIT_FAILURE);
	}
} // dorealpath()
