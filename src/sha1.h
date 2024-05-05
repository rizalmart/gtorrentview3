/**
 * @file sha1.h
 *
 * @brief header file for sha.c
 *
 * Original code: Copyright (C) 2001-2003  Christophe Devine
 * 
 * 2004-10-08  Alejandro Claro <aleo@apollyon.no-ip.com>
 *   - Variables types adapted to Glib types for compatibilities reasons.
 *   - SHA1 function, compute sha1 of a memory block using just 1 function.
 *   - R, S and P macros moved outside sha1_process function.
 *   - SHA_DIGEST_LENGTH defined and sustituted where needed.
 *
 *  Fri Oct  8 20:25:48 2004
 *  Copyright (C) 2004  Alejandro Claro
 *  aleo@apollyon.no-ip.com
 */

#ifndef _SHA1_H
#define _SHA1_H

#define SHA_DIGEST_LENGTH  20

typedef struct
{
    guint32 total[2];
    guint32 state[5];
    guint8  buffer[64];
} sha1_context;

G_BEGIN_DECLS

void sha1_starts(sha1_context *ctx);
void sha1_update(sha1_context *ctx, guint8 *input, guint32 length);
void sha1_finish(sha1_context *ctx, guint8 digest[20]);

guint8 *SHA1(guint8 *input, guint32 length, guint8 *digest);

G_END_DECLS

#endif /* _SHA1_H */
