/* akifiletable - Filetable builder for AKI Corporation's N64 wrestling games
 * written by freem
 *
 * This program is licensed under the Unlicense.
 * See the "UNLICENSE" file for more information.
 */
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "frozen.h"
#include "akifiletable.h"

// todo: figure out "kludge bytes" to handle certain filetable index oddities
// - no mercy: last 4 bytes are 01 28 63 52, which is an address before the
// end of the file data

/*============================================================================
 * Program input arguments
 *============================================================================*/

/* print program usage */
static void Usage(char* execName){
	printf("usage: %s -l LIST_FILE [-o OUTPUT_FILETABLE] [-i OUTPUT_INDEX] [-h OUTPUT_HEADER] [-v] [-d]\n", execName);
	printf("  -l LIST_FILE         JSON filetable list (see docs for format info)\n");
	printf("  -o OUTPUT_FILETABLE  output filetable binary filename (optional; default: %s)\n",defaultArgs.outDataFilename);
	printf("  -i OUTPUT_INDEX      output filetable index filename (optional; default: %s)\n",defaultArgs.outIndexFilename);
	printf("  -h OUTPUT_HEADER     output filetable symbol header filename (optional; default: %s)\n",defaultArgs.outHeaderFilename);
	printf("  -n OUTPUT_INCLUDE    output filetable linker symbol filename (optional; default: %s)\n",defaultArgs.outLinkerFilename);
	printf("  -a OUTPUT_INCLUDE    output filetable assembly symbol filename (optional; default: %s)\n",defaultArgs.outIncludeFilename);
	printf("  -v                   verbose mode (show info about each entry; likely inflates build times)\n");
	printf("  -d                   output filetable symbol header only\n");
}

/* handle program arguments */
static int parseArgs(int argc, char* argv[], InputArgs* outArgs)
{
	for(int i = 1; i < argc; i++){
		if(argv[i][0] == '-'){
			switch(argv[i][1]){
				case 'l': /* input list file */
					if(++i >= argc){
						printf("Error: -l requires a filename\n");
						return 0;
					}
					outArgs->listFilename = argv[i];
					break;

				case 'o': /* output filetable data */
					if(++i >= argc){
						printf("Error: -o requires a filename\n");
						return 0;
					}
					outArgs->outDataFilename = argv[i];
					break;

				case 'i': /* output filetable index */
					if(++i >= argc){
						printf("Error: -i requires a filename\n");
						return 0;
					}
					outArgs->outIndexFilename = argv[i];
					break;

				case 'h': /* output filetable header */
					if(++i >= argc){
						printf("Error: -h requires a filename\n");
						return 0;
					}
					outArgs->outHeaderFilename = argv[i];
					break;

				case 'n': /* output filetable linker include */
					if(++i >= argc){
						printf("Error: -n requires a filename\n");
						return 0;
					}
					outArgs->outLinkerFilename = argv[i];
					break;

				case 'a': /* output filetable assembly include */
					if(++i >= argc){
						printf("Error: -a requires a filename\n");
						return 0;
					}
					outArgs->outIncludeFilename = argv[i];
					break;

				case 'v': /* enable verbose mode */
					outArgs->verbose = true;
					break;

				case 'd': /* only generate header */
					outArgs->headerOnly = true;
					break;

				default:
					printf("Unrecognized option '%s'.\n", argv[i]);
					return 0;
					break;
			}
		}
	}

	return 1; /* ok */
}

/* validate the passed in arguments */
static int ValidateArgs(const InputArgs* args)
{
	if(args->listFilename == NULL){
		printf("Error: LIST_FILE (-l) must be defined.\n");
		return 0;
	}
	
	return 1; /* ok */
}

