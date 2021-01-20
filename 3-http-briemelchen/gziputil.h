/**
 * @author briemelchen
 * @date 03.01.2020
 * @brief Module which offers function to compress/decompress data(files) using gzip.
 * @details zlib is used as libary offering does functionality to inflate/deflate data.
 * Implementation relies on the zlib documentation, manuals and examples (https://zlib.net/)
 * Because files should be encoded to gzip, it is not sufficient to use zlib's compress and decompress,
 * because gzip needs specific window-bits. 
 * For implemenmtation details @see gziputil.c
 **/

#ifndef gzip_util_h
#define gzip_util_h
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#define GZIP_CHUNK_SIZE 256 // chunk size for compress/decompres data

/**
 * @brief compresses a file into gzip-format and may writes the compressed content to another file (in our case socket!)
 * @details uses zlib libary and the provied deflate functions to perform the compressing.
 *          the compressing is performed using a compromiss between speed and compress.
 *          If the dest-file param is non-null, the compressed content is written  to the file.
 * @param source file which reading and compressing should be peformed from.
 * @param dest file where the compressed content should be written to. CAN be NULL, than only the
 *              size of the compressed file is calculated.
 * @param content_size pointer to an integer, where the size of the compressed file should be written.
 * @return 0 on success, otherwise a value non equal to 0 is returned.
 **/
int compress_gzip(FILE *source, FILE *dest, int *content_size);

/**
 * @brief decompresses a file from gzip to plain-text/binary and writes it to an given out file. 
 * @details uses zlib libary and the provied inflate functions to perform the decompressing.
 *          Decompresses whole file till EOF is reached.
 * @param outF file where the decoded content should be written to
 * @param socket where the gzip content should be read from
 * @return 0 on success, otherwise a value non equal to 0 is returned.
 **/
int decompress_gzip(FILE *out, FILE *socket);
#endif