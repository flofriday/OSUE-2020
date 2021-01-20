 /**
  * @author briemelchen
  * @date 03.01.2020
  * @brief implementation of  @see gziputil.h.
  * @details implements gzip compressing/decompressing using C's zlib API (inflate, deflate)
  * for more information @see gziputil.h
 **/

#include "gziputil.h"

/**
 * @brief set's up a given z_stream struct for deflating(compressing)
 * @details initalies needed values for the struct.
 * @param stream pointer to the z_stream which should be setted up.
 * @return return-code of deflateInit2
 **/
static int setup_zstream_deflate(z_stream *stream);

/**
 * @brief set's up a given z_stream struct for inflating(decompressing)
 * @details initalies needed values for the struct.
 * @param stream pointer to the z_stream which should be setted up.
 * @return return-code of inflateInit2
 **/
static int setup_zstream_inflate(z_stream *stream);

int compress_gzip(FILE *source, FILE *dest, int *content_size)
{
    int return_value, state;
    // in/out buffers used for deflate
    Bytef in[GZIP_CHUNK_SIZE], out[GZIP_CHUNK_SIZE];
    unsigned long amount_deflated;

    z_stream stream;
    return_value = setup_zstream_deflate(&stream);
    if (return_value != Z_OK)
    {
        return Z_ERRNO;
    }

    do // loop till eof
    {
        // read data from input
        stream.avail_in = fread(in, 1, GZIP_CHUNK_SIZE, source);
        stream.next_in = in;

        // error while reading
        if (ferror(source))
        {
            deflateEnd(&stream);
            return Z_ERRNO;
        }
        // no more to compress indicates to leave
        if (feof(source))
            state = Z_FINISH;
        else // still something to read
            state = 0;

        do // deflate as long their is something to deflate
        {
            stream.avail_out = GZIP_CHUNK_SIZE;
            stream.next_out = out;
            return_value = deflate(&stream, state);
            amount_deflated = GZIP_CHUNK_SIZE - stream.avail_out;
            if (dest != NULL) // content should be written
            {
                if (fwrite(out, 1, amount_deflated, dest) != amount_deflated || ferror(dest)) // write to output
                {
                    deflateEnd(&stream);
                    return Z_ERRNO;
                }
            }

            *content_size += amount_deflated; // update size of compressed file
        } while (stream.avail_out == 0);
    } while (state != Z_FINISH);
    rewind(source); // rewind, because maybe file is needed again in same process
    deflateEnd(&stream); // cleanup
    return return_value;
}

int decompress_gzip(FILE *outF, FILE *socket)
{
    int return_value, amount_inflated;
    Bytef in[GZIP_CHUNK_SIZE], out[GZIP_CHUNK_SIZE];
    z_stream stream;
    return_value = setup_zstream_inflate(&stream);

    do //decompress till whole file has been decompressed (indicated by inflate!)
    {
        stream.avail_in = fread(in, 1, GZIP_CHUNK_SIZE, socket);
        if (ferror(socket))
        {
            inflateEnd(&stream);
            return Z_ERRNO;
        }
        if (stream.avail_in == 0)
            break;
        stream.next_in = in;
        do // generate output as long there is something to inflate
        {
            stream.avail_out = GZIP_CHUNK_SIZE;
            stream.next_out = out;
            return_value = inflate(&stream, Z_NO_FLUSH);
            // inflating failed
            if (return_value == Z_NEED_DICT || return_value == Z_DATA_ERROR || return_value == Z_MEM_ERROR)
            {
                inflateEnd(&stream);
                return return_value;
            }
            amount_inflated = GZIP_CHUNK_SIZE - stream.avail_out;
            if (fwrite(out, 1, amount_inflated, outF) != amount_inflated || ferror(outF)) // write to output
            {
                inflateEnd(&stream);
                return Z_ERRNO;
            }

        } while (stream.avail_out == 0);

    } while (return_value != Z_STREAM_END);

    inflateEnd(&stream);
    return return_value;
}

static int setup_zstream_deflate(z_stream *stream)
{

    // set to NULL so zlib uses the default routines
    stream->zalloc = Z_NULL;
    stream->zfree = Z_NULL;
    stream->opaque = Z_NULL;
    // 31 because 15 + 16(marks gzip); DEFAULT -> best compromiss between speed and ratio
    return deflateInit2(stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 31, 8, Z_DEFAULT_STRATEGY);
}

static int setup_zstream_inflate(z_stream *stream)
{

    // set to NULL so zlib uses the default routines
    stream->zalloc = Z_NULL;
    stream->zfree = Z_NULL;
    stream->opaque = Z_NULL;
    stream->avail_in = 0;
    stream->next_in = Z_NULL;
    // 31 because 15 + 16(marks gzip)
    return inflateInit2(stream, 31);
}