/* entry point */
int main(int argc, char* argv[]){
	InputArgs progArgs = defaultArgs;
	FILE* outData; /* filetable data */
	FILE* outIndex; /* filetable index */
	FILE* outHeader; /* filetable header */
	FILE* outLinker; /* filetable symbols for linker */
	FILE* outInclude; /* filetable symbols for assembly */
	int numFiles = 0;
	unsigned long curLocation = 0;
	int curFileLength;
	int decodedFileLength;

	printf("akifiletable v1.6 - Filetable builder for N64 AKI wrestling games\n");

	if(argc <= 1){
		Usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	int parseArgsResult = parseArgs(argc, argv, &progArgs);
	if(!parseArgsResult || !ValidateArgs(&progArgs)){
		exit(EXIT_FAILURE);
	}
	
	char *list = json_fread(progArgs.listFilename);
	if(list == NULL){
		printf("Error: Unable to read from input list file '%s'.\n", progArgs.listFilename);
		exit(EXIT_FAILURE);
	}

	if(!progArgs.headerOnly){
		outData = fopen(progArgs.outDataFilename,"wb");

		if(outData == NULL){
			printf("Error: Unable to create output filetable file '%s'.\n",progArgs.outDataFilename);
			exit(EXIT_FAILURE);
		}

		outIndex = fopen(progArgs.outIndexFilename,"wb");
		if(outIndex == NULL){
			printf("Error: Unable to create output index file '%s'.\n",progArgs.outIndexFilename);
			exit(EXIT_FAILURE);
		}
	}
	
	outHeader = fopen(progArgs.outHeaderFilename,"w");
	if(outHeader == NULL){
		printf("Error: Unable to create output header file '%s'.\n",progArgs.outHeaderFilename);
		exit(EXIT_FAILURE);
	}
	fprintf(outHeader, "/* Generated by akifiletable, DO NOT EDIT */\n");
	fprintf(outHeader, "#ifndef _FILETABLE_H_\n");
	fprintf(outHeader, "#define _FILETABLE_H_\n\n");

	outLinker = fopen(progArgs.outLinkerFilename,"w");
	if(outLinker == NULL){
		printf("Error: Unable to create output linker file '%s'.\n",progArgs.outLinkerFilename);
		exit(EXIT_FAILURE);
	}
	fprintf(outLinker, "/* Generated by akifiletable, DO NOT EDIT */\n");

	outInclude = fopen(progArgs.outIncludeFilename,"w");
	if(outInclude == NULL){
		printf("Error: Unable to create output include file '%s'.\n",progArgs.outIncludeFilename);
		exit(EXIT_FAILURE);
	}
	fprintf(outInclude, "# Generated by akifiletable, DO NOT EDIT\n");

	/* and now the fun part begins: parse the json file */
	printf("Loading list from '%s'; this might take a while.\n", progArgs.listFilename);
	struct json_token tok;
	int len = strlen(list);
	ListEntry entry;

	/* count number of valid entries first
	 * this is slow for reasons I don't particularly understand
	 */
	for(int i = 0; json_scanf_array_elem(list, len, "", i, &tok) > 0; i++){
		if(json_scanf(tok.ptr, tok.len,"{ file:%Q }", &entry.file) >= 0){
			++numFiles;
			printf("Counting entries... 0x%04X\r",numFiles);
		}
	}
	printf("\nTotal number of files: %d (0x%04X)\n", numFiles, numFiles);
	fprintf(outHeader, "/* Filetable Information */\n");
	fprintf(outHeader, "#define FILETABLE_NUM_FILES 0x%04X\n", numFiles);
	fprintf(outHeader, "#define FILETABLE_INDEX_SIZE (FILETABLE_NUM_FILES * 4)\n\n");

	fprintf(outLinker, "FILETABLE_NUM_FILES = 0x%04X;\n",numFiles);
	fprintf(outLinker, "FILETABLE_INDEX_SIZE = (FILETABLE_NUM_FILES * 4);\n");

	fprintf(outInclude, ".set FILETABLE_NUM_FILES, 0x%04X;\n",numFiles);
	fprintf(outInclude, ".set FILETABLE_INDEX_SIZE, (FILETABLE_NUM_FILES * 4);\n");

	fprintf(outHeader, "/* friendly names for filetable entries */\n");
	printf("Building filetable; this _will_ take a while.\n");
	for(int i = 0; json_scanf_array_elem(list, len, "", i, &tok) > 0; i++){
		curFileLength = 0;
		decodedFileLength = 0;
		/* fallback values for optional data */
		entry.symbol = NULL;
		entry.exportsize = false;
		entry.exportsizepad = 0;

		printf("Processing entry 0x%04X/0x%04X (%.2f%%)\r",i+1,numFiles,(((i+1)*1.0f)/(numFiles*1.0f))*100.0f);

		if(progArgs.verbose){
			printf("\nInfo for Entry 0x%04X\n",i+1);
		}

		// todo: check for errors (return value negative number)
		json_scanf(tok.ptr, tok.len,
			"{ file:%Q, lzss:%B, symbol:%Q, exportsize:%B, exportsizepad:%d }",
			&entry.file, &entry.lzss, &entry.symbol, &entry.exportsize, &entry.exportsizepad
		);

		if(progArgs.verbose){
			printf("file = %s\n",entry.file);
			printf("lzss = %s\n",entry.lzss ? "true" : "false");
			if(entry.symbol != NULL){
				printf("symbol = FILEID_%s\n",entry.symbol);
			}
			printf("exportsize = %s\n",entry.exportsize ? "true" : "false");
			if(entry.exportsizepad > 0){
				printf("exportsizepad = %d\n",entry.exportsizepad);
			}
		}

		/* try finding file and add to filetable if found */
		FILE *curFile = fopen(entry.file, "rb");
		if(curFile == NULL){
			printf("Error: Unable to open file '%s'.\n",entry.file);
			exit(EXIT_FAILURE);
		}

		/* get input file length */
		fseek(curFile, 0, SEEK_END);
		curFileLength = ftell(curFile);
		if(progArgs.verbose){
			printf("File size: %d bytes\n",curFileLength);
		}
		rewind(curFile);

		char *curFileData = (char *)malloc(curFileLength);
		if(curFileData == NULL){
			printf("Error: Unable to allocate memory for file '%s'.\n",entry.file);
			exit(EXIT_FAILURE);
		}
		// todo: check value of result
		size_t result = fread(curFileData, 1, curFileLength, curFile);

		if(!progArgs.headerOnly){
			fwrite(curFileData, 1, curFileLength, outData);
		}
		fclose(curFile);

		// if this is an LZSS'd file, we need to set decodedFileLength
		// using the first four bytes of the file
		if(entry.lzss){
			decodedFileLength += (curFileData[0] & 0xFF) << 24;
			decodedFileLength += (curFileData[1] & 0xFF) << 16;
			decodedFileLength += (curFileData[2] & 0xFF) << 8;
			decodedFileLength += (curFileData[3] & 0xFF);
		}

		if(curFileData != NULL){
			free(curFileData);
		}

		/* add padding if neccessary */
		if(curFileLength % 2 != 0){
			if(!progArgs.headerOnly){
				fputc('\0', outData);
			}
			++curFileLength;
		}

		if(progArgs.verbose){
			printf("Location: 0x%lX\n", curLocation);
		}

		/* update filetable index */
		if(!progArgs.headerOnly){
			fputc((curLocation & 0xFF000000) >> 24, outIndex);
			fputc((curLocation & 0x00FF0000) >> 16, outIndex);
			fputc((curLocation & 0x0000FF00) >> 8, outIndex);
			if(entry.lzss){
				fputc((curLocation & 0x000000FE) | 1, outIndex);
			}
			else{
				fputc((curLocation & 0x000000FE), outIndex);
			}
		}

		curLocation += curFileLength;

		/* header export */
		if(entry.symbol != NULL){
			fprintf(outHeader, "#define FILEID_%s 0x%04X\n", entry.symbol, i+1);
			fprintf(outLinker, "FILEID_%s = 0x%04X;\n", entry.symbol, i+1);
			fprintf(outInclude, ".set FILEID_%s, 0x%04X;\n", entry.symbol, i+1);

			if(entry.exportsize){
				int outSize;
				if(entry.lzss){
					outSize = decodedFileLength;
				}
				else{
					outSize = curFileLength;
				}
				if(entry.exportsizepad > 0){
					outSize += entry.exportsizepad;
				}
				fprintf(outHeader, "#define FILESIZE_%s %d\n", entry.symbol, outSize);
				fprintf(outLinker, "FILESIZE_%s = %d;\n", entry.symbol, outSize);
				fprintf(outInclude, ".set FILESIZE_%s,%d;\n", entry.symbol, outSize);
			}
		}
		else{
			if(entry.exportsize){
				printf("Error: exportsize requires a symbol name to be defined!\n");
				exit(EXIT_FAILURE);
			}
			if(entry.exportsizepad > 0){
				printf("Error: exportsizepad requires a symbol name to be defined!\n");
				exit(EXIT_FAILURE);
			}
		}

		free(entry.file);
		if(entry.symbol != NULL){ free(entry.symbol); }
		if(progArgs.verbose){ printf("\n"); }
	}

	/* output offset for filetable index */
	if(!progArgs.headerOnly){
		fputc((curLocation & 0xFF000000) >> 24, outIndex);
		fputc((curLocation & 0x00FF0000) >> 16, outIndex);
		fputc((curLocation & 0x0000FF00) >> 8, outIndex);
		fputc((curLocation & 0x000000FF), outIndex);
	}

	fprintf(outHeader, "\n#endif\n");

	printf("\nFiletable build process complete.\n");

	/* final cleanup */
	if(list != NULL){
		free(list);
	}

	if(!progArgs.headerOnly){
		fclose(outData);
		fclose(outIndex);
	}

	fclose(outHeader);
	fclose(outLinker);
	fclose(outInclude);

	return EXIT_SUCCESS;
}
